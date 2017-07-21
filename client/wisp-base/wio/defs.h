#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//Simple exception handling marco
//(Usage is similar to rust's "try!")
#define WIO_TRY(expr) { \
        wio_status_t status = expr; \
        if (status) \
            return status; \
    }
//Callback declaration helper
#define WIO_CALLBACK(name) \
    wio_status_t name(void* data, wio_status_t status, void* result)

//WIO status type
typedef uint8_t wio_status_t;
//WIO callback type
typedef wio_status_t (*wio_callback_t)(void*, wio_status_t, void*);

//No error
static const wio_status_t WIO_OK = 0x00;
//Out of range
static const wio_status_t WIO_ERR_OUT_OF_RANGE = 0x01;
//No memory
static const wio_status_t WIO_ERR_NO_MEMORY = 0x02;
//Already in use
static const wio_status_t WIO_ERR_ALREADY = 0x03;

/**
 * Initialize WIO library
 *
 * @return WIO_OK
 */
extern wio_status_t wio_init();
