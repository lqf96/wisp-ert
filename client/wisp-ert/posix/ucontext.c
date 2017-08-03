#include <ucontext.h>

extern void getmcontext(mcontext_t* mctx);
extern void setmcontext(mcontext_t* mctx);

/**
 * {@inheritDoc}
 */
int getcontext(
    ucontext_t* ctx
) {
    //TODO: Get context

    return 0;
}

/**
 * {@inheritDoc}
 */
extern void makecontext(
    ucontext_t* ctx,
    void (*func)(),
    int argc,
    ...
) {
    //TODO: Make context
}

/**
 * {@inheritDoc}
 */
extern int swapcontext(
    ucontext_t* octx,
    ucontext_t* ctx
) {
    //TODO: Swap context

    return 0;
}
