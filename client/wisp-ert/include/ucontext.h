#pragma once

#include <stdint.h>
#include <signal.h>

//Get user context marco
#define getcontext(ctx) (getmcontext(&(ctx)->uc_mcontext))
//Set user context marco
#define setcontext(ctx) (setmcontext(&(ctx)->uc_mcontext))

//ERT machine context type
typedef struct mcontext {
    //Program counter
    uint16_t pc;
    //Stack pointer
    uint16_t sp;

    //General purpose registers
    uint16_t r4;
    uint16_t r5;
    uint16_t r6;
    uint16_t r7;
    uint16_t r8;
    uint16_t r9;
    uint16_t r10;
    uint16_t r11;
    uint16_t r12;
    uint16_t r13;
    uint16_t r14;
    uint16_t r15;
} mcontext_t;

//ERT user context type
typedef struct ucontext {
    //Stack used by this context
    stack_t uc_stack;
    //Machine context
    mcontext_t uc_mcontext;
} ucontext_t;

/**
 * Get machine context.
 *
 * @param ctx Machine context
 * @return 0 when context is saved, 1 when context is restored
 */
extern int getmcontext(
    mcontext_t* ctx
);

/**
 * Set machine context.
 *
 * @param ctx Machine context
 */
extern int setmcontext(
    const mcontext_t* ctx
);

/**
 * Make user context.
 *
 * @param ctx User context
 * @param func Function to run
 * @param argc Number of arguments
 */
extern void makecontext(
    ucontext_t* ctx,
    void (*func)(),
    int argc,
    ...
);

/**
 * Swap user context.
 *
 * @param octx Old user context
 * @param ctx User context
 */
extern int swapcontext(
    ucontext_t* octx,
    const ucontext_t* ctx
);
