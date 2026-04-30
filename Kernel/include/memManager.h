#ifndef MEM_MANAGER_H
#define MEM_MANAGER_H

#include <stdint.h>

// Metadata stored at the beginning of every heap block.
// The actual data lives immediately after this header in memory.
typedef struct block {
    uint64_t size;       // size of the data region in bytes (not including this header)
    int is_free;         // 1 if available, 0 if allocated
    struct block *next;  // pointer to the next block in the list, NULL if last
} block_t;

// Called once at boot: tells the allocator where the heap starts and how big it is
void   mm_init(void *heapStart, uint64_t heapSize);

// Allocates 'size' bytes and returns a pointer to the block, or NULL if out of memory
void * mm_alloc(uint64_t size);

// Frees a previously allocated block given its pointer
void   mm_free(void *ptr);

// Returns heap stats: total size, bytes in use, and bytes available
void   mm_info(uint64_t *total, uint64_t *used, uint64_t *free);

#endif