#include <string.h>
#include "wio.h"

//Begin and end of timer linked list
static wio_timer_t* timer_begin = NULL;
static wio_timer_t* timer_end = NULL;

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
    //TODO: Set timeout
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_clear_timeout(
    wio_timer_t* timer
) {
    //TODO: Clear timeout
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_no_op(
    void* data,
    wio_status_t status,
    void* value
) {
    return WIO_OK;
}
