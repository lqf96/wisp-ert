#pragma once

#include <stdint.h>
#include <wio.h>
#include <ucontext.h>

//=== ERT async marcos ===
//Define async function marco
#define ERT_ASYNC_FUNC(func_name, ...) \
    func_name(ucontext_t* _async_ctx, __VA_ARGS__)
//TODO: Run async function
#define ERT_RUN_ASYNC(name, ...)
//Await marco
#define ERT_AWAIT(status_var, result_var, func_name, ...) { \
        WIO_TRY(func_name(__VA_ARGS__, _async_ctx, ert_async_resume)) \
        ert_async_suspend(_async_ctx, &status_var, &result_var); \
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

//=== ERT runtime APIs ===
/**
 * ERT pre-init hook.
 */
extern void __attribute__((weak)) ert_pre_init();

/**
 * Suspend execution of async function.
 *
 * @param _status Status code variable
 * @param _result Result variable
 */
extern void ert_async_suspend(
    ucontext_t* async_ctx,
    ert_status_t* _status,
    void** _result
);

/**
 * Resume execution of async function.
 */
extern WIO_CALLBACK(ert_async_resume);

/**
 * WISP program entry point.
 */
extern void main(void);
