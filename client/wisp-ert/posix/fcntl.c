#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <runtime.h>
#include <urpc.h>
#include <rpc.h>

/**
 * {@inheritDoc}
 */
extern int open(const char* path, int flags, ...) {
    mode_t mode = 0;

    //Read mode parameter when creating file
    if (flags&O_CREAT) {
        va_list args;

        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    //Do u-RPC call
    urpc_call(
        ert_rpc_ep,
        ERT_HANDLE(open),
        URPC_SIG(3, URPC_TYPE_VARY, URPC_TYPE_I16, URPC_TYPE_U16),
        URPC_ARG(WIO_VARY(strlen(path), path), &flags, &mode),
        NULL,
        ert_resume_exec
    );
    //Suspend execution
    ert_suspend_exec();

    //Return result
    ERT_RETURN(int16_t)
}
