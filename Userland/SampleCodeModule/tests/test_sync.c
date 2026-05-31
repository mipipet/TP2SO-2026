#include <stdint.h>
#include "../include/lib.h"
#include "../include/process_syscalls.h"
#include "../include/semaphore_syscalls.h"
#include "test_util.h"

#define SEM_ID "sem"
#define TOTAL_PAIR_PROCESSES 2

int64_t global; // shared memory

void slowInc(int64_t *p, int64_t inc) {
  uint64_t aux = *p;
  if (GetUniform(100) < 30)
    sys_yield(); // This makes the race condition highly probable
  aux += inc;
  *p = aux;
}

uint64_t process_inc(uint64_t argc, char *argv[]) {
  uint64_t n;
  int8_t inc;
  int8_t use_sem;
  sem_t sem_id = -1;

  if (argc != 3)
    return -1;

  if ((n = satoi(argv[0])) <= 0)
    return -1;
  if ((inc = satoi(argv[1])) == 0)
    return -1;
  if ((use_sem = satoi(argv[2])) < 0)
    return -1;

  if (use_sem) {
    sem_id = sys_sem_open(SEM_ID, 1);
    if (sem_id == -1) {
      printf("test_sync: ERROR opening semaphore\n");
      return -1;
    }
  }

  for (uint64_t i = 0; i < n; i++) {
    if (use_sem)
      sys_sem_wait(sem_id);
    slowInc(&global, inc);
    if (use_sem)
      sys_sem_post(sem_id);
  }

  if (use_sem)
    sys_sem_close(sem_id);

  return 0;
}

uint64_t test_sync(uint64_t argc, char *argv[]) {
  uint64_t pids[2 * TOTAL_PAIR_PROCESSES];

  if (argc != 2)
    return -1;

  char *argvDec[] = {argv[0], "-1", argv[1], NULL};
  char *argvInc[] = {argv[0], "1", argv[1], NULL};

  global = 0;

  for (uint64_t i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    pids[i] = sys_create(process_inc, "process_inc", 3, 0, 3, argvDec);
    pids[i + TOTAL_PAIR_PROCESSES] = sys_create(process_inc, "process_inc", 3, 0, 3, argvInc);
  }

  for (uint64_t i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
    sys_wait(pids[i]);
    sys_wait(pids[i + TOTAL_PAIR_PROCESSES]);
  }

  printf("Final value: %d\n", global);

  return 0;
}