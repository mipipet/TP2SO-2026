#include <stddef.h>
#include <memManager.h>

#define MIN_ORDER 5 // smallest block 32 bytes
#define MAX_ORDER 20 // largest block 1 mega byte
#define NUM_ORDERS (MAX_ORDER - MIN_ORDER + 1)

typedef struct BlockHeader {
    uint64_t order;
} BlockHeader;

typedef struct FreeBlock {
    BlockHeader header;
    struct FreeBlock *next;
    struct FreeBlock *prev;
} FreeBlock;

static FreeBlock *free_lists[NUM_ORDERS];
static void     *heap_base = NULL;
static uint64_t  heap_size = 0;

static int list_index(int order) {
    return order - MIN_ORDER;
}

static void list_push(int order, FreeBlock * block){
    int idx = list_index(order);
    block->next = free_lists[idx]; 
    block->prev = NULL;

    if(free_lists[idx] != NULL) {
        free_lists[idx]->prev = block;
    }

    free_lists[idx] = block; 
}

static void list_remove(int order, FreeBlock * block){
    int idx = list_index(order); 
    if(free_lists[idx] != NULL){
        if(block->prev != NULL){
            block->prev->next = block->next; 
        }else{
            free_lists[idx] = block->next;  
        }
        if(block->next != NULL){
            block->next->prev = block->prev; 
        }
    }
    block->next = NULL;
    block->prev = NULL;
}

static int order_for_size(uint64_t size) {
    int order = MIN_ORDER;
    while ((1ULL << order) < size)
        order++;
    if (order > MAX_ORDER)
        return -1;
    return order;
}

// Called once at boot: tells the allocator where the heap starts and how big it is
void mm_init(void *heapStart, uint64_t heapSize){
    // Initialize
    heap_base = heapStart; 
    heap_size = heapSize; 

    for( int i=0 ; i < NUM_ORDERS ; i++){
        free_lists[i] = NULL; 
    }

    uint64_t remaining = heapSize; 
    uint64_t offset = 0; 

    for( int order = MAX_ORDER ; order >= MIN_ORDER && remaining > 0 ; order--){
        uint64_t block_size = (uint64_t)1 << order ; 
        while(remaining >= block_size){
            FreeBlock *block = (FreeBlock *)((uint8_t *)heapStart + offset); 
            block->header.order = order;
            list_push(order, block); 

            offset += block_size; 
            remaining -= block_size; 
        }
    }
}

// Allocates 'size' bytes and returns a pointer to the block, or NULL if out of memory
void * mm_alloc(uint64_t size){
    if(size == 0){
        return NULL;
    }

    int order = order_for_size(size + sizeof(BlockHeader)); 

    if(order == -1){
        return NULL; 
    }

    int found = -1; 
    for(int i=order ; i <= MAX_ORDER && found == -1 ; i++){
        if(free_lists[list_index(i)] != NULL){
            found = i; 
        }
    }

    if(found == -1){
        return NULL; 
    }

    FreeBlock * block = free_lists[list_index(found)]; 
    list_remove(found, block); 

    while(found > order){
        found--; 
        FreeBlock * second_half = (FreeBlock *)((uint8_t *)block + (1ULL << found));
        second_half->header.order = found;
        list_push(found, second_half); 
    }

    block->header.order = order;
    return (void *)((uint8_t *)block + sizeof(BlockHeader));
}

// Frees a previously allocated block given its pointer
void mm_free(void *ptr){
    if(ptr == NULL){
        return; 
    }

    FreeBlock * block = (FreeBlock *)((uint8_t *)ptr - sizeof(BlockHeader));
    int order = block->header.order;

    uint64_t block_size = (1ULL << order);

    int can_merge = 1; 

    while(order < MAX_ORDER && can_merge){
        uint64_t offset = (uint64_t)((uint8_t *)block - (uint8_t *)heap_base); 
        uint64_t buddy_offset = offset ^ block_size; 

        FreeBlock * buddy = (FreeBlock *)((uint8_t *)heap_base + buddy_offset); 
        FreeBlock * current = free_lists[list_index(order)]; 

        can_merge = 0; 

        while(current != NULL && !can_merge){
            if(current == buddy){
                list_remove(order, buddy);

                if(buddy < block){
                    block = buddy;
                }

                order++;
                block_size <<= 1;

                can_merge = 1;
            }
            current = current->next;
        }
    }
    
    block->header.order = order; 
    list_push(order,block);
}

// Returns heap stats: total size, bytes in use, and bytes available
void mm_info(uint64_t *total, uint64_t *used, uint64_t *free_bytes){
    *total = heap_size; 
    uint64_t available = 0; 

    for(int order = MIN_ORDER ; order <= MAX_ORDER; order++){
        FreeBlock * current = free_lists[list_index(order)]; 

        while(current != NULL){
            available += (1ULL << order); 
            current = current->next; 
        }
    }

    *free_bytes = available; 
    *used = heap_size - available; 
}

int mm_kind(void) {
    return MM_KIND_BUDDY;
}
