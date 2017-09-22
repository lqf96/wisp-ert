#pragma once

#include <stdint.h>
#include <signal.h>

/// Get user context marco
#define getcontext(ctx) (getmcontext(&(ctx)->uc_mcontext))
/// Set user context marco
#define setcontext(ctx) (setmcontext(&(ctx)->uc_mcontext))

/// ERT machine context type
typedef struct mcontext {
    /// Program counter
    uint32_t pc;
    /// Stack pointer
    uint32_t sp;

    /// R4 register
    uint32_t r4;
    /// R5 register
    uint32_t r5;
    /// R6 register
    uint32_t r6;
    /// R7 register
    uint32_t r7;
    /// R8 register
    uint32_t r8;
    /// R9 register
    uint32_t r9;
    /// R10 register
    uint32_t r10;
    /// R11 register
    uint32_t r11;
    /// R12 register
    uint32_t r12;
    /// R13 register
    uint32_t r13;
    /// R14 register
    uint32_t r14;
    /// R15 register
    uint32_t r15;
} mcontext_t;

/// ERT user context type
typedef struct ucontext {
    /// Stack used by this context
    stack_t uc_stack;
    /// Machine context
    mcontext_t uc_mcontext;
} ucontext_t;

/**
 * @brief Get machine context.
 *
 * @param ctx Machine context.
 * @return 0 when context is saved, 1 when context is restored.
 */
extern int getmcontext(
    mcontext_t* ctx
);

/**
 * @brief Set machine context.
 *
 * @param ctx Machine context.
 * @return This function does not actually return.
 */
extern int setmcontext(
    const mcontext_t* ctx
);

/**
 * @brief Make user context.
 *
 * (TODO: Make context issues)
 *
 * @param ctx User context.
 * @param func Function to run.
 * @param argc Number of arguments.
 */
extern void makecontext(
    ucontext_t* ctx,
    void (*func)(),
    int argc,
    ...
);

/**
 * @brief Swap user context.
 *
 * @param octx Old user context to save.
 * @param ctx User context to restore.
 */
extern int swapcontext(
    ucontext_t* octx,
    const ucontext_t* ctx
);
