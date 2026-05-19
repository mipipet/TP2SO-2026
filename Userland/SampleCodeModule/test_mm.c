#include "include/syscall.h"
#include "include/lib.h"

#define MAX_ALLOCS 100
#define MAX_SIZE   256

// Simple pseudo-random number generator
static uint64_t seed = 12345;
static uint64_t rand_next() {
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    return seed;
}

// Check that no two allocated blocks overlap
static int check_no_overlap(void **ptrs, uint64_t *sizes, int count) {
    for (int i = 0; i < count; i++) {
        if (ptrs[i] == 0) continue;
        char *start_i = (char *)ptrs[i];
        char *end_i   = start_i + sizes[i];

        for (int j = i + 1; j < count; j++) {
            if (ptrs[j] == 0) continue;
            char *start_j = (char *)ptrs[j];
            char *end_j   = start_j + sizes[j];

            // Overlap if one starts before the other ends
            if (start_i < end_j && start_j < end_i) {
                printf("OVERLAP detected between block %d and block %d!\n", i, j);
                return 0;
            }
        }
    }
    return 1;
}

void test_mm() {
    void    *ptrs[MAX_ALLOCS];
    uint64_t sizes[MAX_ALLOCS];
    int      iteration = 0;

    printf("test_mm: starting...\n");

    while (1) {
        // 1. Allocate MAX_ALLOCS blocks of random sizes
        for (int i = 0; i < MAX_ALLOCS; i++) {
            sizes[i] = (rand_next() % MAX_SIZE) + 1;
            ptrs[i]  = sys_mem_alloc(sizes[i]);
        }

        // 2. Check no overlaps
        if (check_no_overlap(ptrs, sizes, MAX_ALLOCS)) {
            printf("test_mm iteration %d: OK\n", iteration);
        }

        // 3. Free all blocks
        for (int i = 0; i < MAX_ALLOCS; i++) {
            if (ptrs[i] != 0) {
                sys_mem_free(ptrs[i]);
                ptrs[i] = 0;
            }
        }

        iteration++;
    }
}