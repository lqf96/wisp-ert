#pragma once

#include <stdbool.h>
#include <stddef.h>

//Simple exception handling marco
//(Usage is similar to rust's "try!")
#define WIO_TRY(expr) { \
        wio_status_t status = expr; \
        if (status) \
            return status; \
    }
//Callback declaration helper
#define WIO_CALLBACK(name) \
    name(void* data, wio_status_t status, void* result)

//WIO status type
typedef uint8_t wio_status_t;
//WIO callback type
typedef wio_status_t (*wio_callback_t)(void*, wio_status_t, void*);

//WIO buffer type
typedef struct wio_buf {
    //Buffer
    uint8_t* buffer;
    //Buffer size
    size_t size;

    //Cursor A
    size_t pos_a;
    //Cursor B
    size_t pos_b;
} wio_buf_t;

//WIO timer type
typedef struct wio_timer {
    //In use flag
    bool flag;
    //Callback closure data
    void* cb_data;
    //Callback function
    wio_callback_t cb;

    //Trigger time
    uint16_t _time;
    //Previous timer
    struct wio_timer* _prev;
    //Next timer
    struct wio_timer* _next;
} wio_timer_t;

//No error
const wio_status_t WIO_OK = 0x00;
//Out of range
const wio_status_t WIO_ERR_OUT_OF_RANGE = 0x01;
//No memory
const wio_status_t WIO_ERR_NO_MEMORY = 0x02;
//Already in use
const wio_status_t WIO_ERR_ALREADY = 0x03;

//Current time
extern uint16_t current_time;

/**
 * Initialize WIO buffer.
 *
 * @param self WIO buffer instance
 * @param buffer Underlying buffer
 * @param size Size of underlying buffer
 * @return Operation status
 */
extern wio_status_t wio_buf_init(
    wio_buf_t* self,
    uint8_t* buffer,
    size_t size
);

/**
 * Read data from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param data Read data
 * @param size Size of data to read
 * @return Operation status
 */
extern wio_status_t wio_read(
    wio_buf_t* self,
    void* data,
    size_t size
);

/**
 * Write data to WIO buffer.
 *
 * @param self WIO buffer instance
 * @param data Write data
 * @param size Size of data to read
 * @return Operation status
 */
extern wio_status_t wio_write(
    wio_buf_t* self,
    void* data,
    size_t size
);

/**
 * Allocate memory from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param size Size of the memory
 * @param ptr Pointer to allocated memory
 * @return Operation status
 */
extern wio_status_t wio_alloc(
    wio_buf_t* self,
    size_t size,
    void** ptr
);

/**
 * Free memory from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param size Size of the memory
 * @return Operation status
 */
extern wio_status_t wio_free(
    wio_buf_t* self,
    size_t size
);

/**
 * Initialize timer
 *
 * @param timer Timer instance
 * @return Operation status
 */
extern wio_status_t wio_timer_init(
    wio_timer_t* timer
);

/**
 * Set timeout on timer.
 *
 * @param timer Timer instance
 * @param time Time
 * @param cb_data Callback closure data
 * @param cb Callback function
 * @return Operation status
 */
extern wio_status_t wio_set_timeout(
    wio_timer_t* timer,
    uint16_t time,
    void* cb_data,
    wio_callback_t cb
);

/**
 * Wait for timeout.
 *
 * @param timer Timer instance
 * @param time Time
 * @return Operation status
 */
extern wio_status_t wio_wait4_timeout(
    wio_timer_t* timer,
    uint16_t time
);

/**
 * Clear timeout on timer.
 *
 * @param timer Timer instance
 */
extern wio_status_t wio_clear_timeout(
    wio_timer_t* timer
);

/**
 * Timer interrupt callback.
 */
extern void wio_timer_callback();
