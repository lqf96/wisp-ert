#include <unistd.h>

/**
 * {@inheritDoc}
 */
int close(int fd) {
    //Do u-RPC call
    urpc_call(
        rpc_ep,
        ERT_HANDLE_CLOSE,
        URPC_SIG(1, URPC_TYPE_I16),
        NULL,
        NULL,
        ert_resume_exec
    );
    //Suspend execution
    ert_suspend_exec();

    //TODO: Return value
    return 0;
}

/**
 * {@inheritDoc}
 */
ssize_t read(int fd, void* buf, size_t size) {
    //Do u-RPC call
    urpc_call(
        rpc_ep,
        ERT_HANDLE_READ,
        URPC_SIG(2, URPC_TYPE_I16, URPC_TYPE_U16),
        NULL,
        NULL,
        ert_resume_exec
    );
    //Suspend execution
    ert_suspend_exec();

    //TODO: Return value
    return 0;
}

/**
 * {@inheritDoc}
 */
extern ssize_t write(int fd, void* buf, size_t size) {
    //Do u-RPC call
    urpc_call(
        rpc_ep,
        ERT_HANDLE_WRITE,
        URPC_SIG(2, URPC_TYPE_I16, URPC_TYPE_VARY),
        NULL,
        NULL,
        ert_resume_exec
    );
    //Suspend execution
    ert_suspend_exec();

    //TODO: Return value
    return 0;
}
