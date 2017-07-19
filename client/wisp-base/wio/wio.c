#include <string.h>
#include "../globals.h"
#include "wio.h"

//Begin and end of pending timer linked list
static wio_timer_t* timer_begin = NULL;
static wio_timer_t* timer_end = NULL;

//System timer (Used for polyfilling WISP firmware functions)
static wio_timer_t system_timer;

/**
 * {@inheritDoc}
 */
wio_status_t wio_init() {
    //Initialize system timer
    wio_timer_init(&system_timer);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_buf_init(
    wio_buf_t* self,
    uint8_t* buffer,
    size_t size
) {
    //Set up buffer and size
    self->buffer = buffer;
    self->size = size;

    //Initialize cursors
    self->pos_a = 0;
    self->pos_b = 0;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_read(
    wio_buf_t* self,
    void* data,
    size_t size
) {
    //Out of range check
    if (self->pos_a+size>self->size)
        return WIO_ERR_OUT_OF_RANGE;
    //Copy data
    memcpy(data, self->buffer+self->pos_a, size);
    //Update cursor
    self->pos_a += size;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_write(
    wio_buf_t* self,
    void* data,
    size_t size
) {
    //Out of range check
    if (self->pos_b+size>self->size)
        return WIO_ERR_OUT_OF_RANGE;
    //Copy data
    memcpy(self->buffer+self->pos_b, data, size);
    //Update cursor
    self->pos_b += size;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_alloc(
    wio_buf_t* self,
    size_t size,
    void** ptr
) {
    //TODO: Memory allocation

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_free(
    wio_buf_t* self,
    size_t size
) {
    //Update cursor
    self->pos_a += size;

    return WIO_OK;
}

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
