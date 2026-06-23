#include "include/scheduler.h"
#include "include/process.h"
#include "include/keyboardDriver.h"
#include "include/pipe.h"
#include "include/semaphore.h"
#include <memManager.h>
#include <lib.h>
#include <stddef.h>

static PCB process_table[MAX_PROCESSES];
static int process_count = 0;
static int current_idx = -1;
static int next_pid = 1;
static uint8_t idle_stack[STACK_SIZE];
static int need_resched = 0;

#define READY_NONE -1
#define PRIORITY_LEVELS (PROCESS_PRIORITY_MAX - PROCESS_PRIORITY_MIN + 1)
#define AGING_THRESHOLD_TICKS 6
#define DEFAULT_PRIORITY 3
#define DEFAULT_QUANTUM 1

typedef struct {
    int head;
    int tail;
} ReadyQueue;

static ReadyQueue ready_queues[PRIORITY_LEVELS];

extern void _hlt(void);

void scheduler_exit_current(int status);
static void make_ready(int idx, int reset_effective_priority);
static void remove_from_ready_queue(int idx);

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

static int find_index_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD) {
            return i;
        }
    }

    return -1;
}

static int clamp_priority(int priority) {
    if (priority < PROCESS_PRIORITY_MIN) {
        return PROCESS_PRIORITY_MIN;
    }

    if (priority > PROCESS_PRIORITY_MAX) {
        return PROCESS_PRIORITY_MAX;
    }

    return priority;
}

static int priority_queue_index(int priority) {
    return clamp_priority(priority) - PROCESS_PRIORITY_MIN;
}

static void ready_queues_init(void) {
    for (int i = 0; i < PRIORITY_LEVELS; i++) {
        ready_queues[i].head = READY_NONE;
        ready_queues[i].tail = READY_NONE;
    }
}

static void enqueue_ready_queue(int idx) {
    if (idx <= 0 || idx >= MAX_PROCESSES || process_table[idx].in_ready_queue) {
        return;
    }

    int q_idx = priority_queue_index(process_table[idx].effective_priority);
    process_table[idx].ready_next = READY_NONE;

    if (ready_queues[q_idx].tail == READY_NONE) {
        ready_queues[q_idx].head = idx;
        ready_queues[q_idx].tail = idx;
    } else {
        process_table[ready_queues[q_idx].tail].ready_next = idx;
        ready_queues[q_idx].tail = idx;
    }

    process_table[idx].in_ready_queue = 1;
}

static void remove_from_ready_queue(int idx) {
    if (idx <= 0 || idx >= MAX_PROCESSES || !process_table[idx].in_ready_queue) {
        return;
    }

    int q_idx = priority_queue_index(process_table[idx].effective_priority);
    int prev = READY_NONE;
    int current = ready_queues[q_idx].head;

    while (current != READY_NONE) {
        if (current == idx) {
            int next = process_table[current].ready_next;

            if (prev == READY_NONE) {
                ready_queues[q_idx].head = next;
            } else {
                process_table[prev].ready_next = next;
            }

            if (ready_queues[q_idx].tail == current) {
                ready_queues[q_idx].tail = prev;
            }

            process_table[current].ready_next = READY_NONE;
            process_table[current].in_ready_queue = 0;
            return;
        }

        prev = current;
        current = process_table[current].ready_next;
    }

    process_table[idx].ready_next = READY_NONE;
    process_table[idx].in_ready_queue = 0;
}

static int dequeue_ready_queue(int priority) {
    int q_idx = priority_queue_index(priority);
    int idx = ready_queues[q_idx].head;

    if (idx == READY_NONE) {
        return READY_NONE;
    }

    ready_queues[q_idx].head = process_table[idx].ready_next;
    if (ready_queues[q_idx].head == READY_NONE) {
        ready_queues[q_idx].tail = READY_NONE;
    }

    process_table[idx].ready_next = READY_NONE;
    process_table[idx].in_ready_queue = 0;
    process_table[idx].ready_wait_ticks = 0;
    return idx;
}

static int highest_ready_priority(void) {
    for (int pr = PROCESS_PRIORITY_MAX; pr >= PROCESS_PRIORITY_MIN; pr--) {
        if (ready_queues[priority_queue_index(pr)].head != READY_NONE) {
            return pr;
        }
    }

    return 0;
}

static void age_ready_processes(void) {
    for (int idx = 1; idx < MAX_PROCESSES; idx++) {
        PCB *process = &process_table[idx];

        if (!process->in_ready_queue || process->state != PROCESS_READY) {
            continue;
        }

        process->ready_wait_ticks++;

        if (process->ready_wait_ticks >= AGING_THRESHOLD_TICKS &&
            process->effective_priority < PROCESS_PRIORITY_MAX) {
            remove_from_ready_queue(idx);
            process->effective_priority++;
            process->ready_wait_ticks = 0;
            enqueue_ready_queue(idx);
        }
    }
}

static int dequeue_next_ready(void) {
    age_ready_processes();

    for (int pr = PROCESS_PRIORITY_MAX; pr >= PROCESS_PRIORITY_MIN; pr--) {
        int idx = dequeue_ready_queue(pr);
        if (idx != READY_NONE) {
            process_table[idx].effective_priority = process_table[idx].priority;
            return idx;
        }
    }

    return 0;
}

static void make_ready(int idx, int reset_effective_priority) {
    if (idx <= 0 || idx >= MAX_PROCESSES ||
        process_table[idx].state == PROCESS_DEAD ||
        process_table[idx].state == PROCESS_ZOMBIE) {
        return;
    }

    if (process_table[idx].in_ready_queue) {
        remove_from_ready_queue(idx);
    }

    if (reset_effective_priority) {
        process_table[idx].effective_priority = process_table[idx].priority;
        process_table[idx].ready_wait_ticks = 0;
    }

    process_table[idx].state = PROCESS_READY;
    process_table[idx].quantums_left = DEFAULT_QUANTUM;
    enqueue_ready_queue(idx);

    if (current_idx > 0 &&
        process_table[idx].effective_priority > process_table[current_idx].effective_priority) {
        need_resched = 1;
    }
}

static void ensure_foreground_owner(void) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_DEAD &&
            process_table[i].state != PROCESS_ZOMBIE &&
            process_table[i].foreground) {
            return;
        }
    }

    int shell_idx = find_index_by_pid(1);
    if (shell_idx >= 0 && process_table[shell_idx].state != PROCESS_ZOMBIE) {
        process_table[shell_idx].foreground = 1;
    }
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
    ready_queues_init();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_DEAD;
        process_table[i].ready_next = READY_NONE;
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
    idle->effective_priority = 1;
    idle->ready_next = READY_NONE;
    idle->in_ready_queue = 0;
    idle->ready_wait_ticks = 0;
    copy_name(idle->name, "idle");

    setup_initial_stack(idle, idle_process, 0, NULL);

    process_count = 1;
    current_idx = 0;
    process_table[0].state = PROCESS_RUNNING;
    need_resched = 0;
}

pid_t scheduler_create(void *entry, const char *name, int priority, int fg, int argc, char **argv) {
    if(priority < PROCESS_PRIORITY_MIN || priority > PROCESS_PRIORITY_MAX){
        priority = DEFAULT_PRIORITY;
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
    p->effective_priority = priority;
    p->quantums_left = DEFAULT_QUANTUM; 
    p->ready_next = READY_NONE;
    p->in_ready_queue = 0;
    p->ready_wait_ticks = 0;
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
    make_ready(slot, 1);
    return p->pid;
}

int scheduler_kill(pid_t pid) {
    if (pid <= 1) {
        return -1; 
    }

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if(process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD){
            if (i == current_idx) {
                scheduler_exit_current(-1);
                return 0;
            }

            remove_from_ready_queue(i);
            restore_parent_foreground(&process_table[i]);
            sem_remove_waiting_pid(pid);
            close_process_fds(&process_table[i]);
            process_table[i].state = PROCESS_DEAD;
            process_table[i].quantums_left = 0;
            process_table[i].waiting_for = -1;
            process_table[i].effective_priority = 0;
            process_table[i].ready_next = READY_NONE;
            process_table[i].in_ready_queue = 0;
            process_table[i].ready_wait_ticks = 0;

            if(process_table[i].stack_base != idle_stack){
                mm_free(process_table[i].stack_base);
                process_table[i].stack_base = NULL;
            }

            process_count--;

            for (int j = 0; j < MAX_PROCESSES; j++) {
                if (process_table[j].state == PROCESS_BLOCKED &&
                    process_table[j].waiting_for == pid) {
                    process_table[j].waiting_for = -1;
                    make_ready(j, 1);
                }
            }

            ensure_foreground_owner();
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
    process_table[current_idx].quantums_left = 0;
    need_resched = 1;
}
void scheduler_block(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD) {
            remove_from_ready_queue(i);
            process_table[i].state = PROCESS_BLOCKED;

            if (i == current_idx) {
                process_table[i].quantums_left = 0;
                need_resched = 1;
            }
            return;
        }
    }
}

void scheduler_unblock(pid_t pid){
    for(int i = 0 ; i < MAX_PROCESSES ; i++){
        if(process_table[i].pid == pid && process_table[i].state == PROCESS_BLOCKED){
            make_ready(i, 1);
            return;
        }
    }
}

int scheduler_nice(pid_t pid, int new_priority){
    if(new_priority < PROCESS_PRIORITY_MIN || new_priority > PROCESS_PRIORITY_MAX){
        return -1;
    }

    for(int i = 0 ; i < MAX_PROCESSES ; i++){
        if(process_table[i].pid == pid && process_table[i].state != PROCESS_DEAD){
            int was_ready = process_table[i].in_ready_queue;

            if(was_ready){
                remove_from_ready_queue(i);
            }

            process_table[i].priority = new_priority;
            process_table[i].effective_priority = new_priority;
            process_table[i].ready_wait_ticks = 0;
            process_table[i].quantums_left = DEFAULT_QUANTUM;

            if(was_ready){
                enqueue_ready_queue(i);
            }

            if(i == current_idx ||
               (current_idx >= 0 &&
                new_priority > process_table[current_idx].effective_priority)){
                need_resched = 1;
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

    remove_from_ready_queue(idx);

    if (process_table[idx].stack_base != NULL &&
        process_table[idx].stack_base != idle_stack) {
        mm_free(process_table[idx].stack_base);
    }

    process_table[idx].pid = 0;
    process_table[idx].parent_pid = 0;
    process_table[idx].state = PROCESS_DEAD;
    process_table[idx].priority = 0;
    process_table[idx].effective_priority = 0;
    process_table[idx].quantums_left = 0;
    process_table[idx].ready_next = READY_NONE;
    process_table[idx].in_ready_queue = 0;
    process_table[idx].ready_wait_ticks = 0;
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
            need_resched = 1;
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
    remove_from_ready_queue(current_idx);
    current->state = PROCESS_ZOMBIE;
    current->quantums_left = 0;
    current->ready_wait_ticks = 0;
    need_resched = 1;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_BLOCKED &&
            process_table[i].waiting_for == current->pid) {
            process_table[i].waiting_for = -1;
            if (finished_was_foreground) {
                process_table[i].foreground = 1;
                keyboard_clear_buffer();
            }
            make_ready(i, 1);
        }
    }

    ensure_foreground_owner();
}

uint64_t scheduler_tick(uint64_t current_rsp){
    if(current_idx >= 0){
        process_table[current_idx].rsp = current_rsp;

        if(process_table[current_idx].state == PROCESS_RUNNING){
            process_table[current_idx].quantums_left--;

            if(!need_resched && process_table[current_idx].quantums_left > 0 &&
               highest_ready_priority() <= process_table[current_idx].effective_priority){
                return current_rsp;
            }

            if(current_idx > 0){
                make_ready(current_idx, 1);
            }
        }
    }

    int next = dequeue_next_ready();

    process_table[next].state = PROCESS_RUNNING;
    process_table[next].quantums_left = DEFAULT_QUANTUM;
    current_idx = next;
    need_resched = 0;

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
    need_resched = 1;
}

int scheduler_should_reschedule(void) {
    if (current_idx < 0) {
        return 0;
    }

    PCB *current = &process_table[current_idx];
    return need_resched ||
           current->state != PROCESS_RUNNING ||
           current->quantums_left <= 0 ||
           highest_ready_priority() > current->effective_priority;
}
