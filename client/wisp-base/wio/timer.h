#pragma once

#include "defs.h"

/// WIO timer type
typedef struct wio_timer {
    /// In use flag
    bool flag;
    /// Callback closure data
    void* cb_data;
    /// Callback function
    wio_callback_t cb;

    /// Trigger time
    uint32_t _time;
    /// Previous timer
    struct wio_timer* _prev;
    /// Next timer
    struct wio_timer* _next;
} wio_timer_t;

/// Current time (In unit of 20ms)
extern uint32_t current_time;

/**
 * @brief Initialize timer.
 *
 * @param timer Timer instance.
 * @return WIO_OK.
 */
extern wio_status_t wio_timer_init(
    wio_timer_t* timer
);

/**
 * @brief Set timeout on timer.
 *
 * @param timer Timer instance.
 * @param time Time.
 * @param cb_data Callback closure data.
 * @param cb Callback function.
 * @return WIO_OK.
 */
extern wio_status_t wio_set_timeout(
    wio_timer_t* timer,
    uint16_t time,
    void* cb_data,
    wio_callback_t cb
);

/**
 * @brief Wait for timer to time out.
 *
 * @param timer Timer instance.
 * @param time Time.
 * @return WIO_OK.
 */
extern wio_status_t wio_wait4_timeout(
    wio_timer_t* timer,
    uint16_t time
);

/**
 * @brief Clear timeout on timer.
 *
 * @param timer Timer instance.
 */
extern wio_status_t wio_clear_timeout(
    wio_timer_t* timer
);

/**
 * @brief Initialize WIO timer subsystem.
 */
extern wio_status_t wio_timer_subsys_init();

/**
 * @brief Timer interrupt callback.
 */
extern void wio_timer_callback();
