#include <stdint.h>
#include <stdio.h>
#include "../include/process_syscalls.h"
#include "test_util.h"

#define TOTAL_PROCESSES 3

// Ajustar segun los valores de prioridad validos en el scheduler
#define LOWEST  1
#define MEDIUM  3
#define HIGHEST 5

static int64_t prio[TOTAL_PROCESSES] = {LOWEST, MEDIUM, HIGHEST};

static uint64_t max_value = 0;

// Proceso que cuenta de 0 a max_value y termina
static uint64_t zero_to_max(uint64_t argc, char *argv[]) {
    uint64_t value = 0;
    while (value++ != max_value)
        ;
    printf("PROCESS %d DONE!\n", (int)sys_getpid());
    sys_exit(0);
    return 0;
}

uint64_t test_prio(uint64_t argc, char *argv[]) {
    int64_t pids[TOTAL_PROCESSES];
    char *ztm_argv[] = {NULL};
    uint64_t i;

    if (argc != 1) {
        printf("Uso: test_prio <max_value>\n");
        return -1;
    }

    if ((max_value = satoi(argv[0])) <= 0)
        return -1;

    // --- Fase 1: misma prioridad ---
    printf("SAME PRIORITY...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++)
        pids[i] = sys_create((process_func)zero_to_max, "zero_to_max", 3, 0, 0, ztm_argv);

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_wait(pids[i]);

    // --- Fase 2: prioridades distintas, se cambian despues de crear ---
    printf("SAME PRIORITY, THEN CHANGE IT...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        pids[i] = sys_create((process_func)zero_to_max, "zero_to_max", 3, 0, 0, ztm_argv);
        sys_nice(pids[i], prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_wait(pids[i]);

    // --- Fase 3: bloqueados, se les cambia prioridad, luego se desbloquean ---
    printf("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...\n");

    for (i = 0; i < TOTAL_PROCESSES; i++) {
        pids[i] = sys_create((process_func)zero_to_max, "zero_to_max", 3, 0, 0, ztm_argv);
        sys_block(pids[i]);
        sys_nice(pids[i], prio[i]);
        printf("  PROCESS %d NEW PRIORITY: %d\n", (int)pids[i], (int)prio[i]);
    }

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_unblock(pids[i]);

    for (i = 0; i < TOTAL_PROCESSES; i++)
        sys_wait(pids[i]);

    return 0;
}
