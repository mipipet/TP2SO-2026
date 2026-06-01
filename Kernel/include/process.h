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
#define SYS_UNBLOCK 25
#define SYS_SEM_OPEN  26
#define SYS_SEM_WAIT  27
#define SYS_SEM_POST  28
#define SYS_SEM_CLOSE 29

#define MAX_PROCESSES  64
#define STACK_SIZE     (4 * 1024)   // 4 KB per process

#define MAX_FDS 8

typedef enum {
    FD_NONE,
    FD_STDIN,
    FD_STDOUT,
    FD_PIPE_READ,
    FD_PIPE_WRITE
} FDType;

typedef struct {
    FDType type;
    int pipe_id;   // index pipes table
} FileDescriptor;

// process control block
typedef struct {
    int       pid;
    int       parent_pid;       
    char      name[32];
    int       state;            
    int       priority;         
    int       quantums_left;    // countdown per turn
    uint64_t  rsp;              
    uint8_t  *stack_base;      
    int       foreground;       // 1 = owns keyboard
    FileDescriptor fds[MAX_FDS];
} PCB;

#endif