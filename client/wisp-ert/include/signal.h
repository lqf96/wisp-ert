#pragma once

#include <stddef.h>

/// MSP430 stack type
typedef struct stack {
    /// Stack base pointer
    void* ss_sp;
    /// Stack size
    size_t ss_size;
} stack_t;
