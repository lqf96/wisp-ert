#include <stdlib.h>
#include <string.h>
#include "queue.h"

/**
 * {@inheritDoc}
 */
wio_status_t wio_queue_init(
    wio_queue_t* self,
    uint16_t item_size,
    uint16_t capacity
) {
    //Set item size and capacity
    self->item_size = item_size;
    self->capacity = capacity;
    //Size
    self->size = 0;

    //Begin and end index
    self->begin = self->end = 0;
    //Allocate space for queue data
    uint8_t* data = malloc(item_size*capacity);
    if (!data)
        return WIO_ERR_NO_MEMORY;
    self->data = data;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_queue_fini(
    wio_queue_t* self
) {
    //Release queue memory
    free(self->data);

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_queue_push(
    wio_queue_t* self,
    const void* item
) {
    //Queue is full
    if (self->size>=self->capacity)
        return WIO_ERR_NO_MEMORY;

    //Get next available position
    uint8_t* avail_pos = WIO_QUEUE_AT(self, uint8_t, self->begin);
    //Copy queue item
    memcpy(avail_pos, item, self->item_size);
    //Update queue begin
    self->begin++;
    if (self->begin>=self->capacity)
        self->begin = 0;
    //Update queue size
    self->size++;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_queue_pop(
    wio_queue_t* self,
    void* item
) {
    //Queue is empty
    if (self->size==0)
        return WIO_ERR_EMPTY;

    //Get queue end data
    uint8_t* end_pos = WIO_QUEUE_AT(self, uint8_t, self->end);
    //Copy queue item
    if (item)
        memcpy(item, end_pos, self->item_size);
    //Update queue end
    self->end++;
    if (self->end>=self->capacity)
        self->end = 0;
    //Update queue size
    self->size--;

    return WIO_OK;
}
