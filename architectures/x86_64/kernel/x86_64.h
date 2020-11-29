#pragma once

#include "architectures/x86/kernel/x86.h"

static inline uint64_t RBP()
{
    uint64_t r;
    asm volatile("mov %%rbp, %0"
                 : "=r"(r));
    return r;
}
