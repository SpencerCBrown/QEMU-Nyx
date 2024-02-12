#pragma once

#include <stdint.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE ((uint64_t) qemu_real_host_page_size)
#endif

// NOTE: test_and_set_bit operates on 8-byte words, so bitmap sizes must be multiples of 8
#define BITMAP_SIZE(x)      (((((x / PAGE_SIZE) >> 3) + 7) >> 3) << 3)
#define DIRTY_STACK_SIZE(x) ((x / PAGE_SIZE) * sizeof(uint64_t))


uint64_t get_ram_size(void);
