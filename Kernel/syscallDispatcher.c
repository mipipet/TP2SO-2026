#include <syscalls_lib.h>
#include <memManager.h>
#include <process.h>
#include <scheduler.h>

typedef uint64_t (*SyscallHandler)(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t r10, uint64_t r8, uint64_t r9);


// Wrappers for userland usage
uint64_t syscall_mem_alloc(uint64_t rdi) {
    return (uint64_t) mm_alloc(rdi);
}

uint64_t syscall_mem_free(uint64_t rdi) {
    mm_free((void *) rdi);
    return 0;
}

uint64_t syscall_mem_info(uint64_t rdi, uint64_t rsi, uint64_t rdx) {
    mm_info((uint64_t *) rdi, (uint64_t *) rsi, (uint64_t *) rdx);
    return 0;
}

static SyscallHandler syscallHandlers[] = {
    (SyscallHandler)syscall_read,   
    (SyscallHandler)syscall_write,  
    (SyscallHandler)syscall_getTime,
    (SyscallHandler)syscall_clearScreen, 
    (SyscallHandler)syscall_beep, 
    (SyscallHandler)syscall_sleep,
    (SyscallHandler)syscall_setFontScale,
    (SyscallHandler)syscall_video_clearScreenColor, 
    (SyscallHandler)syscall_video_putPixel,
    (SyscallHandler)syscall_video_putChar, 
    (SyscallHandler)syscall_video_putCharXY, 
    (SyscallHandler)syscall_get_regs,
    (SyscallHandler)syscall_is_key_pressed,
    (SyscallHandler)syscall_shutdown,
    (SyscallHandler)syscall_get_screen_dimensions,
    (SyscallHandler)syscall_mem_alloc,
    (SyscallHandler)syscall_mem_free,
    (SyscallHandler)syscall_mem_info,
    (SyscallHandler)syscall_create_process,         
    (SyscallHandler)syscall_kill,                  
    (SyscallHandler)syscall_getpid,                 
    (SyscallHandler)syscall_yield,                 
    (SyscallHandler)syscall_block,              
    (SyscallHandler)syscall_nice,                 
    (SyscallHandler)syscall_ps, 
    (SyscallHandler)syscall_unblock,
    (SyscallHandler)syscall_sem_open,
    (SyscallHandler)syscall_sem_wait,
    (SyscallHandler)syscall_sem_post,
    (SyscallHandler)syscall_sem_close,
    (SyscallHandler)syscall_pipe_open,   
    (SyscallHandler)syscall_pipe_close, 
    (SyscallHandler)syscall_pipe_set_fd, 
    (SyscallHandler)syscall_wait,
    (SyscallHandler)syscall_exit,
};

#define SYSCALLS_COUNT (sizeof(syscallHandlers) / sizeof(syscallHandlers[0]))

uint64_t syscallDispatcher(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t r10, uint64_t r8, uint64_t r9, uint64_t rax) {
    if (rax >= SYSCALLS_COUNT) {
        return 0;
    }
    return syscallHandlers[rax](rdi, rsi, rdx, r10, r8, r9);
}
