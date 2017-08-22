#include <ert/fs.h>

/**
 * {@inheritDoc}
 */
ert_status_t ert_open(
    const char* path,
    int flags,
    mode_t mode,
    void* cb_data,
    wio_callback_t cb
) {
    //TODO: RPC call
    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
ert_status_t ert_close(
    int fd,
    void* cb_data,
    wio_callback_t cb
) {
    //TODO: RPC call
    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
ert_status_t ert_read(
    int fd,
    void* buf,
    size_t size,
    void* cb_data,
    wio_callback_t cb
) {
    //TODO: RPC call
    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
ert_status_t ert_write(
    int fd,
    void* buf,
    size_t size,
    void* cb_data,
    wio_callback_t cb
) {
    //TODO: RPC call
    return WIO_OK;
}
