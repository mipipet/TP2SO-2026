#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include "syscall.h"

static inline void *malloc(uint64_t size) {
    return sys_mem_alloc(size);
}

static inline void free(void *ptr) {
    sys_mem_free(ptr);
}

#endif
