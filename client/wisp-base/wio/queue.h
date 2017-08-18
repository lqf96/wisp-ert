#pragma once

#include "defs.h"

//WIO queue member helper marco
#define WIO_QUEUE_AT(queue, type, index) \
    ((type*)(queue->data+index*queue->item_size))
//WIO queue begin helper marco
#define WIO_QUEUE_BEGIN(queue, type) \
    WIO_QUEUE_AT(queue, type, ((queue->begin==0)?(queue->capacity-1):(queue->begin-1)))
//WIO queue end helper marco
#define WIO_QUEUE_END(queue, type) \
    WIO_QUEUE_AT(queue, type, queue->end)

//WIO circular queue type
typedef struct wio_queue {
    //Queue size
    uint16_t size;
    //Queue capacity
    uint16_t capacity;
    //Item size
    uint16_t item_size;

    //Queue data
    uint8_t* data;

    //Queue begin
    uint16_t begin;
    //Queue end
    uint16_t end;
} wio_queue_t;

/**
 * Initialize a WIO queue.
 *
 * @param self WIO queue instance
 * @param item_size Queue item size
 * @param capacity Queue capacity
 */
wio_status_t wio_queue_init(
    wio_queue_t* self,
    uint16_t item_size,
    uint16_t capacity
);

/**
 * Finalize a WIO queue.
 *
 * @param self WIO queue instance
 */
wio_status_t wio_queue_fini(
    wio_queue_t* self
);

/**
 * Push an item into the queue.
 *
 * @param self WIO queue instance
 * @param item Item to be pushed into the queue
 */
wio_status_t wio_queue_push(
    wio_queue_t* self,
    const void* item
);

/**
 * Pop an item from the queue.
 *
 * @param self WIO queue instance
 * @param _item Item to be popped from the queue
 */
wio_status_t wio_queue_pop(
    wio_queue_t* self,
    void* item
);
