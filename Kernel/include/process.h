#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

// process states
#define PROCESS_READY    0
#define PROCESS_RUNNING  1
#define PROCESS_BLOCKED  2
#define PROCESS_DEAD     3

// syscall numbers for process management
#define SYS_CREATE  18
#define SYS_KILL    19
#define SYS_GETPID  20
#define SYS_YIELD   21
#define SYS_BLOCK   22
#define SYS_NICE    23
#define SYS_PS      24

// process control block
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8,  r9,  r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t cs,  ss;
    int      pid;
    int      priority;
    int      state;
    char     name[32];
    void    *stack_base;
} PCB;

#endif