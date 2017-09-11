#include <string.h>
#include <stdlib.h>
#include <ert/rpc.h>
#include <ert/urpc.h>
#include <ert/fs.h>

/**
 * ERT RPC file system operation callback.
 */
static WIO_CALLBACK(ert_fs_rpc_cb) {
    //Original closure
    wio_closure_t* closure = (wio_closure_t*)data;

    //RPC call failed
    if (status)
        closure->func(closure->data, status, NULL);
    else {
        //u-RPC results
        uint8_t** rpc_results = (uint8_t**)result;
        //Number of results
        uint8_t n_results = rpc_results[0][0];

        //Operation result
        int16_t op_result;
        //Hacks to work around MSP430 memory alignment issue
        uint8_t* _op_result = (uint8_t*)(&op_result);
        _op_result[0] = rpc_results[1][0];
        _op_result[1] = rpc_results[1][1];

        //Operation succeed
        if (op_result>0) {
            //Callback is invoked with read data for "ert_read()"
            //or int16_t return value for other calls
            void* cb_result = (n_results==1)?(&op_result):(rpc_results[2]);

            //Invoke wrapped callback
            closure->func(closure->data, WIO_OK, cb_result);
        //Operation failed
        } else {
            //Callback is invoked with server-side POSIX error code
            op_result *= -1;

            //Invoke wrapped callback
            closure->func(closure->data, ERT_ERR_SYS_FAILED, &op_result);
        }
    }

    //Release closure memory
    free(closure);

    return WIO_OK;
}

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
    //Allocate closure memory
    wio_closure_t* closure = malloc(sizeof(wio_closure_t));
    if (!closure)
        return WIO_ERR_NO_MEMORY;
    //Initialize closure
    closure->func = cb;
    closure->data = cb_data;

    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ert_func_open,
        URPC_SIG(3, URPC_TYPE_VARY, URPC_TYPE_I16, URPC_TYPE_U16),
        URPC_ARG(URPC_VARY_CONST(path, strlen(path)), &flags, &mode),
        closure,
        ert_fs_rpc_cb
    );

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
    //Allocate closure memory
    wio_closure_t* closure = malloc(sizeof(wio_closure_t));
    if (!closure)
        return WIO_ERR_NO_MEMORY;
    //Initialize closure
    closure->func = cb;
    closure->data = cb_data;

    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ert_func_open,
        URPC_SIG(1, URPC_TYPE_I16),
        URPC_ARG(&fd),
        closure,
        ert_fs_rpc_cb
    );

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
ert_status_t ert_read(
    int fd,
    size_t size,
    void* cb_data,
    wio_callback_t cb
) {
    //Allocate closure memory
    wio_closure_t* closure = malloc(sizeof(wio_closure_t));
    if (!closure)
        return WIO_ERR_NO_MEMORY;
    //Initialize closure
    closure->func = cb;
    closure->data = cb_data;

    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ert_func_read,
        URPC_SIG(2, URPC_TYPE_I16, URPC_TYPE_U16),
        URPC_ARG(&fd, &size),
        closure,
        ert_fs_rpc_cb
    );

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
ert_status_t ert_write(
    int fd,
    const void* buf,
    size_t size,
    void* cb_data,
    wio_callback_t cb
) {
    //Allocate closure memory
    wio_closure_t* closure = malloc(sizeof(wio_closure_t));
    if (!closure)
        return WIO_ERR_NO_MEMORY;
    //Initialize closure
    closure->func = cb;
    closure->data = cb_data;

    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ert_func_open,
        URPC_SIG(2, URPC_TYPE_I16, URPC_TYPE_VARY),
        URPC_ARG(&fd, URPC_VARY_CONST(buf, size)),
        closure,
        ert_fs_rpc_cb
    );

    return WIO_OK;
}
