#include <stdarg.h>
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
    //Variable arguments list
    va_list va_args;
    //Machine context
    mcontext_t* mctx = &ctx->uc_mcontext;
    //Stack pointer
    uint32_t* sp = ctx->uc_stack.ss_sp;

    va_start(va_args, argc);
    //Copy arguments to registers or to stack
    for (int i=0;i<argc;i++) {
        //Pointer or 32-bit argument
        uint32_t arg = va_arg(va_args, uint32_t);

        switch (i) {
            //R12
            case 0: {
                mctx->r12 = arg;
                break;
            }
            //R13
            case 1: {
                mctx->r13 = arg;
                break;
            }
            //R14
            case 2: {
                mctx->r14 = arg;
                break;
            }
            //R15
            case 3: {
                mctx->r15 = arg;
                break;
            }
            //Pass by stack
            default: {
                *sp = arg;
                //Update stack pointer
                sp++;

                break;
            }
        }
    }
    va_end(va_args);

    //Push return address onto stack
    *sp = 0;
    //Update stack pointer
    sp++;
    //Set stack pointer
    mctx->sp = (uint32_t)sp;

    //Set program counter
    mctx->pc = (uint32_t)func;
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
