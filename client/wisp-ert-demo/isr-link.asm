; This file assigns ISR vector targets. This must be done in the client
;  application so linker will observe these assignments, otherwise ISRs
;  will not be linked.

    .cdecls C,LIST, "wisp-base.h"
    .cdecls C,LIST, "wio/timer.h"

	.sect	".int45"			    ; Timer0_A0 Vector
	.short  Timer0A0_ISR			; int53 = Timer0A0_ISR addr.

	.sect	".int44"			    ; Timer0_A1 Vector
	.short  Timer0A1_ISR			; int52 = Timer0A1_ISR addr.

	.sect ".int36"					; Port 2 Vector
	.short  RX_ISR					; int02 = RX_ISR addr.
