#ifndef PROCESS_SYSCALL_H
#define PROCESS_SYSCALL_H

#include <stdint.h>

typedef void (*process_func)(int argc, char **argv);

uint64_t sys_create(const char *name, process_func func, int priority, int argc, char **argv); // creates a new process
uint64_t sys_kill(int pid); // kills a process
uint64_t sys_getpid(void); // returns process pid
uint64_t sys_yield(void); // the process gives control to the CPU
uint64_t sys_block(int pid); // blocks a process
uint64_t sys_nice(int pid, int newPriority); // it change the priority of a process
uint64_t sys_ps(char *buffer); // lists all processes and their status

#endif