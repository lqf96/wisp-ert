#pragma once

#include "wio.h"

//ERT status type
typedef wio_status_t ert_status_t;

/**
 * ERT user code entry point.
 *
 * @param argc Number of arguments
 * @param argv Arguments
 */
extern ert_status_t ert_main(int argc, char** argv);

/**
 * Get machine context.
 */
extern ert_status_t ert_get_mcontext(ert_mcontext_t* ctx);

/**
 * Make machine context.
 */
extern ert_status_t ert_make_mcontext(ert_mcontext_t* ctx);

/**
 * Swap machine context.
 */
extern ert_status_t ert_set_mcontext(ert_mcontext_t* ctx, ert_mcontext_t* octx);

/**
 * Initialize function handles.
 */
extern ert_status_t ert_func_init();

/**
 * Initialize constants.
 */
extern ert_status_t ert_const_init();
