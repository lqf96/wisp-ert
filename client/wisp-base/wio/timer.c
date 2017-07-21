#include "../globals.h"
#include "timer.h"

//Begin and end of pending timer linked list
static wio_timer_t* timer_begin = NULL;
static wio_timer_t* timer_end = NULL;

//System timer (Used for polyfilling WISP firmware functions)
wio_timer_t system_timer;

/**
 * {@inheritDoc}
 */
wio_status_t wio_timer_init(
    wio_timer_t* timer
) {
    //Not in use
    timer->flag = false;
    //Callback and closure data
    timer->cb = NULL;
    timer->cb_data = NULL;

    //Trigger time
    timer->_time = 0;
    //Next timer item
    timer->_next = NULL;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_set_timeout(
    wio_timer_t* timer,
    uint16_t time,
    void* cb_data,
    wio_callback_t cb
) {
    //Already in use
    if (timer->flag)
        return WIO_ERR_ALREADY;

    //TODO: Add timer to linked list

    //Set flag
    timer->flag = true;
    //TODO: Set time
    timer->_time = 0;
    //Set callback and closure data
    timer->cb = cb;
    timer->cb_data = cb_data;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_wait4_timeout(
    wio_timer_t* timer,
    uint16_t time
) {
    //Set timeout
    WIO_TRY(wio_set_timeout(timer, time, NULL, NULL))

    //Wait for timer to finish
    while (timer->flag);

    return WIO_OK;
}

/**
 * Generate timer delay (WISP firmware polyfill function)
 *
 * @param time Time
 */
void Timer_LooseDelay(uint16_t time) {
    wio_wait4_timeout(&system_timer, time);
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_clear_timeout(
    wio_timer_t* timer
) {
    wio_timer_t* prev;
    wio_timer_t* next;

    //Not in use
    if (!timer->flag)
        return WIO_OK;

    prev = timer->_prev;
    next = timer->_next;
    //Remove timer from linked list
    if (prev)
        prev->_next = next;
    else
        timer_begin = next;
    if (next)
        next->_prev = prev;
    else
        timer_end = prev;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
void wio_timer_callback() {
    //TODO: Timer interrupt callback
}
