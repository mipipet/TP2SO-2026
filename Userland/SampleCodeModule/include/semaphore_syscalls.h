#ifndef SEMAPHORE_SYSCALL_H
#define SEMAPHORE_SYSCALL_H

#include <stdint.h>

typedef int sem_t;

sem_t sys_sem_open(const char *name, int initial_value);
int sys_sem_wait(sem_t id);
int sys_sem_post(sem_t id);
int sys_sem_close(sem_t id);
int sys_wait(uint64_t pid);

#endif