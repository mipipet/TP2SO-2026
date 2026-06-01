#include "include/scheduler.h"
#include "include/process.h"
#include <memManager.h>
#include <lib.h>
#include <stddef.h>

static PCB process_table[MAX_PROCESSES];
static int process_count = 0;
static int current_idx = -1;
static int next_pid = 1;
static uint8_t idle_stack[STACK_SIZE];

extern void _hlt(void);

static void idle_process(void){
    while(1){
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
    uint64_t stack_top = (uint64_t)sp;

    *(--sp) = 0x00;                    // SS 
    *(--sp) = stack_top;               // RSP - points to top of stack
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
    copy_name(p->name, name);
    setup_initial_stack(p, entry, argc, argv);

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
            process_table[i].state = PROCESS_DEAD;
            process_table[i].quantums_left = 0;

            if(process_table[i].stack_base != idle_stack){
                mm_free(process_table[i].stack_base);
                process_table[i].stack_base = NULL;
            }

            process_count--;

            return 0;
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