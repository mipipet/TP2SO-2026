#ifndef PROCESS_SYSCALL_H
#define PROCESS_SYSCALL_H

#include <stdint.h>

typedef uint64_t (*process_func)(uint64_t argc, char **argv);

uint64_t sys_create(process_func func, const char *name, int priority, int fg, int argc, char **argv); // creates a new process
uint64_t sys_kill(int pid); // kills a process
uint64_t sys_getpid(void); // returns process pid
uint64_t sys_yield(void); // the process gives control to the CPU
uint64_t sys_block(int pid); // blocks a process
uint64_t sys_nice(int pid, int newPriority); // it change the priority of a process
uint64_t sys_ps(char *buffer,uint64_t max_len); // lists all processes and their status
uint64_t sys_unblock(int pid); // unblocks a process
int sys_pipe_open(void);
int sys_pipe_open_capacity(int capacity);
int sys_pipe_close(int pipe_id, int is_write);
int sys_pipe_set_fd(int pid, int fd, int pipe_id, int is_write);
int sys_wait(int pid); // waits for a child process
void sys_exit(int status); // exits current process

#endif
