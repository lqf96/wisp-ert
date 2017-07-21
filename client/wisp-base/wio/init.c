#include "../Timing/timer.h"
#include "timer.h"

/**
 * {@inheritDoc}
 */
wio_status_t wio_init() {
    //Enable CCR0 interrupt
    TA2CCTL0 = CCIE;
    //Timer interrupt interval (1ms)
    TA2CCR0 = LP_LSDLY_1MS;
    //ACLK(=REFO), up mode, clear TAR
    TA2CTL = TASSEL_1|MC_1|TACLR;

    //Enable interrupts
    __bis_SR_register(LPM3_bits|GIE);

    //Initialize system timer
    wio_timer_init(&system_timer);

    return WIO_OK;
}
