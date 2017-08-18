#pragma once

#include <stdbool.h>
#include <wio.h>
#include "defs.h"

//WTP sliding window-based transmit control type
typedef struct wtp_tx_ctrl {
    //Sequence number
    uint16_t _seq_num;
    //Sliding window size
    uint16_t _window_size;
    //Timeout
    uint16_t _timeout;

    //Send packets buffer
    wio_buf_t _pkt_buf;
    //Send messages buffer
    wio_buf_t _msg_buf;

    //Current packet begin position
    uint16_t _pkt_begin;
    //Pointer to current packet size
    uint8_t* _pkt_size;

    //TODO: More things
} wtp_tx_ctrl_t;

//WTP data fragment type
typedef struct wtp_rx_fragment {
    //Sequence number
    uint16_t _seq_num;
    //Data
    uint8_t* _data;
    //Size
    uint8_t _size;

    //Assembled flag
    bool _assembled;
    //Next fragment
    struct wtp_rx_fragment* _next;
} wtp_rx_fragment_t;

//WTP received message information type
typedef struct wtp_rx_msg_info {
    //Message begin
    uint16_t _begin;
    //Message size
    uint16_t _size;

    //In use flag
    bool _in_use;
    //Index of next message
    uint8_t _next;
} wtp_rx_msg_info_t;

//WTP sliding window-based receive control type
typedef struct wtp_rx_ctrl {
    //Sequence number
    uint16_t _seq_num;
    //Sliding window size
    uint16_t _window_size;

    //Acknowledged message data buffer
    wio_buf_t _msg_data_buf;

    //Data fragments buffer
    wio_buf_t _fragments_buf;
    //Begin of data fragments linked list
    wtp_rx_fragment_t* _fragments_begin;

    //Message information store
    wtp_rx_msg_info_t* _msg_info_store;
    //Store size
    uint8_t _msg_info_size;
    //Linked list begin index
    uint8_t _msg_info_begin;
} wtp_rx_ctrl_t;

/**
 * Initialize WTP transmit control type.
 *
 * @param self WTP transmit control instance
 * @param window_size Sliding window size
 * @param timeout Packet sending and requesting uplink timeout
 * @param pkt_buf_size Packet buffer size
 * @param msg_buf_size Message buffer size
 */
extern wtp_status_t wtp_tx_init(
    wtp_tx_ctrl_t* self,
    uint16_t window_size,
    uint16_t timeout,
    uint16_t pkt_buf_size,
    uint16_t msg_buf_size
);

/**
 * Finalize WTP transmit control type.
 *
 * @param self WTP transmit control instance
 */
extern wtp_status_t wtp_tx_fini(
    wtp_tx_ctrl_t* self
);

/**
 * Begin construction of WTP packet.
 *
 * @param self WTP transmit control instance
 * @param pkt_type Packet type
 */
extern wtp_status_t wtp_begin_pkt(
    wtp_tx_ctrl_t* self,
    wtp_pkt_t pkt_type
);

/**
 * End construction of WTP packet.
 *
 * @param self WTP transmit control instance
 */
extern wtp_status_t wtp_end_pkt(
    wtp_tx_ctrl_t* self
);

/**
 * Initialize WTP receive control type.
 *
 * @param self WTP transmit control instance
 * @param window_size Sliding window size
 * @param msg_data_size Message data buffer size
 * @param fragments_size Data fragments buffer size
 * @param n_msg_info Maximum number of receiving messages information
 */
extern wtp_status_t wtp_rx_init(
    wtp_rx_ctrl_t* self,
    uint16_t window_size,
    uint16_t msg_data_size,
    uint16_t fragments_size,
    uint8_t n_msg_info
);

/**
 * Finalize WTP receive control type.
 *
 * @param self WTP transmit control instance
 */
extern wtp_status_t wtp_rx_fini(
    wtp_rx_ctrl_t* self
);

/**
 * Handle incoming data packet.
 *
 * @param self WTP receive control instance
 * @param seq_num Packet sequence number
 * @param data Payload data
 * @param size Payload size
 * @param new_msg_size New message size for WTP_PKT_BEGIN_MSG, 0 for WTP_PKT_CONT_MSG
 * @param n_msgs Number of messages received
 */
extern wtp_status_t wtp_rx_handle_packet(
    wtp_rx_ctrl_t* self,
    uint16_t seq_num,
    uint8_t* data,
    uint16_t size,
    uint16_t new_msg_size,
    uint8_t* _n_msgs
);
