#pragma once

#include <wio.h>
#include "defs.h"
#include "transmission.h"

//WTP endpoint type
typedef struct wtp {
    //Downlink state
    wtp_link_state_t _downlink_state;
    //Uplink state
    wtp_link_state_t _uplink_state;

    //EPC buffer
    wio_buf_t _epc_buf;
    //Read memory
    uint8_t* _read_mem;
    //Write memory
    uint8_t* _write_mem;

    //Transmit control instance
    wtp_tx_ctrl_t _tx_ctrl;
    //Receive control instance
    wtp_rx_ctrl_t _rx_ctrl;

    //TODO: Send message callbacks

    //Receive callbacks
    wio_callback_t* _recv_cb;
    //Receive callbacks closure data
    void** _recv_cb_data;
    //Receive callbacks queue begin
    uint8_t _recv_cb_begin;
    //Receive callbacks queue end
    uint8_t _recv_cb_end;
    //Receive callbacks queue size
    uint8_t _recv_cb_size;

    //Event callbacks
    wio_callback_t _event_cb[_WTP_EVENT_MAX];
    //Event callbacks closure data
    void* _event_cb_data[_WTP_EVENT_MAX];
} wtp_t;

/**
 * Initialize WTP endpoint.
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
    uint8_t n_recv_cb
);

/**
 * Finalize WTP endpoint.
 *
 * @param self WTP endpoint instance
 */
extern wtp_status_t wtp_fini(
    wtp_t* self
);

/**
 * Connect to WTP server.
 *
 * @param self WTP endpoint instance
 */
extern wtp_status_t wtp_connect(
    wtp_t* self
);

/**
 * Disconnect from WTP server.
 *
 * @param self WTP endpoint instance
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
    uint16_t size,
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
 * Set WTP event handler.
 *
 * @param self WTP endpoint instance
 * @param event WTP event
 * @param cb_data Callback closure data
 * @param cb Callback function
 */
extern wtp_status_t wtp_on_event(
    wtp_t* self,
    wtp_event_t event,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Trigger WTP event.
 *
 * @param self WTP endpoint instance
 * @param event WTP event
 * @param status Event status
 * @param result Event result
 */
extern wtp_status_t wtp_trigger_event(
    wtp_t* self,
    wtp_event_t event,
    wtp_status_t status,
    void* result
);

/**
 * Fill RFID READ memory.
 *
 * @param self WTP endpoint instance
 */
extern wtp_status_t wtp_fill_read_mem(
    wtp_t* self
);

/**
 * Handle RFID BLOCKWRITE operation.
 *
 * @param self WTP endpoint instance
 * @param size BLOCKWRITE data size
 */
extern wtp_status_t wtp_handle_blockwrite(
    wtp_t* self,
    uint16_t size
);

/**
 * Fill RFID EPC memory.
 *
 * @param self WTP endpoint instance
 */
extern wtp_status_t wtp_fill_epc_mem(
    wtp_t* self
);
