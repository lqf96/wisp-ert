#pragma once

#include "defs.h"

//WIO variable length data container type
typedef struct wio_vary {
    //Data
    void* data;
    //Size of data
    uint16_t size;
} wio_vary_t;

//WIO buffer type
typedef struct wio_buf {
    //Buffer
    uint8_t* buffer;
    //Buffer size
    uint16_t size;

    //Cursor A (Reading)
    uint16_t pos_a;
    //Cursor B (Writing)
    uint16_t pos_b;
} wio_buf_t;

//Current time
extern uint32_t current_time;

/**
 * Initialize WIO buffer.
 *
 * @param self WIO buffer instance
 * @param buffer Underlying buffer
 * @param size Size of underlying buffer
 * @return Operation status
 */
extern wio_status_t wio_buf_init(
    wio_buf_t* self,
    uint8_t* buffer,
    uint16_t size
);

/**
 * Read data from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param data Read data
 * @param size Size of data to read
 * @return Operation status
 */
extern wio_status_t wio_read(
    wio_buf_t* self,
    void* data,
    uint16_t size
);

/**
 * Write data to WIO buffer.
 *
 * @param self WIO buffer instance
 * @param data Write data
 * @param size Size of data to read
 * @return Operation status
 */
extern wio_status_t wio_write(
    wio_buf_t* self,
    const void* data,
    uint16_t size
);

/**
 * Copy data from one WIO buffer to another
 *
 * @param from Source WIO buffer
 * @param to Target WIO buffer
 * @param size Size of data to copy
 */
extern wio_status_t wio_copy(
    wio_buf_t* from,
    wio_buf_t* to,
    uint16_t size
);

/**
 * Allocate memory from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param size Size of the memory
 * @param ptr Pointer to allocated memory
 * @return Operation status
 */
extern wio_status_t wio_alloc(
    wio_buf_t* self,
    uint16_t size,
    void* _ptr
);

/**
 * Free memory from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param size Size of the memory
 * @return Operation status
 */
extern wio_status_t wio_free(
    wio_buf_t* self,
    uint16_t size
);

/**
 * Reset WIO buffer.
 *
 * @param self WIO buffer instance
 * @return WIO_OK
 */
extern wio_status_t wio_reset(
    wio_buf_t* self
);
