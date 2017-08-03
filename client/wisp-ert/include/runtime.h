#pragma once

#include <urpc.h>
#include "wio.h"
#include "wtp.h"

//ERT function return statements
#define ERT_RETURN(type) return *((type*)ert_rpc_ret);
//ERT function handle
#define ERT_HANDLE(name) ert_func_##name

//WTP endpoint
extern wtp_t* ert_wtp_ep;
//u-RPC endpoint
extern urpc_t* ert_rpc_ep;

//ERT u-RPC call result
extern void* ert_rpc_ret;

extern void ert_main();

/**
 * WTP callback function for resuming ERT program execution.
 */
WIO_CALLBACK(ert_resume_exec);

/**
 * Suspend ERT program execution.
 */
extern void ert_suspend_exec();
