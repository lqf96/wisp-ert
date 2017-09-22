#include <stdlib.h>
#include "endpoint.h"

//WTP packet handlers
wtp_pkt_handler_t wtp_pkt_handlers[];

/**
 * @brief Handle WTP open packet.
 *
 * @param self WTP endpoint instance.
 * @param buf Received packets buffer.
 * @return Error code if failed, otherwise WIO_OK.
 */
static wtp_status_t wtp_handle_open(
    wtp_t* self,
    wio_buf_t* buf
) {
    //Verify checksum
    WIO_TRY(wtp_verify_checksum(self, buf))

    wtp_tx_ctrl_t* tx_ctrl = &self->_tx_ctrl;
    //Packet buffer
    wio_buf_t* pkt_buf = &tx_ctrl->_pkt_buf;

    //Open downlink
    self->_downlink_state = WTP_STATE_OPENED;

    //Send acknowledgement packet
    WIO_TRY(wtp_tx_begin_packet(tx_ctrl, WTP_PKT_ACK))
    WIO_TRY(wio_write(pkt_buf, &tx_ctrl->_seq_num, 2))
    WIO_TRY(wtp_tx_end_packet(tx_ctrl))
    //Invoke and remove callback
    WIO_TRY(wtp_trigger_event(self, WTP_EVENT_OPEN, WIO_OK, NULL))

    return WIO_OK;
}

/**
 * @brief Handle WTP acknowledgement packet.
 *
 * @param self WTP endpoint instance.
 * @param buf Received packets buffer.
 * @return Error code if failed, otherwise WIO_OK.
 */
static wtp_status_t wtp_handle_ack(
    wtp_t* self,
    wio_buf_t* buf
) {
    //Sequence number
    uint16_t seq_num;
    WIO_TRY(wio_read(buf, &seq_num, 2))
    //Verify checksum
    WIO_TRY(wtp_verify_checksum(self, buf))

    //Connected acknowledgement
    if (self->_uplink_state==WTP_STATE_OPENING)
        self->_uplink_state = WTP_STATE_OPENED;
    //Connection closed acknowledgement
    else if (self->_uplink_state==WTP_STATE_CLOSING)
        self->_uplink_state = WTP_STATE_CLOSED;
    else {
        //Number of messages sent
        uint8_t n_sent_msgs = 0;
        //Send callbacks and closure data queue
        wio_queue_t* send_cb_queue = &self->_send_cb_queue;
        wio_queue_t* send_cb_data_queue = &self->_send_cb_data_queue;

        //Handle acknowledgement with transmit control
        WIO_TRY(wtp_tx_handle_ack(&self->_tx_ctrl, seq_num, &n_sent_msgs))
        //Invoke callback functions
        for (uint8_t i=0;i<n_sent_msgs;i++) {
            //Callback function and closure data
            wio_callback_t cb = NULL;
            void* cb_data = NULL;

            //Pop callback and closure data from queue
            if (send_cb_queue->size>0) {
                WIO_TRY(wio_queue_pop(send_cb_queue, &cb))
                WIO_TRY(wio_queue_pop(send_cb_data_queue, &cb_data))
            }

            //Invoke callback (Ignore errors)
            if (cb)
                cb(cb_data, WIO_OK, NULL);
        }
    }

    return WIO_OK;
}

/**
 * @brief Handle WTP message packet.
 *
 * @param self WTP endpoint instance.
 * @param buf Received packets buffer.
 * @param begin_msg True for WTP_PKT_BEGIN_MSG and false for WTP_PKT_CONT_MSG.
 * @return Error code if failed, otherwise WIO_OK.
 */
static wtp_status_t wtp_handle_msg_packet(
    wtp_t* self,
    wio_buf_t* buf,
    bool begin_msg
) {
    //Number of messages received
    uint8_t n_msgs = 0;

    //New message size
    uint16_t new_msg_size = 0;
    if (begin_msg)
        WIO_TRY(wio_read(buf, &new_msg_size, 2))
    //Sequence number
    uint16_t seq_num;
    WIO_TRY(wio_read(buf, &seq_num, 2))
    //Payload size
    uint8_t payload_size;
    WIO_TRY(wio_read(buf, &payload_size, 1))
    //Get pointer to payload
    uint8_t* payload = buf->buffer+buf->pos_a;
    //Update read cursor position
    buf->pos_a += payload_size;
    //Verify checksum
    WIO_TRY(wtp_verify_checksum(self, buf))

    //Handle packet with WTP receive control (Ignore errors)
    wtp_rx_handle_packet(
        &self->_rx_ctrl,
        seq_num,
        payload,
        payload_size,
        new_msg_size,
        &n_msgs
    );

    //Message data buffer
    wio_buf_t* msg_data_buf = &self->_rx_ctrl._msg_data_buf;
    //Receive callback and closure data queue
    wio_queue_t* recv_cb_queue = &self->_recv_cb_queue;
    wio_queue_t* recv_cb_data_queue = &self->_recv_cb_data_queue;

    //Invoke callback functions with received messages
    for (uint8_t i=0;i<n_msgs;i++) {
        //Callback function and closure data
        wio_callback_t cb = NULL;
        void* cb_data = NULL;
        //Pop callback and closure data from queue
        if (recv_cb_queue->size>0) {
            WIO_TRY(wio_queue_pop(recv_cb_queue, &cb))
            WIO_TRY(wio_queue_pop(recv_cb_data_queue, &cb_data))
        }

        //Message size
        uint16_t msg_size;
        //Current message buffer
        wio_buf_t msg_buf;

        //Read message size
        WIO_TRY(wio_read(msg_data_buf, &msg_size, 2))
        //Get message data
        uint8_t* msg_data = msg_data_buf->buffer+msg_data_buf->pos_a;
        //Update read cursor
        msg_data_buf->pos_a += msg_size;
        //Initialize current message buffer
        WIO_TRY(wio_buf_init(&msg_buf, msg_data, msg_size))

        //Invoke callback (Ignore errors)
        if (cb)
            cb(cb_data, WIO_OK, &msg_buf);
    }

    //Transmit control type
    wtp_tx_ctrl_t* tx_ctrl = &self->_tx_ctrl;
    //Packet buffer type
    wio_buf_t* pkt_buf = &tx_ctrl->_pkt_buf;

    //Send acknowledge packet back to server
    WIO_TRY(wtp_tx_begin_packet(tx_ctrl, WTP_PKT_ACK))
    WIO_TRY(wio_write(pkt_buf, &self->_rx_ctrl._seq_num, 2))
    WIO_TRY(wtp_tx_end_packet(tx_ctrl))

    return WIO_OK;
}

/**
 * @brief Handle WTP begin message packet.
 *
 * @param self WTP endpoint instance.
 * @param buf Received packets buffer.
 * @return Error code if failed, otherwise WIO_OK.
 */
static wtp_status_t wtp_handle_begin_msg(
    wtp_t* self,
    wio_buf_t* buf
) {
    return wtp_handle_msg_packet(self, buf, true);
}

/**
 * @brief Handle WTP continue message packet.
 *
 * @param self WTP endpoint instance.
 * @param buf Received packets buffer.
 * @return Error code if failed, otherwise WIO_OK.
 */
static wtp_status_t wtp_handle_cont_msg(
    wtp_t* self,
    wio_buf_t* buf
) {
    return wtp_handle_msg_packet(self, buf, false);
}

/**
 * @brief Handle WTP set parameter message packet.
 *
 * @param self WTP endpoint instance.
 * @param buf Received packets buffer.
 */
static wtp_status_t wtp_handle_set_param(
    wtp_t* self,
    wio_buf_t* buf
) {
    //Read parameter code
    wtp_param_t param_code;
    WIO_TRY(wio_read(buf, &param_code, 1))

    switch (param_code) {
        //TODO: WTP_PARAM_WINDOW_SIZE
        case WTP_PARAM_WINDOW_SIZE: {
            break;
        }
        //WTP_PARAM_READ_SIZE
        case WTP_PARAM_READ_SIZE: {
            //Suggested READ size
            uint8_t read_size;
            WIO_TRY(wio_read(buf, &read_size, 1))
            //Verify checksum
            WIO_TRY(wtp_verify_checksum(self, buf))

            //Set READ size
            self->_tx_ctrl._read_size = read_size;

            break;
        }
    }

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_init(
    wtp_t* self,
    uint8_t* epc_mem,
    uint16_t epc_buf_size,
    uint8_t* read_mem,
    uint8_t* write_mem,
    uint16_t window_size,
    uint16_t timeout,
    uint16_t tx_buf_size,
    uint16_t rx_buf_size,
    uint8_t n_send,
    uint8_t n_recv
) {
    //Downlink state
    self->_downlink_state = WTP_STATE_CLOSED;
    //Uplink state
    self->_uplink_state = WTP_STATE_CLOSED;

    //EPC buffer
    WIO_TRY(wio_buf_init(&self->_epc_buf, epc_mem, epc_buf_size))
    //Read memory
    self->_read_mem = read_mem;
    //Write memory
    self->_write_mem = write_mem;

    //Read memory loaded flag
    self->_read_mem_loaded = false;

    //Initialize packet begin position
    self->_pkt_begin = 0;

    //Transmit control memory unit
    uint16_t tx_mem_unit = tx_buf_size/4;
    //Receive control memory unit
    uint16_t rx_mem_unit = rx_buf_size/2;

    //Transmit control
    WIO_TRY(wtp_tx_init(
        &self->_tx_ctrl,
        window_size,
        timeout,
        24,
        tx_mem_unit,
        tx_mem_unit*3,
        n_send*3,
        n_send
    ))
    //Receive control
    WIO_TRY(wtp_rx_init(
        &self->_rx_ctrl,
        window_size,
        rx_mem_unit,
        rx_mem_unit,
        n_recv
    ))

    //Send message callbacks queue
    WIO_TRY(wio_queue_init(&self->_send_cb_queue, sizeof(wio_callback_t), n_send))
    //Send message callback closure data queue
    WIO_TRY(wio_queue_init(&self->_send_cb_data_queue, sizeof(void*), n_send))

    //Receive callbacks queue
    WIO_TRY(wio_queue_init(&self->_recv_cb_queue, sizeof(wio_callback_t), n_recv))
    //Receive callbacks closure data queue
    WIO_TRY(wio_queue_init(&self->_recv_cb_data_queue, sizeof(void*), n_recv))

    //Event callbacks and closure data
    for (size_t i=0;i<WTP_EVENT_MAX;i++)
        self->_event_cb[i] = NULL;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_fini(
    wtp_t* self
) {
    //Transmit control
    WIO_TRY(wtp_tx_fini(&self->_tx_ctrl))
    //Receive control
    WIO_TRY(wtp_rx_fini(&self->_rx_ctrl))

    //Send message callbacks queue
    free(self->_send_cb_queue.data);
    //Send message callback closure data queue
    free(self->_send_cb_data_queue.data);

    //Receive callbacks queue
    free(self->_recv_cb_queue.data);
    //Receive callbacks closure data queue
    free(self->_recv_cb_data_queue.data);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_connect(
    wtp_t* self
) {
    wtp_tx_ctrl_t* tx_ctrl = &self->_tx_ctrl;

    //Uplink already opened
    if (self->_uplink_state==WTP_STATE_OPENED)
        return WIO_ERR_ALREADY;
    //Set uplink state
    self->_uplink_state = WTP_STATE_OPENING;

    //Construct WTP connect packet
    WIO_TRY(wtp_tx_begin_packet(tx_ctrl, WTP_PKT_OPEN))
    //End packet
    WIO_TRY(wtp_tx_end_packet(tx_ctrl))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_close(
    wtp_t* self
) {
    //TODO: Close

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_send(
    wtp_t* self,
    uint8_t* data,
    uint16_t size,
    void* cb_data,
    wio_callback_t cb
) {
    //Add callback and closure data to queue
    WIO_TRY(wio_queue_push(&self->_send_cb_queue, &cb))
    WIO_TRY(wio_queue_push(&self->_send_cb_data_queue, &cb_data))

    wtp_tx_ctrl_t* tx_ctrl = &self->_tx_ctrl;
    //New message READ OpSpec information
    wtp_tx_read_info_t* read_info;

    //Add message to transmit control
    WIO_TRY(wtp_tx_add_msg(tx_ctrl, data, size, &read_info))

    wio_buf_t* pkt_buf = &tx_ctrl->_pkt_buf;
    //Send request uplink packet
    WIO_TRY(wtp_tx_begin_packet(tx_ctrl, WTP_PKT_REQ_UPLINK))
    WIO_TRY(wio_write(pkt_buf, &read_info->_n_reads, 1))
    WIO_TRY(wio_write(pkt_buf, &read_info->_size, 1))
    WIO_TRY(wtp_tx_end_packet(tx_ctrl))

    //Load READ memory when necessary
    if (!self->_read_mem_loaded)
        WIO_TRY(wtp_load_read_mem(self))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_recv(
    wtp_t* self,
    void* cb_data,
    wio_callback_t cb
) {
    //Add callback and closure data to queue
    WIO_TRY(wio_queue_push(&self->_recv_cb_queue, &cb))
    WIO_TRY(wio_queue_push(&self->_recv_cb_data_queue, &cb_data))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_on_event(
    wtp_t* self,
    wtp_event_t event,
    void* cb_data,
    wio_callback_t cb
) {
    //Invalid event
    if (event>=WTP_EVENT_MAX)
        return WIO_ERR_INVALID;

    //Set callback and closure data
    self->_event_cb[event] = cb;
    self->_event_cb_data[event] = cb_data;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_trigger_event(
    wtp_t* self,
    wtp_event_t event,
    wtp_status_t status,
    void* result
) {
    wio_callback_t cb = self->_event_cb[event];
    void* cb_data = self->_event_cb_data[event];

    //Invoke event callback
    if (cb)
        return cb(cb_data, status, result);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_load_read_mem(
    wtp_t* self
) {
    //Read buffer
    wio_buf_t* read_buf = WIO_INST_PTR(wio_buf_t);
    //Transmit control
    wtp_tx_ctrl_t* tx_ctrl = &self->_tx_ctrl;
    //Read information queue
    wio_queue_t* read_info_queue = &tx_ctrl->_read_info_queue;

    //No fragments to load
    if (read_info_queue->size==0) {
        self->_read_mem_loaded = false;
        //Set the first byte of the Read buffer to WTP_PKT_END
        self->_read_mem[0] = WTP_PKT_END;

        return WIO_OK;
    }
    //Set Read memory loaded flag
    self->_read_mem_loaded = true;

    //Get next READ OpSpec size
    wtp_tx_read_info_t* read_info = WIO_QUEUE_END(read_info_queue, wtp_tx_read_info_t);
    uint8_t read_size = read_info->_size;
    //Update number of reads
    read_info->_n_reads--;
    //Pop read information when necessary
    if (read_info->_n_reads==0)
        WIO_TRY(wio_queue_pop(read_info_queue, NULL))
    //Initialize read buffer
    WIO_TRY(wio_buf_init(read_buf, self->_read_mem, read_size))

    //Data fragments queue
    wio_queue_t* fragments_queue = &tx_ctrl->_fragments_queue;
    //Send fragment
    wtp_tx_fragment_t* send_fragment = NULL;

    uint8_t queue_index = fragments_queue->end;
    //Try to find existing data fragment to send
    for (uint8_t i=0;i<fragments_queue->size;i++) {
        wtp_tx_fragment_t* fragment = WIO_QUEUE_AT(fragments_queue, wtp_tx_fragment_t, queue_index);

        if (fragment->_need_send) {
            send_fragment = fragment;
            break;
        }

        //Update queue index
        queue_index++;
        if (queue_index>=fragments_queue->capacity)
            queue_index = 0;
    }
    //Try to make new data fragment to send
    if (!send_fragment)
        WIO_TRY(wtp_tx_make_fragment(tx_ctrl, tx_ctrl->_read_size, &send_fragment))
    //No more fragments to send
    if (!send_fragment)
        return WIO_OK;

    //Write packet header
    if (send_fragment->_msg_size) {
        WIO_TRY(wio_write(read_buf, &WTP_PKT_BEGIN_MSG, 1))
        WIO_TRY(wio_write(read_buf, &send_fragment->_msg_size, 2))
    } else {
        WIO_TRY(wio_write(read_buf, &WTP_PKT_CONT_MSG, 1))
    }
    WIO_TRY(wio_write(read_buf, &send_fragment->_seq_num, 2))
    WIO_TRY(wio_write(read_buf, &send_fragment->_size, 1))
    //Write packet data
    WIO_TRY(wio_write(read_buf, send_fragment->_data, send_fragment->_size))
    //Write end packet byte (Ignore failure)
    wio_write(read_buf, &WTP_PKT_END, 1);

    //TODO: Set fragment timeout

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_handle_blockwrite(
    wtp_t* self
) {
    //Write status
    wtp_status_t status;
    //Write buffer
    wio_buf_t* write_buf = WIO_INST_PTR(wio_buf_t);
    //Packet type
    wtp_pkt_t pkt_type;

    //BlockWrite size
    uint16_t blockwrite_size = *self->_write_mem;
    //BlockWrite memory
    uint8_t* blockwrite_mem = self->_write_mem+1;

    //Initialize write buffer
    WIO_TRY(wio_buf_init(write_buf, blockwrite_mem, blockwrite_size))
    //Read packets
    while (true) {
        //Set packet begin position
        self->_pkt_begin = write_buf->pos_a;

        //Read packet type
        status = wio_read(write_buf, &pkt_type, 1);
        //No more packets
        if (status==WIO_ERR_OUT_OF_RANGE)
            pkt_type = WTP_PKT_END;
        //Read error
        else if (status!=WIO_OK)
            return status;

        //No more packets
        if (pkt_type==WTP_PKT_END)
            break;
        //Unsupported operation
        if (pkt_type>=WTP_PKT_MAX)
            return WTP_ERR_UNSUPPORT_OP;

        //Handle packet with respective handler
        wtp_pkt_handler_t handler = wtp_pkt_handlers[pkt_type];
        if (!handler)
            return WTP_ERR_UNSUPPORT_OP;
        WIO_TRY(handler(self, write_buf))
    }

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_verify_checksum(
    wtp_t* self,
    wio_buf_t* write_buf
) {
    //Get packet end position
    uint16_t pkt_end = write_buf->pos_a;
    //Calculate checksum
    uint8_t calc_checksum = wtp_xor_checksum(write_buf->buffer, self->_pkt_begin, pkt_end);

    //Read checksum from buffer
    uint8_t pkt_checksum;
    WIO_TRY(wio_read(write_buf, &pkt_checksum, 1))

    return (calc_checksum==pkt_checksum)?WIO_OK:WIO_ERR_INVALID;
}

/**
 * {@inheritDoc}
 */
uint8_t wtp_xor_checksum(
    uint8_t* mem,
    uint16_t begin,
    uint16_t end
) {
    uint8_t checksum = 0;

    for (uint16_t i=begin;i<end;i++)
        checksum ^= mem[i];

    return checksum;
}

/// WTP packet handlers
wtp_pkt_handler_t wtp_pkt_handlers[_WTP_PKT_MAX] = {
    NULL, //WTP_PKT_END
    wtp_handle_open, //WTP_PKT_OPEN
    NULL, //WTP_PKT_CLOSE
    wtp_handle_ack, //WTP_PKT_ACK
    wtp_handle_begin_msg, //WTP_PKT_BEGIN_MSG
    wtp_handle_cont_msg, //WTP_PKT_CONT_MSG
    NULL, //WTP_PKT_REQ_UPLINK
    wtp_handle_set_param, //WTP_PKT_SET_PARAM
};
