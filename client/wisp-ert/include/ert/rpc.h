#pragma once

#include <ert/runtime.h>
#include <ert/urpc.h>

//=== ERT RPC marcos ===
/// ERT constant helper
#define ERT_CONST(name) (ert_consts_store.name)

//=== ERT constants ===
/// ERT filesystem constants structure
typedef struct __attribute__((packed)) ert_consts {
    //Open flags
    int16_t O_CREAT;
    int16_t O_RDONLY;
    int16_t O_WRONLY;
    int16_t O_RDWR;
    int16_t O_APPEND;

    //Whence
    int16_t SEEK_SET;
    int16_t SEEK_CUR;
    int16_t SEEK_END;

    //Error numbers
    int16_t EBADF;
    int16_t EINVAL;
} ert_consts_t;

/// ERT filesystem constants store
extern ert_consts_t ert_consts_store;

//=== ERT functions definition ===
/// Get ERT service constants function handle
static const urpc_func_t ert_func_srv_consts = 0;
/// Open remote file function handle
static const urpc_func_t ert_func_open = 1;
/// Close remote file function handle
static const urpc_func_t ert_func_close = 2;
/// Read remote file function handle
static const urpc_func_t ert_func_read = 3;
/// Write remote file function handle
static const urpc_func_t ert_func_write = 4;
