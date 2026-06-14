#include "../include/syscall.h"
#include "test_util.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define MAX_BLOCKS 128

typedef struct MM_rq {
    void *address;
    uint32_t size;
} mm_rq;

// Wrappers para que el test use malloc/free via syscalls del kernel
static void *my_malloc(uint64_t size) {
    return sys_mem_alloc(size);
}

static void my_free(void *ptr) {
    sys_mem_free(ptr);
}

uint64_t test_mm(uint64_t argc, char *argv[]) {
    mm_rq mm_rqs[MAX_BLOCKS];
    uint8_t rq;
    uint32_t total;
    uint64_t max_memory;

    if (argc != 1) {
        printf("Uso: test_mm <max_memory_bytes>\n");
        return -1;
    }

    if ((max_memory = satoi(argv[0])) <= 0)
        return -1;

    while (1) {
        rq = 0;
        total = 0;

        // Pedir la mayor cantidad de bloques posible
        while (rq < MAX_BLOCKS && total < max_memory) {
            mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
            mm_rqs[rq].address = my_malloc(mm_rqs[rq].size);

            if (mm_rqs[rq].address) {
                total += mm_rqs[rq].size;
                rq++;
            } else {
                break;
            }
        }

        // Setear cada bloque con un patron unico
        uint32_t i;
        for (i = 0; i < rq; i++)
            if (mm_rqs[i].address)
                memset(mm_rqs[i].address, i, mm_rqs[i].size);

        // Verificar que no se solapan
        for (i = 0; i < rq; i++)
            if (mm_rqs[i].address)
                if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
                    printf("test_mm ERROR: solapamiento detectado en bloque %d\n", i);
                    return -1;
                }

        // Liberar todo
        for (i = 0; i < rq; i++)
            if (mm_rqs[i].address)
                my_free(mm_rqs[i].address);
    }

    return 0;
}