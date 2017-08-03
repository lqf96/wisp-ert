#pragma once

#include <stddef.h>

//MSP430 stack type
typedef struct stack {
    //Stack base pointer
    void* ss_sp;
    //Stack size
    size_t ss_size;
    //Stack flags
    int ss_flags;
} stack_t;
