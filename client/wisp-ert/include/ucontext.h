#pragma once

#include <signal.h>

//ERT machine context type
typedef struct mcontext {
    //TODO: MSP430 registers here
} mcontext_t;

//ERT user context type
typedef struct ucontext {
    //Stack used by this context
    stack_t uc_stack;
    //Machine context
    mcontext_t uc_mcontext;
} ucontext_t;

/**
 * Get user context.
 *
 * @param ctx Machine context
 */
extern int getcontext(
    ucontext_t* ctx
);

/**
 * Make user context.
 *
 * @param ctx Machine context
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
 * @param octx Old machine context
 * @param ctx Machine context
 */
extern int swapcontext(
    ucontext_t* octx,
    ucontext_t* ctx
);
