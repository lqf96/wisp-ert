; Global symbols
	.global getmcontext, setmcontext

; Get machine context
getmcontext:
	; Save stack pointer
	MOV SP, 2(R12)
    ; Save general purpose registers
	MOV R4, 4(R12)
    MOV R5, 6(R12)
    MOV R6, 8(R12)
    MOV R7, 10(R12)
    MOV R8, 12(R12)
    MOV R9, 14(R12)
    MOV R10, 16(R12)
    MOV R11, 18(R12)
    MOV R13, 22(R12)
    MOV R14, 24(R12)
    MOV R15, 26(R12)
    ; Return value for machine context
    MOV #1, 20(R12)
    ; Program counter
    MOV (SP), R7
    MOV R7, (R12)
    ; Restore R7
    MOV 10(R12), R7
    ; Return value
    MOV #0, R12
    RETA

; Set machine context
setmcontext:
	; Restore stack pointer
	MOV 2(R12), SP
	; Push program counter onto stack
	PUSH (R12)
    ; Restore general purpose registers
    MOV 4(R12), R4
    MOV 6(R12), R5
    MOV 8(R12), R6
    MOV 10(R12), R7
    MOV 12(R12), R8
    MOV 14(R12), R9
    MOV 16(R12), R10
    MOV 18(R12), R11
    MOV 22(R12), R13
    MOV 24(R12), R14
    MOV 26(R12), R15
    ; Restore R12
    MOV 20(R12), R12
    ; Restore program counter
    RET

.end
