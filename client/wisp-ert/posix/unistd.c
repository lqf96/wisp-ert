#include <unistd.h>
#include <runtime.h>
#include <urpc.h>
#include <rpc.h>

/**
 * {@inheritDoc}
 */
int close(int fd) {
    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ERT_HANDLE(close),
        URPC_SIG(1, URPC_TYPE_I16),
        URPC_ARG(&fd),
        NULL,
        ert_resume_exec
    );
    //Suspend execution
    ert_suspend_exec();

    //Return result
    ERT_RETURN(int16_t)
}

/**
 * {@inheritDoc}
 */
ssize_t read(int fd, void* buf, size_t size) {
    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ERT_HANDLE(read),
        URPC_SIG(2, URPC_TYPE_I16, URPC_TYPE_U16),
        URPC_ARG(&fd, &size),
        NULL,
        ert_resume_exec
    );
    //Suspend execution
    ert_suspend_exec();

    //Return result
    ERT_RETURN(int16_t)
}

/**
 * {@inheritDoc}
 */
extern ssize_t write(int fd, void* buf, size_t size) {
    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ERT_HANDLE(write),
        URPC_SIG(2, URPC_TYPE_I16, URPC_TYPE_VARY),
        URPC_ARG(&fd, WIO_VARY(size, buf)),
        NULL,
        ert_resume_exec
    );
    //Suspend execution
    ert_suspend_exec();

    //Return result
    ERT_RETURN(int16_t)
}
