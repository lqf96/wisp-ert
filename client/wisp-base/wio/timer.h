#pragma once

#include "defs.h"

//WIO timer type
typedef struct wio_timer {
    //In use flag
    bool flag;
    //Callback closure data
    void* cb_data;
    //Callback function
    wio_callback_t cb;

    //Trigger time
    uint32_t _time;
    //Previous timer
    struct wio_timer* _prev;
    //Next timer
    struct wio_timer* _next;
} wio_timer_t;

//Current time (In microseconds)
extern uint32_t current_time;
//System timer (Used for polyfilling WISP firmware functions)
extern wio_timer_t system_timer;

/**
 * Initialize timer
 *
 * @param timer Timer instance
 * @return Operation status
 */
extern wio_status_t wio_timer_init(
    wio_timer_t* timer
);

/**
 * Set timeout on timer.
 *
 * @param timer Timer instance
 * @param time Time
 * @param cb_data Callback closure data
 * @param cb Callback function
 * @return Operation status
 */
extern wio_status_t wio_set_timeout(
    wio_timer_t* timer,
    uint16_t time,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Wait for timeout.
 *
 * @param timer Timer instance
 * @param time Time
 * @return Operation status
 */
extern wio_status_t wio_wait4_timeout(
    wio_timer_t* timer,
    uint16_t time
);

/**
 * Clear timeout on timer.
 *
 * @param timer Timer instance
 */
extern wio_status_t wio_clear_timeout(
    wio_timer_t* timer
);

/**
 * Timer interrupt callback.
 */
extern void wio_timer_callback();
