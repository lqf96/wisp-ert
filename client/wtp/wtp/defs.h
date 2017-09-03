#pragma once

#include <wio.h>

//=== WTP types ===
//WTP endpoint type forward declaration
struct wtp;

//WTP status type
typedef wio_status_t wtp_status_t;
//WTP packet type
typedef uint8_t wtp_pkt_t;
//WTP link state type
typedef uint8_t wtp_link_state_t;
//WTP event type
typedef uint8_t wtp_event_t;
//WTP parameter code type
typedef uint8_t wtp_param_t;
//WTP packet handler type
typedef wtp_status_t (*wtp_pkt_handler_t)(
    struct wtp*,
    wio_buf_t*
);

//=== WTP error codes ===
//Unsupported operation
static const wtp_status_t WTP_ERR_UNSUPPORT_OP = 0x10;

//=== WTP packet types ===
//No more packets
static const wtp_pkt_t WTP_PKT_END = 0x00;
//Open WTP connection
static const wtp_pkt_t WTP_PKT_OPEN = 0x01;
//Close WTP connection
static const wtp_pkt_t WTP_PKT_CLOSE = 0x02;
//Acknowledgement
static const wtp_pkt_t WTP_PKT_ACK = 0x03;
//Begin message
static const wtp_pkt_t WTP_PKT_BEGIN_MSG = 0x04;
//Continue message
static const wtp_pkt_t WTP_PKT_CONT_MSG = 0x05;
//Request uplink transfer
static const wtp_pkt_t WTP_PKT_REQ_UPLINK = 0x06;
//Set WTP parameter
static const wtp_pkt_t WTP_PKT_SET_PARAM = 0x07;

//Packet max
#define _WTP_PKT_MAX 0x08
static const wtp_pkt_t WTP_PKT_MAX = _WTP_PKT_MAX;

//=== WTP connection states ===
//Closed
static const wtp_link_state_t WTP_STATE_CLOSED = 0x00;
//Opening
static const wtp_link_state_t WTP_STATE_OPENING = 0x01;
//Opened
static const wtp_link_state_t WTP_STATE_OPENED = 0x02;
//Closing
static const wtp_link_state_t WTP_STATE_CLOSING = 0x03;

//=== WTP events ===
//Connection opened
static const wtp_event_t WTP_EVENT_OPEN = 0x00;
//Downlink closed
static const wtp_event_t WTP_EVENT_HALF_CLOSE = 0x01;
//Connection closed
static const wtp_event_t WTP_EVENT_CLOSE = 0x02;

//Event max
#define _WTP_EVENT_MAX 0x03
static const wtp_event_t WTP_EVENT_MAX = _WTP_EVENT_MAX;

//=== WTP parameter code ===
//Sliding window size
static const wtp_param_t WTP_PARAM_WINDOW_SIZE = 0x00;
//Read size
static const wtp_param_t WTP_PARAM_READ_SIZE = 0x01;
