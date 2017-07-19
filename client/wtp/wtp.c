#include "wtp.h"

//WTP packet handler type
typedef wio_status_t (*wtp_pkt_handler_t)(wtp_t*);

/**
 * Invoke event callback.
 *
 * @param self WTP endpoint instance
 * @param event WTP event
 * @param status Operation status
 * @param result Result
 */
static wtp_event_t wtp_invoke_event_callback(
    wtp_t* self,
    wtp_event_t event,
    wtp_status_t status,
    void* value
) {
    wio_callback_t cb = self->_event_cb[event];
    void* cb_data = self->_event_cb_data[event];

    if (cb)
        return cb(cb_data, status, result);
    else
        return WIO_OK;
}

/**
 * Handle WTP packet data.
 *
 * @param self WTP endpoint instance
 * @return Operation status
 */
static wtp_status_t wtp_handle_packet_data(
    wtp_t* self
) {

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
    wtp_mode_t mode;

    //Read transport mode
    WIO_TRY(wio_read(buf, &mode, 1))
    //Verify checksum
    WIO_TRY(wtp_verify_checksum(self))

    //Downlink transport mode
    if (mode>WTP_MODE_MAX)
        return WTP_ERR_INVALID_PARAM;

    //Open downlink
    self->_downlink_open = true;
    //Set downlink mode
    self->_downlink_mode = mode;

    //Send acknowledgement
    WIO_TRY(wtp_send_ack(self))
    //Invoke and remove callback
    WIO_TRY(wtp_invoke_event_callback(self, WTP_EVENT_OPEN, WIO_OK, NULL))

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
        if (self->_uplink_open)
            WIO_TRY(wtp_invoke_event_callback(self, WTP_EVENT_DOWNLINK_CLOSE, WIO_OK, NULL))
        //Fully closed
        else
            WIO_TRY(wtp_invoke_event_callback(self, WTP_EVENT_CLOSE, WIO_OK, NULL))
    }

    //Send acknowledgement
    WIO_TRY(wtp_send_ack(self))

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
    WIO_TRY(wtp_verify_chksum(self))

    //Open uplink acknowledgement
    if (self->_uplink_state==WTP_STATE_OPENING)
        self->_uplink_state = WTP_STATE_OPENED;
    //Close uplink acknowledgement
    else if (self->_uplink_state==WTP_STATE_CLOSING)
        self->_uplink_state = WTP_STATE_CLOSED;

    //TODO: Sliding window-based sending

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
    wio_buf_t* buf = &self->_recv_buf;
    uint16_t msg_size;

    //Message length
    WIO_TRY(wio_read(buf, &msg_size, 2))
    //Handle incoming packet
    WIO_TRY(wtp_handle_data_packet(self, ))



    //Send acknowledgement
    WIO_TRY(wtp_send_ack(self))

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
    wio_buf_t* buf = &self->_recv_buf;

    //Handle packet data
    WIO_TRY(wtp_handle_packet_data(buf))

    //Send acknowledgement
    WIO_TRY(wtp_send_ack(self))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
extern wtp_status_t wtp_init(
   wtp_t* self,
   uint8_t* send_buf,
   size_t send_buf_size,
   uint8_t* recv_buf,
   size_t recv_buf_size
) {
    //Downlink and uplink status
    self->_downlink_state = WTP_STATE_CLOSED;
    self->_uplink_state = WTP_STATE_CLOSED;

    //Send control buffer
    wio_buf_init(&self->_send_ctrl_buf, NULL, 0);
    //Send data buffer
    wio_buf_init(&self->_send_data_buf, NULL, 0);
    //Send message begin position and size
    self->_send_msg_begin = 0;
    self->_send_msg_size = 0;

    //Receive buffer
    wio_buf_init(&self->_recv_buf, NULL, 0);
    //Receive message begin position
    self->_recv_msg_begin = 0;
    self->_recv_msg_size = 0;

    //TODO: Sliding window (Temporarily set to 128)
    self->_sliding_window = 128;

    //Timer
    wio_timer_init(&self->_timer);

    //Event callbacks
    for (size_t i=0;i<=WTP_EVENT_MAX;i++) {
        self->_event_cb[i] = NULL;
        self->_event_cb_data[i] = NULL;
    }
}

/**
 * {@inheritDoc}
 */
extern wtp_status_t wtp_connect(
    wtp_t* self,
    wtp_mode_t mode
) {
    wio_buf_t* buf = &self->_send_ctrl_buf;

    //Uplink already opened
    if (self->_uplink_state==WTP_STATE_OPENED)
        return WTP_ERR_ALREADY;
    //Uplink transport mode
    if (mode>WTP_MODE_MAX)
        return WTP_ERR_INVALID_PARAM;

    WIO_TRY(wtp_begin_packet(self, WTP_PKT_OPEN))
    //Construct open packet
    WIO_TRY(wio_write(buf, &mode, 1))
    //Send open packet
    WIO_TRY(wtp_send_packet(self))

    //Update uplink state
    self->_uplink_state = WTP_STATE_OPENING;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
extern wtp_status_t wtp_close(
    wtp_t* self
) {
    //Connection closed
    if ((self->_uplink_mode==WTP_STATE_CLOSED)&&(self->_downlink_mode==WTP_STATE_CLOSED))
        return WTP_ERR_ALREADY;

    WIO_TRY(wtp_begin_packet(self, WTP_PKT_CLOSE))
    //Send close packet
    WIO_TRY(wtp_send_packet(self))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
extern wtp_status_t wtp_send(
    wtp_t* self,
    void* data,
    size_t size
) {
    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
extern wtp_status_t wtp_recv(
    wtp_t* self,
    size_t size
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
    wio_chksum_stream_t stream_inst;
    wio_chksum_stream_t* stream = &stream_inst;

    //Initialize checksum stream
    WIO_TRY(wio_chksum_stream_init(stream, data, size));
    //Read packets
    while (true) {
        wtp_pkt_t packet_type;

        //Read packet type
        WIO_TRY(wio_reset_checksum(stream))
        WIO_TRY(wio_read(stream, &packet_type, 1))

        //No more packets
        if (packet_type==WTP_PKT_END)
            break;
        //Unsupported operation
        if (packet_type>WTP_PKT_MAX)
            return WTP_ERR_UNSUPPORT_OP;

        //Handle packet with respective handler
        wtp_pkt_handler_t handler = wtp_pkt_handlers[packet_type];
        if (!handler)
            return WTP_ERR_UNSUPPORT_OP;
        WIO_TRY(handler(self, stream))
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
    if (event>WTP_EVENT_MAX)
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
