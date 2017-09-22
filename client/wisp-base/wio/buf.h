#pragma once

#include "defs.h"

/// WIO buffer type
typedef struct wio_buf {
    /// Buffer
    uint8_t* buffer;
    /// Buffer size
    uint16_t size;

    /// Cursor A (Read cursor)
    uint16_t pos_a;
    /// Cursor B (Write cursor)
    uint16_t pos_b;
} wio_buf_t;

/**
 * @brief Initialize WIO buffer.
 *
 * @param self WIO buffer instance.
 * @param buffer Underlying buffer.
 * @param size Size of underlying buffer.
 * @return WIO_OK.
 */
extern wio_status_t wio_buf_init(
    wio_buf_t* self,
    uint8_t* buffer,
    uint16_t size
);

/**
 * @brief Initialize WIO buffer with dynamically allocated memory.
 *
 * @param self WIO buffer instance.
 * @param size Size of underlying buffer.
 * @return WIO_ERR_NO_MEMORY if allocation failed, otherwise WIO_OK.
 */
extern wio_status_t wio_buf_alloc_init(
    wio_buf_t* self,
    uint16_t size
);

/**
 * @brief Read data from WIO buffer.
 *
 * @param self WIO buffer instance.
 * @param data Pointer to memory for holding read data.
 * @param size Size of data to read.
 * @return WIO_ERR_OUT_OF_RANGE if read beyond buffer range, otherwise WIO_OK.
 */
extern wio_status_t wio_read(
    wio_buf_t* self,
    void* data,
    uint16_t size
);

/**
 * @brief Write data to WIO buffer.
 *
 * @param self WIO buffer instance.
 * @param data Pointer to write data.
 * @param size Size of data to write.
 * @return WIO_ERR_OUT_OF_RANGE if write beyond buffer range, otherwise WIO_OK.
 */
extern wio_status_t wio_write(
    wio_buf_t* self,
    const void* data,
    uint16_t size
);

/**
 * @brief Copy data from one WIO buffer to another.
 *
 * @param from Source WIO buffer.
 * @param to Target WIO buffer.
 * @param size Size of data to copy.
 * @return WIO_ERR_OUT_OF_RANGE if out-of-range read or write happens, otherwise WIO_OK.
 */
extern wio_status_t wio_copy(
    wio_buf_t* from,
    wio_buf_t* to,
    uint16_t size
);

/**
 * @brief Allocate memory from WIO buffer in a circlular manner.
 *
 * @param self WIO buffer instance.
 * @param size Size of the memory to allocate.
 * @param _ptr Pointer to memory for holding pointer to allocated memory.
 * @return WIO_ERR_NO_MEMORY if no memory available for allocation, otherwise WIO_OK.
 */
extern wio_status_t wio_alloc(
    wio_buf_t* self,
    uint16_t size,
    void* _ptr
);

/**
 * @brief Free memory from WIO buffer in a circular manner.
 *
 * @param self WIO buffer instance.
 * @param size Size of the memory to free.
 * @return WIO_OK.
 */
extern wio_status_t wio_free(
    wio_buf_t* self,
    uint16_t size
);
