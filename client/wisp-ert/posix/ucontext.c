#include <ucontext.h>

/**
 * {@inheritDoc}
 */
void makecontext(
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
int swapcontext(
    ucontext_t* octx,
    const ucontext_t* ctx
) {
    if (getcontext(octx)==0)
        setcontext(ctx);
    return 0;
}
