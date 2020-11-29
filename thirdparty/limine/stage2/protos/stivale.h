#ifndef __PROTOS__STIVALE_H__
#define __PROTOS__STIVALE_H__

#include <stdbool.h>
#include <stdint.h>
#include <sys/e820.h>
#include <mm/vmm.h>

void stivale_load(char *cmdline);

pagemap_t stivale_build_pagemap(bool level5pg);
__attribute__((noreturn)) void stivale_spinup(
                 int bits, bool level5pg, pagemap_t pagemap,
                 uint64_t entry_point, void *stivale_struct, uint64_t stack);

#endif
