#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <ert/runtime.h>

/**
 * @brief Open a remote file.
 *
 * @param path Remote file path.
 * @param flags Open flags.
 * @param mode Open mode.
 * @param cb_data Callback closure data.
 * @param cb Callback.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern ert_status_t ert_open(
    const char* path,
    int flags,
    mode_t mode,
    void* cb_data,
    wio_callback_t cb
);

/**
 * @brief Close a remote file.
 *
 * @param fd Remote file descriptor.
 * @param cb_data Callback closure data.
 * @param cb Callback.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern ert_status_t ert_close(
    int fd,
    void* cb_data,
    wio_callback_t cb
);

/**
 * @brief Read data from remote file.
 *
 * @param fd Remote file descriptor.
 * @param size Data size.
 * @param cb_data Callback closure data.
 * @param cb Callback.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern ert_status_t ert_read(
    int fd,
    size_t size,
    void* cb_data,
    wio_callback_t cb
);

/**
 * @brief Write data to remote file.
 *
 * @param fd Remote file descriptor.
 * @param buf Data to write.
 * @param size Size of data to write.
 * @param cb_data Callback closure data.
 * @param cb Callback.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern ert_status_t ert_write(
    int fd,
    const void* buf,
    size_t size,
    void* cb_data,
    wio_callback_t cb
);
