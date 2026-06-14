#include <stdio.h>
#include <stdint.h>
#include "../include/process_syscalls.h"
#include "test_util.h"

enum State { RUNNING, BLOCKED, KILLED };

typedef struct P_rq {
    int32_t pid;
    enum State state;
} p_rq;

// Funcion dummy que corre en loop: debe estar registrada en shellCmds con el nombre "endless_loop"
static uint64_t endless_loop_entry(uint64_t argc, char *argv[]) {
    endless_loop();
    return 0;
}

uint64_t test_processes(uint64_t argc, char *argv[]) {
    uint8_t rq;
    uint8_t alive = 0;
    uint8_t action;
    uint64_t max_processes;
    char *argvAux[] = {NULL};

    if (argc != 1) {
        printf("Uso: test_proc <max_processes>\n");
        return -1;
    }

    if ((max_processes = satoi(argv[0])) <= 0)
        return -1;

    p_rq p_rqs[max_processes];

    while (1) {
        // Crear max_processes procesos dummy
        for (rq = 0; rq < max_processes; rq++) {
            p_rqs[rq].pid = sys_create(
                (process_func)endless_loop_entry,
                "endless_loop", 3, 0, 0, argvAux);

            if (p_rqs[rq].pid == -1) {
                printf("test_proc: ERROR creando proceso\n");
                return -1;
            } else {
                p_rqs[rq].state = RUNNING;
                alive++;
            }
        }

        // Aleatoriamente matar o bloquear hasta que todos esten muertos
        while (alive > 0) {
            for (rq = 0; rq < max_processes; rq++) {
                action = GetUniform(100) % 2;

                switch (action) {
                    case 0:
                        if (p_rqs[rq].state == RUNNING || p_rqs[rq].state == BLOCKED) {
                            if (sys_kill(p_rqs[rq].pid) == -1) {
                                printf("test_proc: ERROR matando proceso\n");
                                return -1;
                            }
                            p_rqs[rq].state = KILLED;
                            sys_wait(p_rqs[rq].pid);
                            alive--;
                        }
                        break;

                    case 1:
                        if (p_rqs[rq].state == RUNNING) {
                            if (sys_block(p_rqs[rq].pid) == -1) {
                                printf("test_proc: ERROR bloqueando proceso\n");
                                return -1;
                            }
                            p_rqs[rq].state = BLOCKED;
                        }
                        break;
                }
            }

            // Desbloquear aleatoriamente para evitar deadlock
            for (rq = 0; rq < max_processes; rq++)
                if (p_rqs[rq].state == BLOCKED && GetUniform(100) % 2) {
                    if (sys_unblock(p_rqs[rq].pid) == -1) {
                        printf("test_proc: ERROR desbloqueando proceso\n");
                        return -1;
                    }
                    p_rqs[rq].state = RUNNING;
                }
        }
    }

    return 0;
}