/**
 * @file timer.c
 *
 * Provides hardware and software delay and alarm/scheduling functions
 *
 * @author Aaron Parks, Qifan Lu
 */

#include "timer.h"
#include "../globals.h"

//WIO timer interrupt callback
extern void wio_timer_callback();

//The implementation of function "Timer_LooseDelay()" has been moved to "wio/wio.c".
//The function is only provided for compatibility purposes.
//You are encouraged to use WIO timer APIs instead.

//----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////
// INT_TimerA1
//
// Interrupt 0 for timer A2 (CCR0). Used in low power ACLK delay routine.
//
////////////////////////////////////////////////////////////////////////////
#pragma vector=TIMER2_A0_VECTOR //TCCR0 Interrupt Vector for TIMER A1
__interrupt void INT_Timer2A0(void) {
    //Timer interrupt callback
    wio_timer_callback();

    __bis_SR_register_on_exit(LPM3_bits);
}
