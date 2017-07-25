#include <stdlib.h>
#include "wtp.h"

//WTP packet handler type
typedef wio_status_t (*wtp_pkt_handler_t)(wtp_t*);

//WTP packet handlers
wtp_pkt_handler_t wtp_pkt_handlers[];

/**
 * Trigger event.
 *
 * @param self WTP endpoint instance
 * @param event WTP event
 * @param status Operation result
 * @param result Result
 * @returns Operation result of event handler
 */
static wtp_event_t wtp_trigger_event(
    wtp_t* self,
    wtp_event_t event,
    wtp_status_t status,
    void* result
) {
    wio_callback_t cb = self->_event_cb[event];
    void* cb_data = self->_event_cb_data[event];

    if (cb)
        return cb(cb_data, status, result);
    else
        return WIO_OK;
}

/**
 * Verify checksum of received data.
 *
 * @param self WTP endpoint instance
 * @returns WIO_OK if verifed, otherwise WTP_ERR_INVALID_CHECKSUM
 */
static wtp_status_t wtp_verify_checksum(
    wtp_t* self
) {
    wio_buf_t* buf = &self->_recv_buf;
    uint8_t read_checksum, calc_checksum;

    //Calculate checksum
    calc_checksum = self->_checksum_func(buf->buffer, self->_recv_pkt_begin, buf->pos_a);
    //Read checksum from stream
    WIO_TRY(wio_read(buf, &read_checksum, 1))
    //Compare checksum
    if (read_checksum!=calc_checksum)
        return WTP_ERR_INVALID_CHECKSUM;

    return WIO_OK;
}

/**
 * Begin constructing a new WTP packet.
 *
 * @param self WTP endpoint instance
 * @param packet_type Packet type
 */
static wtp_status_t wtp_begin_packet(
    wtp_t* self,
    wtp_pkt_t packet_type
) {
    wio_buf_t* pkt_buf = &self->_send_pkt_buf;

    //Reserve space for send packet size
    WIO_TRY(wio_alloc(pkt_buf, 1, &self->_send_pkt_size))
    //Begin position
    self->_send_pkt_begin = pkt_buf->pos_b;
    //Write packet type
    WIO_TRY(wio_write(pkt_buf, &packet_type, 1))

    return WIO_OK;
}

/**
 * End a WTP packet and request sending the packet.
 *
 * @param self WTP endpoint instance
 */
static wtp_status_t wtp_end_packet(
    wtp_t* self
) {
    //Packet size
    uint8_t pkt_size = (uint8_t)(self->_send_pkt_buf.pos_b-self->_send_pkt_begin);
    //Write packet size
    *(self->_send_pkt_size) = pkt_size;

    return WIO_OK;
}

/**
 * Handle WTP open packet.
 *
 * @param self WTP endpoint instance
 * @return Operation status
 */
static wtp_status_t wtp_handle_open(
    wtp_t* self
) {
    wio_buf_t* buf = &self->_recv_buf;
    uint8_t downlink_reliable;

    //Read downlink mode
    WIO_TRY(wio_read(buf, &downlink_reliable, 1))
    //Verify checksum
    WIO_TRY(wtp_verify_checksum(self))

    //Open downlink
    self->_downlink_state = WTP_STATE_OPENED;
    //Set downlink reliability
    self->_downlink_reliable = (bool)downlink_reliable;

    //Send acknowledgement
    WIO_TRY(wtp_begin_packet(self, WTP_PKT_ACK))
    WIO_TRY(wtp_end_packet(self))
    //Invoke and remove callback
    WIO_TRY(wtp_trigger_event(self, WTP_EVENT_OPEN, WIO_OK, NULL))

    return WIO_OK;
}

/**
 * Handle WTP close packet.
 *
 * @param self WTP endpoint instance
 * @return Operation status
 */
static wtp_status_t wtp_handle_close(
    wtp_t* self
) {
    //Verify checksum
    WIO_TRY(wtp_verify_checksum(self))

    if (self->_downlink_state==WTP_STATE_OPENED) {
        //Update downlink state
        self->_downlink_state = WTP_STATE_CLOSED;

        //Uplink still open
        if (self->_uplink_state==WTP_STATE_OPENED)
            WIO_TRY(wtp_trigger_event(self, WTP_EVENT_HALF_CLOSE, WIO_OK, NULL))
        //Fully closed
        else
            WIO_TRY(wtp_trigger_event(self, WTP_EVENT_CLOSE, WIO_OK, NULL))
    }

    //Send acknowledgement
    WIO_TRY(wtp_begin_packet(self, WTP_PKT_ACK))
    WIO_TRY(wtp_end_packet(self))

    return WIO_OK;
}

/**
 * Handle WTP acknowledgement packet.
 *
 * @param self WTP endpoint instance
 * @return Operation status
 */
static wtp_status_t wtp_handle_ack(
    wtp_t* self
) {
    wio_buf_t* buf = &self->_recv_buf;
    uint16_t recv_seq;

    //Receive data sequence number
    WIO_TRY(wio_read(buf, &recv_seq, 2))
    //Verify checksum
    WIO_TRY(wtp_verify_checksum(self))

    //Connected acknowledgement
    if (self->_uplink_state==WTP_STATE_OPENING)
        self->_uplink_state = WTP_STATE_OPENED;
    //Connection closed acknowledgement
    else if (self->_uplink_state==WTP_STATE_CLOSING)
        self->_uplink_state = WTP_STATE_CLOSED;

    //TODO: Receive send packet ack

    return WIO_OK;
}

/**
 * Handle WTP message data packet.
 *
 * @param self WTP endpoint instance
 * @param begin_msg Begin of message flag
 * @return Operation status
 */
static wtp_status_t wtp_handle_msg_data(
    wtp_t* self,
    bool begin_msg
) {
    wio_buf_t* recv_buf = &self->_recv_buf;
    wio_buf_t* msg_buf = &self->_recv_msg_buf;

    uint8_t payload_size;
    uint16_t msg_size, offset;
    uint16_t msg_end = self->_recv_msg_begin+self->_recv_msg_size;

    //Read message size
    if (begin_msg)
        WIO_TRY(wio_read(recv_buf, &msg_size, 2))
    else
        msg_size = self->_recv_msg_size;
    //Read offset and payload size
    WIO_TRY(wio_read(recv_buf, &offset, 2))
    WIO_TRY(wio_read(recv_buf, &payload_size, 1))
    //Validate checksum
    WIO_TRY(wtp_verify_checksum(self))

    //Check if packet begins at acknowledged position
    if (offset!=self->_recv_seq)
        return WTP_ERR_NOT_ACKED;

    //For begin of the message
    if (begin_msg) {
        //Check if new message begins at the ent of old message
        if (offset!=msg_end)
            return WTP_ERR_NOT_ACKED;
        //Check if payload size is greater than message size
        if (payload_size>msg_size)
            return WTP_ERR_NOT_ACKED;

        //Update message begin and message size
        self->_recv_msg_begin = offset;
        self->_recv_msg_size = msg_size;
        //Update message end
        msg_end = offset+msg_size;
        //Reset received message buffer
        WIO_TRY(wio_reset(msg_buf))
    } else {
        //Check if end of payload exceeds message data range
        if (offset+payload_size>msg_end)
            return WTP_ERR_NOT_ACKED;
    }

    //Update sequence number
    self->_recv_seq += payload_size;
    //Append data to buffer
    WIO_TRY(wio_copy(recv_buf, msg_buf, payload_size))

    //End of message
    if (offset+payload_size==msg_end) {
        size_t next_cb = self->_recv_cb_begin;
        wio_callback_t cb = self->_recv_cb[next_cb];
        void* cb_data = self->_recv_cb_data[next_cb];

        //Invoke callback
        if (cb)
            cb(cb_data, WIO_OK, &self->_recv_msg_buf);
        //Update queue begin
        self->_recv_cb_begin++;
        if (self->_recv_cb_begin==WIO_RECV_CB_MAX)
            self->_recv_cb_begin = 0;
    }

    //Send acknowledgement
    WIO_TRY(wtp_end_packet(self))

    return WIO_OK;
}

/**
 * Handle WTP begin message packet.
 *
 * @param self WTP endpoint instance
 * @return Operation status
 */
static wtp_status_t wtp_handle_begin_msg(
    wtp_t* self
) {
    WIO_TRY(wtp_handle_msg_data(self, true))

    return WIO_OK;
}

/**
 * Handle WTP continue message packet.
 *
 * @param self WTP endpoint instance
 * @return Operation status
 */
static wtp_status_t wtp_handle_cont_msg(
    wtp_t* self
) {
    WIO_TRY(wtp_handle_msg_data(self, false))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_init(
   wtp_t* self,
   uint8_t* epc_mem,
   size_t epc_buf_size,
   uint8_t* write_mem,
   size_t write_buf_size,
   size_t send_ctrl_buf_size,
   size_t send_data_buf_size,
   size_t recv_msg_buf_size,
   uint8_t (*checksum_func)(uint8_t*, uint16_t, uint16_t)
) {
    uint8_t* send_pkt_mem;
    uint8_t* send_data_mem;
    uint8_t* recv_msg_mem;

    //Downlink and uplink state
    self->_downlink_state = WTP_STATE_CLOSED;
    self->_uplink_state = WTP_STATE_CLOSED;
    //Sliding window (Temporarily set to 128)
    self->_sliding_window = 128;

    //EPC buffer
    WIO_TRY(wio_buf_init(&self->_epc_buf, epc_mem, epc_buf_size))
    //Write memory buffer
    WIO_TRY(wio_buf_init(&self->_write_mem_buf, write_mem, write_buf_size))

    //Send packets buffer
    send_pkt_mem = malloc(send_ctrl_buf_size);
    if (!send_pkt_mem)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_send_pkt_buf, send_pkt_mem, send_ctrl_buf_size))
    //Send data buffer
    send_data_mem = malloc(recv_msg_buf_size);
    if (!send_data_mem)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_send_data_buf, send_data_mem, send_data_buf_size))
    //Send data sequence number
    self->_send_seq = 0;
    //Send message begin position and size
    self->_send_msg_begin = 0;
    self->_send_msg_size = 0;
    //Send flag
    self->_send_flag = false;

    //Receive message buffer
    recv_msg_mem = malloc(recv_msg_buf_size);
    if (!recv_msg_mem)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_recv_msg_buf, recv_msg_mem, recv_msg_buf_size))
    //Receive sequence number
    self->_recv_seq = 0;
    //Receive message begin position
    self->_recv_msg_begin = 0;
    self->_recv_msg_size = 0;

    //Begin and end of receive message callbacks
    self->_recv_cb_begin = 0;
    self->_recv_cb_end = 0;

    //Timer
    WIO_TRY(wio_timer_init(&self->_timer))

    //Checksum function
    self->_checksum_func = checksum_func;

    //Event callbacks
    for (size_t i=0;i<=WTP_EVENT_MAX;i++) {
        self->_event_cb[i] = NULL;
        self->_event_cb_data[i] = NULL;
    }

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_connect(
    wtp_t* self,
    bool reliable_uplink
) {
    wio_buf_t* buf = &self->_send_pkt_buf;
    uint8_t _reliable_uplink = reliable_uplink;

    //Uplink already opened
    if (self->_uplink_state==WTP_STATE_OPENED)
        return WTP_ERR_ALREADY;

    WIO_TRY(wtp_begin_packet(self, WTP_PKT_OPEN))
    //Construct open packet
    WIO_TRY(wio_write(buf, &_reliable_uplink, 1))
    //Send open packet
    WIO_TRY(wtp_end_packet(self))

    //Update uplink state
    self->_uplink_state = WTP_STATE_OPENING;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_close(
    wtp_t* self
) {
    //Connection closed
    if ((self->_uplink_state==WTP_STATE_CLOSED)&&(self->_downlink_state==WTP_STATE_CLOSED))
        return WTP_ERR_ALREADY;

    WIO_TRY(wtp_begin_packet(self, WTP_PKT_CLOSE))
    //Send close packet
    WIO_TRY(wtp_end_packet(self))

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
    wio_buf_t* data_buf = &self->_send_data_buf;

    //Write data to send data buffer
    WIO_TRY(wio_write(data_buf, data, size))
    //TODO: Send callback

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
    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_read_hook(
    wtp_t* self,
    void* data,
    size_t size
) {
    wio_status_t status = WIO_OK;
    wio_buf_t* pkt_buf = &self->_send_pkt_buf;
    wio_buf_t* data_buf = &self->_send_data_buf;
    wio_buf_t* epc_buf = &self->_epc_buf;
    wio_buf_t* write_mem_buf = &self->_write_mem_buf;
    uint8_t pkt_size;

    //Clear EPC and write memory buffer
    WIO_TRY(wio_reset(epc_buf))
    WIO_TRY(wio_reset(write_mem_buf))
    //Copy packets from packets buffer to EPC buffer
    while (status==WIO_OK) {
        WIO_TRY(wio_read(pkt_buf, &pkt_size, 1))
        WIO_TRY(wio_copy(pkt_buf, epc_buf, pkt_size))

        //No more packets to send
        if (pkt_buf->pos_a>=pkt_buf->pos_b)
            break;
    }
    //TODO: Copy data to Read buffer

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wtp_status_t wtp_blockwrite_hook(
    wtp_t* self,
    void* data,
    size_t size
) {
    wio_buf_t* buf = &self->_recv_buf;

    //Initialize stream
    WIO_TRY(wio_buf_init(buf, data, size))
    //Read packets
    while (true) {
        wtp_pkt_t packet_type;

        //Packet begin position
        self->_recv_pkt_begin = buf->pos_a;
        //Read packet type
        WIO_TRY(wio_read(buf, &packet_type, 1))

        //No more packets
        if (packet_type==WTP_PKT_END)
            break;
        //Unsupported operation
        if (packet_type>=WTP_PKT_MAX)
            return WTP_ERR_UNSUPPORT_OP;

        //Handle packet with respective handler
        wtp_pkt_handler_t handler = wtp_pkt_handlers[packet_type];
        if (!handler)
            return WTP_ERR_UNSUPPORT_OP;
        WIO_TRY(handler(self))
    }

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
        return WTP_ERR_INVALID_PARAM;

    //Set callback and closure data
    self->_event_cb[event] = cb;
    self->_event_cb_data[event] = cb_data;

    return WIO_OK;
}

//WTP packet handlers
wtp_pkt_handler_t wtp_pkt_handlers[] = {
    NULL, //WTP_PKT_END
    wtp_handle_open, //WTP_PKT_OPEN
    wtp_handle_close, //WTP_PKT_CLOSE
    wtp_handle_ack, //WTP_PKT_ACK
    wtp_handle_begin_msg, //WTP_PKT_BEGIN_MSG
    wtp_handle_cont_msg, //WTP_PKT_CONT_MSG
    NULL, //WTP_PKT_REQ_UPLINK
    NULL //WTP_PKT_ERR
};
