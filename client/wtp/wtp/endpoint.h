#pragma once

#include <wio.h>
#include "defs.h"
#include "transmission.h"

/// WTP endpoint type
typedef struct wtp {
    /// Downlink state
    wtp_link_state_t _downlink_state;
    /// Uplink state
    wtp_link_state_t _uplink_state;

    /// EPC buffer
    wio_buf_t _epc_buf;
    /// Read memory
    uint8_t* _read_mem;
    /// BlockWrite memory
    uint8_t* _write_mem;

    /// Read memory loaded flag
    bool _read_mem_loaded;

    /// Packet begin position
    uint16_t _pkt_begin;

    /// Transmit control instance
    wtp_tx_ctrl_t _tx_ctrl;
    /// Receive control instance
    wtp_rx_ctrl_t _rx_ctrl;

    /// Send message callbacks queue
    wio_queue_t _send_cb_queue;
    /// Send message callback closure data queue
    wio_queue_t _send_cb_data_queue;

    /// Receive callbacks queue
    wio_queue_t _recv_cb_queue;
    /// Receive callbacks closure data queue
    wio_queue_t _recv_cb_data_queue;

    /// Event callbacks
    wio_callback_t _event_cb[_WTP_EVENT_MAX];
    /// Event callbacks closure data
    void* _event_cb_data[_WTP_EVENT_MAX];
} wtp_t;

/**
 * @brief Initialize WTP endpoint.
 *
 * @param self WTP endpoint instance.
 * @param epc_mem EPC-96 data section memory.
 * @param epc_buf_size Size of the EPC-96 data section.
 * @param read_mem Read memory.
 * @param write_mem BlockWrite memory.
 * @param window_size WTP sliding window size.
 * @param timeout WTP packet retransmission timeout.
 * @param tx_buf_size Transmit control buffer size.
 * @param rx_buf_size Receive control buffer size.
 * @param n_send Capacity of send callbacks.
 * @param n_recv Capacity of receive callbacks.
 * @return WIO_ERR_NO_MEMORY if memory allocation failed, otherwise WIO_OK.
 */
extern wtp_status_t wtp_init(
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
);

/**
 * @brief Finalize WTP endpoint.
 *
 * @param self WTP endpoint instance.
 * @return WIO_OK.
 */
extern wtp_status_t wtp_fini(
    wtp_t* self
);

/**
 * @brief Connect to WTP server.
 *
 * @param self WTP endpoint instance.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern wtp_status_t wtp_connect(
    wtp_t* self
);

/**
 * @brief Disconnect from WTP server.
 *
 * @param self WTP endpoint instance.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern wtp_status_t wtp_close(
    wtp_t* self
);

/**
 * @brief Send message to WTP server.
 *
 * @param self WTP endpoint instance.
 * @param data Data to send.
 * @param size Size of data.
 * @param cb_data Callback closure data.
 * @param cb Callback function.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern wtp_status_t wtp_send(
    wtp_t* self,
    uint8_t* data,
    uint16_t size,
    void* cb_data,
    wio_callback_t cb
);

/**
 * @brief Receive message from WTP server.
 *
 * @param self WTP endpoint instance.
 * @param cb_data Callback closure data.
 * @param cb Callback function.
 * @return WIO_ERR_NO_MEMORY if no memory available for storing callback, otherwise WIO_OK.
 */
extern wtp_status_t wtp_recv(
    wtp_t* self,
    void* cb_data,
    wio_callback_t cb
);

/**
 * @brief Set WTP event handler.
 *
 * @param self WTP endpoint instance.
 * @param event WTP event.
 * @param cb_data Callback closure data.
 * @param cb Callback function.
 * @return WIO_ERR_INVALID for invalid event, otherwise WIO_OK.
 */
extern wtp_status_t wtp_on_event(
    wtp_t* self,
    wtp_event_t event,
    void* cb_data,
    wio_callback_t cb
);

/**
 * @brief Trigger WTP event.
 *
 * @param self WTP endpoint instance.
 * @param event WTP event.
 * @param status Event status.
 * @param result Event result.
 * @return Status code returned by repsective callback function.
 */
extern wtp_status_t wtp_trigger_event(
    wtp_t* self,
    wtp_event_t event,
    wtp_status_t status,
    void* result
);

/**
 * @brief Load RFID READ memory for WTP communication.
 *
 * @param self WTP endpoint instance.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern wtp_status_t wtp_load_read_mem(
    wtp_t* self
);

/**
 * @brief Handle RFID BLOCKWRITE operation.
 *
 * @param self WTP endpoint instance.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern wtp_status_t wtp_handle_blockwrite(
    wtp_t* self
);

/**
 * @brief Verify the checksum of received WTP packet.
 *
 * @param self WTP endpoint instance.
 * @param write_buf Received WTP packets buffer.
 * @return Error code if checksum validation failed, otherwise WIO_OK.
 */
extern wtp_status_t wtp_verify_checksum(
    wtp_t* self,
    wio_buf_t* write_buf
);

/**
 * @brief WTP Xor checksum function.
 *
 * @param mem Memory to calculate checksum.
 * @param begin Begin index for checksum calculation.
 * @param end End index for checksum calculation.
 * @return Xor checksum for given range of memory.
 */
extern uint8_t wtp_xor_checksum(
    uint8_t* mem,
    uint16_t begin,
    uint16_t end
);
