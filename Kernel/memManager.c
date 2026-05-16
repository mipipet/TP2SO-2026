#include <memManager.h>
# include <stddef.h>

#define MIN_BLOCK_SIZE 8

// Global variables
static void * heap_start = NULL;
static uint64_t heap_size = 0;

static block_t * first_block = NULL;

// Called once at boot: tells the allocator where the heap starts and how big it is
void mm_init(void *heapStart, uint64_t heapSize){
    heap_start = heapStart; 
    heap_size = heapSize; 

    first_block = (block_t *)heapStart; 

    first_block->size = heapSize - sizeof(block_t); 
    first_block->is_free = 1; 
    first_block->next = NULL; 
}

// Allocates 'size' bytes and returns a pointer to the block, or NULL if out of memory
void * mm_alloc(uint64_t size){
    if(size == 0){
        return NULL; 
    }

    // Align memory to 8 bytes
    size = (size + 7) & ~7; 

    block_t * current = first_block; 

    while(current != NULL){

        // Finds big enough block
        if(current->is_free && current->size >= size){

            // Can the block be divided?
            if(current->size >= size + sizeof(block_t) + MIN_BLOCK_SIZE){

                // Create new free block after asigned space
                block_t * new_block = (block_t *)((char *)(current+1)+size); 

                new_block->size = current->size - size - sizeof(block_t); 
                new_block->is_free = 1; 
                new_block->next = current->next; 

                // Shrink current block - Update next in current
                current->size = size; 
                current->next = new_block;
            }

            current->is_free = 0; 
            return (void *)(current+1); // Return pointer to data
        }

        current = current->next; 
    }

    // Not enough space
    return NULL; 
}

// Frees a previously allocated block given its pointer
void mm_free(void *ptr){
    // Check not null pointer
    if(ptr == NULL){
        return; 
    }

    char *heap_end = (char *) heap_start + heap_size;

    // Check valid pointer
    if((char *) ptr < (char *) heap_start || (char *) ptr >= heap_end){
        return; 
    }

    block_t * block = ((block_t *)ptr) - 1; 

    if(block->is_free){
        return;
    }
    
    block->is_free = 1; 

    // Merge with next block if it is free -> here is the bug
    if(block->next && block->next->is_free){
        block->size += sizeof(block_t) + block->next->size; 
        block->next = block->next->next; 
    }
    // Marge also if the previous one is free
    block_t *prev = first_block;
    while (prev->next != NULL && prev->next != block) {
        prev = prev->next;
    }
    if (prev != block && prev->is_free) {
        prev->size += sizeof(block_t) + block->size;
        prev->next = block->next;
    }
}

// Returns heap stats: total size, bytes in use, and bytes available
void mm_info(uint64_t *total, uint64_t *used, uint64_t *free){
    *total = heap_size; 
    *used = 0; 
    *free = 0; 

    block_t * current = first_block; 

    while(current != NULL){
        if(current->is_free){
            *free += current->size;
        }else{
            *used += current->size; 
        }

        current = current->next; 
    }
}