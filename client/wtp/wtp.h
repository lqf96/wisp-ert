#pragma once

#include <stdbool.h>
#include "wio.h"

//Maximum event type
#define _WTP_EVENT_MAX 0x03
//Size of receive callback
#define _WTP_RECV_CB_MAX 0x05

//WTP status type
typedef wio_status_t wtp_status_t;
//WTP packet type
typedef uint8_t wtp_pkt_t;
//WTP event type
typedef uint8_t wtp_event_t;
//WTP link state type
typedef uint8_t wtp_link_state_t;

//WTP callback linked list item type
typedef struct wtp_cb_item {
    //Callback closure data
    void* _cb_data;
    //Callback function
    wio_callback_t _cb;

    //Previous item
    struct wtp_cb_item* _prev;
    //Next item
    struct wtp_cb_item* _next;
} wtp_cb_item_t;

//WTP client type
typedef struct wtp {
    //Downlink state
    wtp_link_state_t _downlink_state;
    //Uplink state
    wtp_link_state_t _uplink_state;

    //Reliable downlink
    bool _downlink_reliable;
    //Reliable uplink
    bool _uplink_reliable;

    //Sliding window size
    uint16_t _sliding_window;

    //EPC buffer
    wio_buf_t _epc_buf;
    //Write memory buffer
    wio_buf_t _write_mem_buf;

    //Send packet buffer
    wio_buf_t _send_pkt_buf;
    //Send data buffer
    wio_buf_t _send_data_buf;
    //Send data sequence number
    uint16_t _send_seq;

    //Pointer to send packet size
    uint8_t* _send_pkt_size;
    //Send packet begin position
    uint16_t _send_pkt_begin;
    //Send message begin position
    uint16_t _send_msg_begin;
    //Send message length
    uint16_t _send_msg_size;
    //Send flag
    bool _send_flag;

    //Receive buffer
    wio_buf_t _recv_buf;
    //Receive data sequence number
    uint16_t _recv_seq;
    //Receive packet begin position
    uint16_t _recv_pkt_begin;

    //Received message buffer
    wio_buf_t _recv_msg_buf;
    //Received message begin position
    uint16_t _recv_msg_begin;
    //Received message length
    uint16_t _recv_msg_size;

    //Receive message callbacks
    wio_callback_t _recv_cb[_WTP_RECV_CB_MAX];
    //Closure data
    void* _recv_cb_data[_WTP_RECV_CB_MAX];
    //Begin of receive message callbacks
    size_t _recv_cb_begin;
    //End of receive message callbacks
    size_t _recv_cb_end;

    //Checksum function
    uint8_t (*_checksum_func)(uint8_t*, uint16_t, uint16_t);

    //Timer instance
    wio_timer_t _timer;

    //Event callback
    wio_callback_t _event_cb[_WTP_EVENT_MAX];
    //Callback closure data
    void* _event_cb_data[_WTP_EVENT_MAX];
} wtp_t;

//Not acknowledged
const wtp_status_t WTP_ERR_NOT_ACKED = 0x10;
//Required action already done
const wtp_status_t WTP_ERR_ALREADY = 0x11;
//Connection is busy
const wtp_status_t WTP_ERR_BUSY = 0x12;
//Invalid parameter
const wtp_status_t WTP_ERR_INVALID_PARAM = 0x13;
//Invalid checksum
const wtp_status_t WTP_ERR_INVALID_CHECKSUM = 0x14;
//Unsupported operation
const wtp_status_t WTP_ERR_UNSUPPORT_OP = 0x15;

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
const wtp_pkt_t WTP_PKT_MAX = 0x08;

//Connection opened
const wtp_event_t WTP_EVENT_OPEN = 0x00;
//Downlink closed
const wtp_event_t WTP_EVENT_HALF_CLOSE = 0x01;
//Connection closed
const wtp_event_t WTP_EVENT_CLOSE = 0x02;

//Maximum event type
const wtp_event_t WTP_EVENT_MAX = 0x03;

//Closed
const wtp_link_state_t WTP_STATE_CLOSED = 0x00;
//Opening
const wtp_link_state_t WTP_STATE_OPENING = 0x01;
//Opened
const wtp_link_state_t WTP_STATE_OPENED = 0x02;
//Closing
const wtp_link_state_t WTP_STATE_CLOSING = 0x03;

const size_t WIO_RECV_CB_MAX = _WTP_RECV_CB_MAX;

/**
 * WTP type constructor
 */
extern wtp_status_t wtp_init(
    wtp_t* self,
    uint8_t* epc_mem,
    size_t epc_buf_size,
    uint8_t* write_mem,
    size_t write_buf_size,
    size_t send_ctrl_buf_size,
    size_t send_data_buf_size,
    size_t recv_msg_buf_size,
    uint8_t (*checksum_func)(uint8_t*, uint16_t, uint16_t)
);

/**
 * Connect to WTP server
 */
extern wtp_status_t wtp_connect(
    wtp_t* self,
    bool uplink_reliable
);

/**
 * Disconnect from WTP server
 */
extern wtp_status_t wtp_close(
    wtp_t* self
);

/**
 * Send message to WTP server.
 *
 * @param self WTP endpoint instance
 * @param data Data to send
 * @param size Size of data
 * @param cb_data Callback closure data
 * @param cb Callback function
 */
extern wtp_status_t wtp_send(
    wtp_t* self,
    uint8_t* data,
    size_t size,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Receive message from WTP server.
 *
 * @param self WTP endpoint instance
 * @param cb_data Callback closure
 * @param cb Callback function
 */
extern wtp_status_t wtp_recv(
    wtp_t* self,
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
);
