; Global symbols
    .global getmcontext, setmcontext

; Get machine context
getmcontext:
    ; Save stack pointer
    MOVX SP, 4(R12)
    ; Save general purpose registers
    MOVX R4, 8(R12)
    MOVX R5, 12(R12)
    MOVX R6, 16(R12)
    MOVX R7, 20(R12)
    MOVX R8, 24(R12)
    MOVX R9, 28(R12)
    MOVX R10, 32(R12)
    MOVX R11, 36(R12)
    MOVX R13, 44(R12)
    MOVX R14, 48(R12)
    MOVX R15, 52(R12)
    ; Return value for machine context
    MOVX #1, 40(R12)
    ; Program counter
    MOVX 0(SP), 0(R12)
    ; Return value
    MOVX #0, R12
    RETA

; Set machine context
setmcontext:
    ; Restore stack pointer
    MOVX 4(R12), SP
    ; Push program counter onto stack
    MOVX 0(R12), R7
    PUSHA R7
    ; Restore general purpose registers
    MOVX 8(R12), R4
    MOVX 12(R12), R5
    MOVX 16(R12), R6
    MOVX 20(R12), R7
    MOVX 24(R12), R8
    MOVX 28(R12), R9
    MOVX 32(R12), R10
    MOVX 36(R12), R11
    MOVX 44(R12), R13
    MOVX 48(R12), R14
    MOVX 52(R12), R15
    ; Restore R12
    MOVX 40(R12), R12
    ; Restore program counter
    RETA

.end
