#pragma once

#include <stdint.h>
#include <wio.h>
#include <wtp.h>
#include <ert/urpc.h>
#include <ucontext.h>

//=== ERT async marcos ===
//User code await marco
#define ERT_AWAIT(status_var, result_type, result_var, func_name, ...) { \
        func_name(__VA_ARGS__, NULL, ert_user_resume); \
        ert_user_suspend(&status_var, sizeof(result_type), &result_var); \
    }

//=== ERT types ===
//ERT status type
typedef wio_status_t ert_status_t;

//=== ERT constants ===
//Blockwrite data buffer size
#define _ERT_BW_SIZE 0x20
static const uint8_t ERT_BW_SIZE = _ERT_BW_SIZE;
//EPC size
static const uint8_t ERT_EPC_SIZE = 12;
//WISP class
static const uint8_t ERT_WISP_CLASS = 0x10;
//ERT user stack size
static const uint16_t ERT_USER_STACK_SIZE = 200;

//=== ERT error codes ===
//Error code for failed remote system call
static const uint8_t ERT_ERR_SYS_FAILED = 0x30;

//=== ERT variables ===
//WTP endpoint
extern wtp_t* ert_wtp_ep;
//u-RPC endpoint
extern urpc_t* ert_rpc_ep;

//=== ERT runtime APIs ===
/**
 * ERT pre-init hook.
 */
extern void __attribute__((weak)) ert_pre_init();

/**
 * ERT user main routine.
 */
extern void __attribute__((weak)) ert_main();

/**
 * Suspend execution of user context.
 *
 * @param _status_var Pointer to status variable
 * @param result_size Size of result variable
 * @param _result_var Pointer to result variable
 */
extern void ert_user_suspend(
    ert_status_t* _status_var,
    uint16_t result_size,
    void* _result_var
);

/**
 * Resume execution of user context
 */
extern WIO_CALLBACK(ert_user_resume);

/**
 * WISP program entry point.
 */
extern void main(void);
