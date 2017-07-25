#include "../Timing/timer.h"
#include "timer.h"

//Begin and end of pending timer linked list
static wio_timer_t* timer_begin = NULL;
static wio_timer_t* timer_end = NULL;

//Current time (In microseconds)
uint32_t current_time = 0;
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
    //Previous and next timer item
    timer->_prev = NULL;
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
    wio_timer_t* next_timer = timer_begin;

    //Already in use
    if (timer->flag)
        return WIO_ERR_ALREADY;

    //Set flag
    timer->flag = true;
    //Set time
    timer->_time = current_time+time;
    //Set callback and closure data
    timer->cb = cb;
    timer->cb_data = cb_data;

    //Find place to insert
    while (next_timer&&(next_timer->_time<current_time))
        next_timer = next_timer->_next;
    //Insert timer to linked list
    if (next_timer) {
        timer->_prev = next_timer->_prev;
        timer->_next = next_timer;

        next_timer->_prev->_next = timer;
        next_timer->_prev = timer;
    } else {
        timer->_prev = timer_end;
        timer->_next = NULL;

        if (!timer_end)
            timer_begin = timer;
        else
            timer_end->_next = timer;

        timer_end = timer;
    }

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
    wio_wait4_timeout(&system_timer, time/LP_LSDLY_1MS);
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_clear_timeout(
    wio_timer_t* timer
) {
    wio_timer_t* prev = timer->_prev;
    wio_timer_t* next = timer->_next;

    //Not in use
    if (!timer->flag)
        return WIO_OK;

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
    //Activate timers
    while (timer_begin&&(timer_begin->_time<=current_time)) {
        wio_timer_t* timer = timer_begin;

        //Invoke timer
        if (timer->cb)
            timer->cb(timer->cb_data, WIO_OK, NULL);
        //Update linked list begin item
        timer_begin = timer->_next;
        //Reset timer
        timer->flag = false;
    }

    //Update current time
    current_time++;
}
