#include "include/scheduler.h"
#include "include/process.h"
#include "include/keyboardDriver.h"
#include "include/pipe.h"
#include <memManager.h>
#include <lib.h>
#include <stddef.h>

static PCB process_table[MAX_PROCESSES];
static int process_count = 0;
static int current_idx = -1;
static int next_pid = 1;
static uint8_t idle_stack[STACK_SIZE];

extern void _hlt(void);

void scheduler_exit_current(int status);

static void idle_process(void){
    while(1){
        _hlt();
    }
}

static void process_exit_trampoline(void) {
    scheduler_exit_current(0);
    while (1) {
        _hlt();
    }
}

static void copy_name(char *dst, const char *src) {
    int i = 0;
    while (i < 31 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void setup_initial_stack(PCB *pcb, void *entry, int argc, char **argv) {
    uint64_t *sp = (uint64_t *)((uint8_t *)pcb->stack_base + STACK_SIZE);
    uint64_t *return_slot = sp - 1;
    *return_slot = (uint64_t)process_exit_trampoline;
    uint64_t initial_rsp = (uint64_t)return_slot;

    *(--sp) = 0x00;                    // SS 
    *(--sp) = initial_rsp;             // RSP - contains the process return address
    *(--sp) = 0x202;                   // RFLAGS - interrupts enabled 
    *(--sp) = 0x08;                    // CS - kernel code segment
    *(--sp) = (uint64_t)entry;         // RIP - entry point

    *(--sp) = 0;  // rax 
    *(--sp) = 0;  // rbx 
    *(--sp) = 0;  // rcx 
    *(--sp) = 0;  // rdx 
    *(--sp) = 0;  // rbp 
    *(--sp) = (uint64_t)argc;  // rdi
    *(--sp) = (uint64_t)argv;  // rsi 
    *(--sp) = 0;  // r8 
    *(--sp) = 0;  // r9  
    *(--sp) = 0;  // r10 
    *(--sp) = 0;  // r11 
    *(--sp) = 0;  // r12 
    *(--sp) = 0;  // r13 
    *(--sp) = 0;  // r14 
    *(--sp) = 0;  // r15 

    pcb->rsp = (uint64_t)sp;
}

static int find_free_slot(void) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_DEAD) {
            return i;
        }
    }
    return -1;
}

static int find_next_ready(void) {
    int start = (current_idx + 1) % MAX_PROCESSES;

    for (int checked = 0; checked < MAX_PROCESSES; checked++) {
        int idx = (start + checked) % MAX_PROCESSES;
        if (process_table[idx].state == PROCESS_READY) {
            return idx;
        }
    }

    return 0;
}

static int find_index_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD) {
            return i;
        }
    }

    return -1;
}

static void restore_parent_foreground(PCB *process) {
    if (!process->foreground) {
        return;
    }

    int parent_idx = find_index_by_pid(process->parent_pid);
    if (parent_idx >= 0 && process_table[parent_idx].state != PROCESS_ZOMBIE) {
        process_table[parent_idx].foreground = 1;
        keyboard_clear_buffer();
    }

    process->foreground = 0;
}

static void close_process_fds(PCB *process) {
    for (int fd = 0; fd < MAX_FDS; fd++) {
        if (process->fds[fd].type == FD_PIPE_READ) {
            pipe_close(process->fds[fd].pipe_id, 0);
        } else if (process->fds[fd].type == FD_PIPE_WRITE) {
            pipe_close(process->fds[fd].pipe_id, 1);
        }

        process->fds[fd].type = FD_NONE;
        process->fds[fd].pipe_id = -1;
    }
}

void scheduler_init(void) {
    memset(process_table, 0, sizeof(process_table));

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_DEAD;
    }

    PCB * idle = &process_table[0]; 
    idle->pid = 0; 
    idle->parent_pid = 0; 
    idle->state = PROCESS_READY; 
    idle->priority = 1; 
    idle->quantums_left = 1; 
    idle->stack_base = idle_stack; 
    idle->foreground = 0; 
    idle->waiting_for = -1;
    idle->exit_status = 0;
    copy_name(idle->name, "idle");

    setup_initial_stack(idle, idle_process, 0, NULL);

    process_count = 1;
    current_idx = 0;
    process_table[0].state = PROCESS_RUNNING;
}

pid_t scheduler_create(void *entry, const char *name, int priority, int fg, int argc, char **argv) {
    if(priority <= 0 || priority > 5){
        priority = 3;
    }

    int slot = find_free_slot();
    if(slot < 0){
        return -1;
    }

    uint8_t *stack = (uint8_t *)mm_alloc(STACK_SIZE);
    if (stack == NULL) {
        return -1;
    }

    PCB * p = &process_table[slot]; 
    p->pid = next_pid++; 
    p->parent_pid = (current_idx >= 0) ? process_table[current_idx].pid : 0; 
    p->state = PROCESS_READY; 
    p->priority = priority; 
    p->quantums_left = priority; 
    p->stack_base = stack; 
    p->foreground = fg; 
    p->waiting_for = -1;
    p->exit_status = 0;
    copy_name(p->name, name);
    setup_initial_stack(p, entry, argc, argv);

    if (fg && current_idx >= 0) {
        process_table[current_idx].foreground = 0;
    }

    // initialize fds
    p->fds[0].type = FD_STDIN; 
    p->fds[0].pipe_id = -1; 
    p->fds[1].type = FD_STDOUT; 
    p->fds[1].pipe_id = -1; 

    for(int i = 2 ; i< MAX_FDS ; i++){
        p->fds[i].type = FD_NONE; 
        p->fds[i].pipe_id = -1; 
    }

    process_count++;
    return p->pid;
}

int scheduler_kill(pid_t pid) {
    if (pid == 0) {
        return -1; 
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD){
            if (i == current_idx) {
                scheduler_exit_current(-1);
                return 0;
            }

            restore_parent_foreground(&process_table[i]);
            close_process_fds(&process_table[i]);
            process_table[i].state = PROCESS_DEAD;
            process_table[i].quantums_left = 0;
            process_table[i].waiting_for = -1;

            if(process_table[i].stack_base != idle_stack){
                mm_free(process_table[i].stack_base);
                process_table[i].stack_base = NULL;
            }

            process_count--;

            for (int j = 0; j < MAX_PROCESSES; j++) {
                if (process_table[j].state == PROCESS_BLOCKED &&
                    process_table[j].waiting_for == pid) {
                    process_table[j].waiting_for = -1;
                    process_table[j].state = PROCESS_READY;
                }
            }

            return 0;
        }
    }

    return -1;
}

int scheduler_kill_foreground(void) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_DEAD &&
            process_table[i].foreground &&
            process_table[i].pid > 1) {
            return scheduler_kill(process_table[i].pid);
        }
    }

    return -1;
}

void scheduler_block_current(void) {
    if (current_idx < 0) {
        return;
    }
    process_table[current_idx].state = PROCESS_BLOCKED;
}
void scheduler_block(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD) {
            process_table[i].state = PROCESS_BLOCKED;

            if (i == current_idx) {
                process_table[i].quantums_left = 0;
            }
            return;
        }
    }
}

void scheduler_unblock(pid_t pid){
    for(int i = 0 ; i < MAX_PROCESSES ; i++){
        if(process_table[i].pid == pid && process_table[i].state == PROCESS_BLOCKED){
            process_table[i].state = PROCESS_READY;
            return;
        }
    }
}

int scheduler_nice(pid_t pid, int new_priority){
    if( new_priority <= 0 || new_priority > 5){
        return -1;
    }

    for(int i = 0 ; i < MAX_PROCESSES ; i++){
        if(process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD){
            process_table[i].priority = new_priority;

            if(process_table[i].quantums_left > new_priority){
                process_table[i].quantums_left = new_priority;
            }

            return 0;
        }
    }

    return -1;
}

pid_t scheduler_getpid(void){
    if (current_idx < 0){
        return -1;
    }
    return process_table[current_idx].pid;
}

PCB *scheduler_current(void){
    if(current_idx < 0){
        return NULL;
    }
    return &process_table[current_idx];
}

PCB *get_process_by_pid(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_DEAD &&
            process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

static void reap_process(int idx) {
    if (idx <= 0 || idx >= MAX_PROCESSES) {
        return;
    }

    if (process_table[idx].stack_base != NULL &&
        process_table[idx].stack_base != idle_stack) {
        mm_free(process_table[idx].stack_base);
    }

    process_table[idx].pid = 0;
    process_table[idx].parent_pid = 0;
    process_table[idx].state = PROCESS_DEAD;
    process_table[idx].priority = 0;
    process_table[idx].quantums_left = 0;
    process_table[idx].rsp = 0;
    process_table[idx].stack_base = NULL;
    process_table[idx].foreground = 0;
    process_table[idx].waiting_for = -1;
    process_table[idx].exit_status = 0;

    for (int fd = 0; fd < MAX_FDS; fd++) {
        process_table[idx].fds[fd].type = FD_NONE;
        process_table[idx].fds[fd].pipe_id = -1;
    }
}

int scheduler_wait(pid_t pid) {
    PCB *current = scheduler_current();
    if (current == NULL || pid <= 0) {
        return -1;
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD) {
            if (process_table[i].parent_pid != current->pid) {
                return -1;
            }

            if (process_table[i].state == PROCESS_ZOMBIE) {
                int status = process_table[i].exit_status;
                reap_process(i);
                process_count--;
                return status;
            }

            current->waiting_for = pid;
            current->state = PROCESS_BLOCKED;
            current->quantums_left = 0;
            return SCHEDULER_WAIT_BLOCKED;
        }
    }

    return -1;
}

void scheduler_exit_current(int status) {
    if (current_idx <= 0) {
        return;
    }

    PCB *current = &process_table[current_idx];
    int finished_was_foreground = current->foreground;

    current->exit_status = status;
    restore_parent_foreground(current);
    close_process_fds(current);
    current->state = PROCESS_ZOMBIE;
    current->quantums_left = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_BLOCKED &&
            process_table[i].waiting_for == current->pid) {
            process_table[i].waiting_for = -1;
            if (finished_was_foreground) {
                process_table[i].foreground = 1;
                keyboard_clear_buffer();
            }
            process_table[i].state = PROCESS_READY;
        }
    }
}

uint64_t scheduler_tick(uint64_t current_rsp){
    if(current_idx >= 0){
        process_table[current_idx].rsp = current_rsp;

        if(process_table[current_idx].state == PROCESS_RUNNING){
            process_table[current_idx].quantums_left--;

            if(process_table[current_idx].quantums_left > 0){
                return current_rsp;
            }

            process_table[current_idx].state = PROCESS_READY;
            process_table[current_idx].quantums_left = process_table[current_idx].priority;
        }
    }

    int next = find_next_ready();

    process_table[next].state = PROCESS_RUNNING;
    current_idx = next;

    return process_table[next].rsp;
}

int scheduler_list(PCB *out_buf, int max){
    int count = 0;
    for(int i =0 ; i < MAX_PROCESSES && count < max ; i++){
        if(process_table[i].state != PROCESS_DEAD){
            out_buf[count++] = process_table[i];
        }
    }
    return count;
}

void scheduler_yield(void){
    if(current_idx < 0){
        return;
    }
    process_table[current_idx].quantums_left = 0;
}
