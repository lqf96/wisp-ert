#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <ert/runtime.h>

/**
 * Open a file.
 *
 * @param path File path
 * @param flags Open flags
 * @param mode Open mode
 * @param cb_data Callback closure data
 * @param cb Callback
 */
extern ert_status_t ert_open(
    const char* path,
    int flags,
    mode_t mode,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Close a file.
 *
 * @param fd File descriptor
 * @param cb_data Callback closure data
 * @param cb Callback
 */
extern ert_status_t ert_close(
    int fd,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Read data from file.
 *
 * @param fd File descriptor
 * @param buf Read data
 * @param size Data size
 * @param cb_data Callback closure data
 * @param cb Callback
 */
extern ert_status_t ert_read(
    int fd,
    void* buf,
    size_t size,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Write data to file.
 *
 * @param fd File descriptor
 * @param buf Write data
 * @param size Data size
 * @param cb_data Callback closure data
 * @param cb Callback
 */
extern ert_status_t ert_write(
    int fd,
    void* buf,
    size_t size,
    void* cb_data,
    wio_callback_t cb
);
