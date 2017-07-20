#pragma once

#include <stdbool.h>
#include "wio.h"

//WTP transport mode type
typedef uint8_t wtp_mode_t;
//WTP packet type
typedef uint8_t wtp_pkt_t;
//WTP status type
typedef wio_status_t wtp_status_t;
//WTP event type
typedef uint8_t wtp_event_t;
//WTP link state type
typedef uint8_t wtp_link_state_t;

//WTP client type
typedef struct wtp {
    //Downlink state
    wtp_link_state_t _downlink_state;
    //Uplink state
    wtp_link_state_t _uplink_state;

    //Downlink mode
    wtp_mode_t _downlink_mode;
    //Uplink mode
    wtp_mode_t _uplink_mode;

    //Sliding window size
    uint16_t _sliding_window;

    //Send control buffer
    wio_buf_t _send_ctrl_buf;
    //Send data buffer
    wio_buf_t _send_data_buf;
    //Send data sequence number
    uint16_t _send_seq;

    //Send message begin position
    uint16_t _send_msg_begin;
    //Send message length
    uint16_t _send_msg_size;

    //Receive buffer
    wio_buf_t _recv_buf;
    //Receive data sequence number
    uint16_t _recv_seq;
    //Checksum function
    uint8_t (*_checksum_func)(uint8_t*, size_t, size_t);

    //Received message begin position
    uint16_t _recv_msg_begin;
    //Received message length
    uint16_t _recv_msg_size;

    //Timer instance
    wio_timer_t _timer;

    //Event callbacks
    wio_callback_t _event_cb[WTP_EVENT_MAX];
    //Callback closure data
    void* _event_cb_data[WTP_EVENT_MAX];
} wtp_t;

//Required action already done
const wtp_status_t WTP_ERR_ALREADY = 0x10;
//Connection is busy
const wtp_status_t WTP_ERR_BUSY = 0x11;
//Invalid parameter
const wtp_status_t WTP_ERR_INVALID_PARAM = 0x12;

//Unreliable mode
const wtp_mode_t WTP_MODE_UNRELIABLE = 0x00;
//Reliable stream mode
const wtp_mode_t WTP_MODE_STREAM = 0x01;
//Reliable datagram mode
const wtp_mode_t WTP_MODE_DGRAM = 0x02;

//Maximum mode number
const wtp_mode_t WTP_MODE_MAX = WTP_MODE_DGRAM;

//No more packets
const wtp_pkt_t WTP_PKT_END = 0x00;
//Open WTP connection
const wtp_pkt_t WTP_PKT_OPEN = 0x01;
//Close WTP connection
const wtp_pkt_t WTP_PKT_CLOSE = 0x02;
//Acknowledgement
const wtp_pkt_t WTP_PKT_ACK = 0x03;
//Begin message
const wtp_pkt_t WTP_PKT_BEGIN_MSG = 0x04;
//Continue message
const wtp_pkt_t WTP_PKT_CONT_MSG = 0x05;
//Request uplink transfer
const wtp_pkt_t WTP_PKT_REQ_UPLINK = 0x06;
//Protocol error
const wtp_pkt_t WTP_PKT_ERR = 0x07;

//Maximum packet type number
const wtp_pkt_t WTP_PKT_MAX = WTP_PKT_ERR;

//Connection opened
const wtp_event_t WTP_EVENT_OPEN = 0x00;
//Downlink closed
const wtp_event_t WTP_EVENT_DOWNLINK_CLOSE = 0x01;
//Connection closed
const wtp_event_t WTP_EVENT_CLOSE = 0x02;
//Data sent
const wtp_event_t WTP_EVENT_SENT = 0x03;
//Data received
const wtp_event_t WTP_EVENT_RECEIVED = 0x04;

//Maximum event number
const wtp_event_t WTP_EVENT_MAX = WTP_EVENT_RECEIVED;

//Closed
const wtp_link_state_t WTP_STATE_CLOSED = 0x00;
//Opening
const wtp_link_state_t WTP_STATE_OPENING = 0x01;
//Opened
const wtp_link_state_t WTP_STATE_OPENED = 0x02;
//Closing
const wtp_link_state_t WTP_STATE_CLOSING = 0x03;

/**
 * WTP type constructor
 */
extern wtp_status_t wtp_init(
    wtp_t* self,
    wio_buf_t* send_buf,
    wio_buf_t* recv_buf
);

/**
 * Connect to WTP server
 */
extern wtp_status_t wtp_connect(
    wtp_t* self,
    wtp_mode_t mode,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Disconnect from WTP server
 */
extern wtp_status_t wtp_close(
    wtp_t* self,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Send data to WTP server
 */
extern wtp_status_t wtp_send(
    wtp_t* self,
    uint8_t* data,
    size_t size,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Receive data from WTP server
 */
extern wtp_status_t wtp_recv(
    wtp_t* self,
    size_t size,
    void* cb_data,
    wio_callback_t cb
);

/**
 * READ operation hook
 *
 * @param self WTP endpoint instance
 */
extern wtp_status_t wtp_read_hook(
    wtp_t* self,
    void* data,
    size_t size
);

/**
 * BLOCKWRITE operation hook
 *
 * @param self WTP endpoint instance
 * @param data BLOCKWRITE data
 * @param size BLOCKWRITE data size
 * @return Operation status
 */
extern wtp_status_t wtp_blockwrite_hook(
    wtp_t* self,
    void* data,
    size_t size
);

/**
 * Add WTP event handler.
 *
 * @param self WTP endpoint instance
 * @param event WTP event
 * @param cb_data Callback closure data
 * @param cb Callback function
 * @return Operation status
 */
wtp_status_t wtp_on_event(
    wtp_t* self,
    wtp_event_t event,
    void* cb_data,
    wio_callback_t cb
)
