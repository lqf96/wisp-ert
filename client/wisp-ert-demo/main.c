#include "wio.h"
#include "wisp-base.h"

//ERT user code entry point
wio_status_t ert_main(int argc, char** argv) {
    return WIO_OK;
}

bool led_state = false;
wio_timer_t timer;

WIO_CALLBACK(toggle_led) {
    if (led_state) {
        BITCLR(PLED1OUT,PIN_LED1);
        led_state = false;
    } else {
        BITSET(PLED1OUT,PIN_LED1);
        led_state = true;
    }

    wio_set_timeout(&timer, 1000, NULL, toggle_led);

    return WIO_OK;
}

void main(void) {
    wio_timer_t timer;

    WISP_init();

    wio_timer_init(&timer);
    wio_set_timeout(&timer, 2, NULL, toggle_led);

    wio_init();

    while (true);
}
