#include <syscalls_lib.h>
#include <keyboardDriver.h>
#include "include/videoDriver.h"
#include <audioDriver.h>
#include <rtc.h>
#include <stddef.h>
#include <interrupts.h>
#include <time.h>
#include <scheduler.h>
#include "keystate.h"
#include "include/semaphore.h"
#include "include/pipe.h"
#include "include/process.h"

#define STDIN 0
#define STDOUT 1

extern uint64_t * _getSnapshot();

static void append_char(char **out, int *rem, int *written, char c) {
    if (*rem <= 1) {
        return;
    }
    **out = c;
    (*out)++;
    (*rem)--;
    (*written)++;
}

static void append_str(char **out, int *rem, int *written, const char *str) {
    while (str != NULL && *str != '\0') {
        append_char(out, rem, written, *str);
        str++;
    }
}

static void append_uint(char **out, int *rem, int *written, uint64_t value, uint64_t base) {
    char buffer[32];
    int idx = 0;

    if (value == 0) {
        append_char(out, rem, written, '0');
        return;
    }

    while (value > 0 && idx < (int)sizeof(buffer)) {
        uint64_t digit = value % base;
        buffer[idx++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
        value /= base;
    }

    while (idx > 0) {
        append_char(out, rem, written, buffer[--idx]);
    }
}

static void append_int(char **out, int *rem, int *written, int value) {
    if (value < 0) {
        append_char(out, rem, written, '-');
        append_uint(out, rem, written, (uint64_t)(-value), 10);
        return;
    }
    append_uint(out, rem, written, (uint64_t)value, 10);
}

uint64_t syscall_get_regs(uint64_t *dest) {
    const uint64_t *src = _getSnapshot();
    for (int i = 0; i < REGISTERS_CANT; i++)
        dest[i] = src[i];
    return 1;
}

uint64_t syscall_read(int fd, char *buffer, int count) {
    if (buffer == NULL || count <= 0 || fd < 0 || fd >= MAX_FDS){
        return 0;
    }

    PCB *pcb = scheduler_current();
    FileDescriptor *fdes = &pcb->fds[fd]; 

    if(fdes->type == FD_PIPE_READ){
        return pipe_read(fdes->pipe_id, buffer, count);
    }

    if(fdes->type == FD_STDIN){
        uint64_t read = 0;
        char c;

        while (read < count) {
            c = keyboard_read_getchar();
            if (c == 0){
                break;
            }

            buffer[read++] = c;
            if (c == '\n'){
                break;
            } 
        }

        return read;
    }

    return 0;
}


uint64_t syscall_write(int fd, const char * buffer, int count) {
    if (buffer == NULL || count <= 0 || fd < 0 || fd >= MAX_FDS) {
        return 0;
    }
    
    PCB *pcb = scheduler_current(); 
    FileDescriptor *fdes = &pcb->fds[fd];

    if(fdes->type == FD_PIPE_WRITE){
        return pipe_write(fdes->pipe_id, buffer, count);
    }

    if(fdes->type == FD_STDOUT){
        for (int i = 0; i < count; i++) {
            video_putChar(buffer[i], FOREGROUND_COLOR, BACKGROUND_COLOR);
        }
        
        return count;
    }
    
    return 0;
}


uint64_t syscall_getTime(uint64_t reg) {
    uint8_t t = getTime((uint8_t)reg);
    return (uint64_t)t;
}

uint64_t syscall_clearScreen() {
    video_clearScreen();
    return 1;
}

uint64_t syscall_beep(int frequency, int duration) {
    audio_play(frequency);
    syscall_sleep(duration);
    audio_stop();
    return 1;
}

uint64_t syscall_sleep(int duration) {

    const uint32_t HZ = 18;
    uint64_t start     = ticks_elapsed();
    uint64_t wait_tics = (duration * HZ + 999) / 1000;
    uint64_t target    = start + wait_tics;


    while (ticks_elapsed() < target) {
        _hlt();
    }
    return 0;
}

uint64_t syscall_setFontScale(int scale) {
    if (scale < 1 || scale > 5) {
        return 0; 
    }
    
    setFontScale(scale);
    return 1; 
}

uint64_t syscall_video_putPixel(uint64_t x, uint64_t y, uint64_t color, uint64_t unused1, uint64_t unused2) {
    video_putPixel((uint32_t)color, x, y);
    return 0;
}

uint64_t syscall_video_putChar(uint64_t c, uint64_t fg, uint64_t bg, uint64_t unused1, uint64_t unused2) {
    video_putChar((char)c, fg, bg);
    return 0;
}

uint64_t syscall_video_clearScreenColor(uint64_t color, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    video_clearScreenColor((uint32_t)color);
    return 0;
}

uint64_t syscall_video_putCharXY(uint64_t c, uint64_t x, uint64_t y, uint64_t fg, uint64_t bg) {
    video_putCharXY((char)c, (int)x, (int)y, (uint32_t)fg, (uint32_t)bg);
    return 0;
}

uint64_t syscall_is_key_pressed(uint64_t scancode) {
    return (uint64_t)is_key_pressed((uint8_t)scancode);
}

uint64_t syscall_shutdown(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    outw(0x604, 0x2000);
    
    _cli();
    
    while(1) {
        _hlt();
    }
    
    return 0; 
}

uint64_t syscall_get_screen_dimensions(uint64_t *width, uint64_t *height) {
    if (width == NULL || height == NULL) {
        return 0;
    }
    *width = video_get_width();
    *height = video_get_height();
    return 1;
}

// SCHEDULER

uint64_t syscall_create_process(uint64_t entry, uint64_t name, uint64_t priority, uint64_t fg, uint64_t argc, uint64_t argv) {
    return (uint64_t)scheduler_create((void *)entry, (const char *)name, (int)priority, (int)fg, (int)argc, (char **)argv);
}

uint64_t syscall_kill(uint64_t pid) {
    return (uint64_t)scheduler_kill((pid_t)pid);
}

uint64_t syscall_getpid(void) {
    return (uint64_t)scheduler_getpid();
}

uint64_t syscall_yield(void) {
    scheduler_yield();
    return 0;
}

uint64_t syscall_block(uint64_t pid) {
    scheduler_block((pid_t)pid);
    return 0;
}
uint64_t syscall_nice(uint64_t pid, uint64_t new_priority) {
    return (uint64_t)scheduler_nice((pid_t)pid, (int)new_priority);
}

uint64_t syscall_ps(uint64_t buf, uint64_t max_len) {
    PCB table[MAX_PROCESSES];
    int count = scheduler_list(table, MAX_PROCESSES);

    char *out = (char *)buf;
    int written = 0;
    int rem = (int)max_len;

    for (int i = 0; i < count && rem > 1; i++) {
        append_str(&out, &rem, &written, table[i].name);
        append_str(&out, &rem, &written, " PID=");
        append_int(&out, &rem, &written, table[i].pid);
        append_str(&out, &rem, &written, " PRI=");
        append_int(&out, &rem, &written, table[i].priority);
        append_str(&out, &rem, &written, " FG=");
        append_int(&out, &rem, &written, table[i].foreground);
        append_str(&out, &rem, &written, " RSP=0x");
        append_uint(&out, &rem, &written, table[i].rsp, 16);
        append_str(&out, &rem, &written, " BASE=0x");
        append_uint(&out, &rem, &written, (uint64_t)table[i].stack_base, 16);
        append_str(&out, &rem, &written, " STATE=");
        append_int(&out, &rem, &written, table[i].state);
        append_char(&out, &rem, &written, '\n');
    }
    if (rem > 0) {
        *out = '\0';
    }
    return (uint64_t)written;
}

uint64_t syscall_unblock(uint64_t pid) {
    scheduler_unblock((pid_t)pid);
    return 0;
}

// SEMAPHORES

uint64_t syscall_sem_open(uint64_t name, uint64_t initial_value) {
    return (uint64_t) sem_open((const char *)name, (int)initial_value);
}

uint64_t syscall_sem_wait(uint64_t sem_id) {
    return (uint64_t) sem_wait((int)sem_id);
}

uint64_t syscall_sem_post(uint64_t sem_id) {
    return (uint64_t) sem_post((int)sem_id);
}

uint64_t syscall_sem_close(uint64_t sem_id) {
    return (uint64_t) sem_close((int)sem_id);
}

// PIPES

uint64_t syscall_pipe_open(void) {
    return pipe_open();
}

uint64_t syscall_pipe_close(uint64_t pipe_id, uint64_t is_write) {
    return pipe_close((int)pipe_id, (int)is_write);
}

uint64_t syscall_pipe_set_fd(uint64_t pid, uint64_t fd, uint64_t pipe_id, uint64_t is_write) {
    PCB *pcb = get_process_by_pid((int)pid);
    if (pcb == NULL) return -1;
    if (fd >= MAX_FDS) return -1;

    pcb->fds[fd].type    = is_write ? FD_PIPE_WRITE : FD_PIPE_READ;
    pcb->fds[fd].pipe_id = (int)pipe_id;
    return 0;
}
