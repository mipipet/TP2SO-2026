#include "include/semaphore.h"
#include <lib.h>
#include "include/scheduler.h"
#include <stddef.h>

#define MAX_SEMAPHORES 64

static Semaphore sem_table[MAX_SEMAPHORES];
static int find_by_name(const char *name);
static int find_free_slot();
static int dequeue(int sem_id);
static int enqueue(int sem_id, int pid);
static int is_valid(int sem_id);
static int remove_waiting_pid_from_sem(int sem_id, int pid);
static int unblock_one_waiter(int sem_id);

// Creates or opens a named semaphore with the given initial value. Returns its id or -1 on failure
int sem_open(const char *name, int initial_value) {
    int id = find_by_name(name);
    if (id != -1) {
        sem_table[id].ref_count++;
        return id;
    }

    id = find_free_slot();
    if (id == -1) return -1;

    strcpy(sem_table[id].name, name);
    sem_table[id].value = initial_value;
    sem_table[id].wait_count = 0;
    sem_table[id].active = 1;
    sem_table[id].ref_count = 1;
    return id;
}

// Returns the id of the semaphore with the given name, or -1 if not found
static int find_by_name(const char *name) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sem_table[i].active == 1 && strcmp(sem_table[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Returns the id of the first free slot, or -1 if no slots available
static int find_free_slot() {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sem_table[i].active == 0) {
            return i;
        }
    }
    return -1;
}

// Closes the semaphore, marking its slot as free
int sem_close(int sem_id) {
    if (!is_valid(sem_id))
    return -1;

    sem_table[sem_id].ref_count--;
    if (sem_table[sem_id].ref_count <= 0) {
        sem_table[sem_id].active = 0;
        sem_table[sem_id].wait_count = 0;
    }

    return 0;
}

// Increments the semaphore. Unblocks a waiting process if any
int sem_post(int sem_id){
    if (!is_valid(sem_id))
    return -1;

    sem_table[sem_id].value++;

    if(sem_table[sem_id].value <= 0){
        unblock_one_waiter(sem_id);
    }

    return 0;
}

int sem_post_all(int sem_id){
    if (!is_valid(sem_id))
    return -1;

    while(sem_table[sem_id].wait_count > 0){
        sem_table[sem_id].value++;
        unblock_one_waiter(sem_id);
    }

    if(sem_table[sem_id].value < 0){
        sem_table[sem_id].value = 0;
    }

    return 0;
}

// Removes and returns the first PID from the waiting queue
static int dequeue(int sem_id) {
    int pid = sem_table[sem_id].waiting[0];
    for (int i = 0; i < sem_table[sem_id].wait_count - 1; i++) {
        sem_table[sem_id].waiting[i] = sem_table[sem_id].waiting[i + 1];
    }
    sem_table[sem_id].wait_count--;
    return pid;
}

// Decrements the semaphore. Returns 1 if the caller was blocked.
int sem_wait(int sem_id){
    if (!is_valid(sem_id)) 
    return -1;

    int pid = scheduler_getpid();
    sem_table[sem_id].value--;

    if(sem_table[sem_id].value >= 0){
        return 0;
    }

    if(enqueue(sem_id,pid) < 0){
        sem_table[sem_id].value++;
        return -1;
    }

    scheduler_block(pid);
    return 1;
}

// Adds a PID to the end of the waiting queue
static int enqueue(int sem_id, int pid) {
    remove_waiting_pid_from_sem(sem_id, pid);
    if(sem_table[sem_id].wait_count >= MAX_WAITING){
        return -1;
    }
    sem_table[sem_id].waiting[sem_table[sem_id].wait_count++] = pid;
    return 0;
}

static int unblock_one_waiter(int sem_id) {
    while(sem_table[sem_id].wait_count > 0){
        int pid = dequeue(sem_id);
        PCB *process = get_process_by_pid(pid);
        if(process != NULL && process->state == PROCESS_BLOCKED){
            scheduler_unblock(pid);
            return 1;
        }
    }

    return 0;
}

void sem_remove_waiting_pid(int pid) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sem_table[i].active) {
            int removed = remove_waiting_pid_from_sem(i, pid);
            sem_table[i].value += removed;
        }
    }
}

static int remove_waiting_pid_from_sem(int sem_id, int pid) {
    int write_idx = 0;
    int removed = 0;

    for (int read_idx = 0; read_idx < sem_table[sem_id].wait_count; read_idx++) {
        if (sem_table[sem_id].waiting[read_idx] != pid) {
            sem_table[sem_id].waiting[write_idx++] = sem_table[sem_id].waiting[read_idx];
        }else{
            removed++;
        }
    }

    sem_table[sem_id].wait_count = write_idx;
    return removed;
}

// Returns 1 if the sem_id is valid and active, 0 otherwise
static int is_valid(int sem_id) {
    return sem_id >= 0 && sem_id < MAX_SEMAPHORES && sem_table[sem_id].active == 1;
}
