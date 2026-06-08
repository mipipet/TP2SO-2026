#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <stdint.h>

#define SCHEDULER_WAIT_BLOCKED -2

// Initializes scheduler
void scheduler_init(void);

// Creates a new process - state READY
pid_t scheduler_create(void *entry, const char *name, int priority, int fg, int argc, char **argv);

// Kills a process
int scheduler_kill(pid_t pid);

// Changes current process from RUNNING to BLOCKED
void scheduler_block_current(void);

// Changes a process state from READY, BLOCKED or RUNNING to BLOCKED
void scheduler_block(pid_t pid); 

// Changes a process state from BLOCKED to READY
void scheduler_unblock(pid_t pid);

// Changes priority of a process
int scheduler_nice(pid_t pid, int new_priority);

// Returns current process PID
pid_t scheduler_getpid(void);

// Returns pointer to current process's PID
PCB *scheduler_current(void);

// Main function called from timer handler irq00
uint64_t scheduler_tick(uint64_t current_rsp);

// Fills buffer with info on all processes
int scheduler_list(PCB *out_buf, int max);

// Forces current process to yield
void scheduler_yield(void);

// Returns current process's PCB
PCB *get_process_by_pid(int pid);

// Blocks current process until the child finishes, or reaps it if already done.
int scheduler_wait(pid_t pid);

// Marks the current process as finished and wakes any waiter.
void scheduler_exit_current(int status);

// Kills the foreground process if it is not the shell.
int scheduler_kill_foreground(void);

#endif
