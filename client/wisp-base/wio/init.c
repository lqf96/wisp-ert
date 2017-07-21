#include "timer.h"

/**
 * {@inheritDoc}
 */
wio_status_t wio_init() {
    //Initialize system timer
    wio_timer_init(&system_timer);

    return WIO_OK;
}
