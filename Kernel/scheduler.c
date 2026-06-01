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

    *(--sp) = 0x00;                    // SS
    *(--sp) = (uint64_t)(sp + 1);      // RSP - points to top of stack
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

static int find_free_slot(void){
    for(int i=0 ; i < MAX_PROCESSES ; i++){
        if(process_table[i].state == PROCESS_DEAD || process_table[i].pid == 0){
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

// Initializes scheduler
void scheduler_init(void){
    memset(process_table, 0, sizeof(process_table));

    PCB * idle = &process_table[0]; 
    idle->pid = 0; 
    idle->parent_pid = 0; 
    idle->state = PROCESS_READY; 
    idle->priority = 1; 
    idle->quantums_left = 1; 
    idle->stack_base = idle_stack; 
    idle->foreground = 0; 

    copy_name(idle->name, "idle");
    process_count = 1; 

    setup_initial_stack(idle, idle_process, 0, NULL);
}

// Creates a new process - state READY
pid_t scheduler_create(void *entry, const char *name, int priority, int fg, int argc, char **argv){    if(priority <= 0 || priority > 5){
        priority = 3; // default
    }

    int slot = find_free_slot();
    if(slot < 0){
        return -1;
    }

    uint8_t *stack = (uint8_t *)mm_alloc(STACK_SIZE);
    if (stack == NULL) {
        return -1; // out of memory
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

// Kills a process
int scheduler_kill(pid_t pid){
    if(pid == 0){
        return -1; 
    }

    for(int i = 0 ; i < MAX_PROCESSES ; i++){
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
// No se usa esta funcion -> la dejo x si lo de abajo esta mal
// Changes current process from RUNNING to BLOCKED
void scheduler_block_current(void){
    if(current_idx < 0){
        return; 
    }

    process_table[current_idx].state = PROCESS_BLOCKED; 
}
// Ojo que hay que chequear que esto sea valido -> pregunte en el foro
// Changes a process state from READY, BLOCKED or RUNNING to BLOCKED
void scheduler_block(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD) {
            process_table[i].state = PROCESS_BLOCKED;
            return;
        }
    }
}

// Changes a process state from BLOCKED to READY
void scheduler_unblock(pid_t pid){
    for(int i = 0 ; i < MAX_PROCESSES ; i++){
        if(process_table[i].pid == pid && process_table[i].state == PROCESS_BLOCKED){
            process_table[i].state = PROCESS_READY; 
            return; 
        }
    }
}

// Changes priority of a process
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

// Returns current process PID
pid_t scheduler_getpid(void){
    if (current_idx < 0){
        return -1; 
    }

    return process_table[current_idx].pid;
}

// Returns pointer to current process's PID
PCB *scheduler_current(void){
    if(current_idx < 0){
        return NULL;
    }

    return &process_table[current_idx]; 
}

// Main function called from timer handler irq00
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

// Fills buffer with info on all processes
int scheduler_list(PCB *out_buf, int max){
    int count = 0; 
    for(int i =0 ; i < MAX_PROCESSES && count < max ; i++){
        if(process_table[i].state != PROCESS_DEAD){
            out_buf[count++] = process_table[i]; 
        }
    }

    return count; 
}

// Forces current process to yield
void scheduler_yield(void){
    if(current_idx < 0){
        return;
    }

    process_table[current_idx].quantums_left = 0; 
}

// Returns current process's PCB
PCB *get_process_by_pid(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_DEAD && process_table[i].pid == pid)
            return &process_table[i];
    }
    return NULL;
}