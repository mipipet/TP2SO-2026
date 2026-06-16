#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H

#include <stdint.h>

#define MM_KIND_FREE_LIST 0
#define MM_KIND_BUDDY 1

// Called once at boot: tells the allocator where the heap starts and how big it is
void mm_init(void *heapStart, uint64_t heapSize);

// Allocates 'size' bytes and returns a pointer to the block, or NULL if out of memory
void * mm_alloc(uint64_t size);

// Frees a previously allocated block given its pointer
void mm_free(void *ptr);

// Returns heap stats: total size, bytes in use, and bytes available
void mm_info(uint64_t *total, uint64_t *used, uint64_t *free_bytes);

// Returns the memory manager implementation compiled into the kernel.
int mm_kind(void);

#endif
