#include "include/semaphore.h"
#include <lib.h>
#include "include/scheduler.h"

#define MAX_SEMAPHORES 64

static Semaphore sem_table[MAX_SEMAPHORES];
static int find_by_name(const char *name); 
static int find_free_slot();   
static int dequeue(int sem_id);   
static void enqueue(int sem_id, int pid);   
static int is_valid(int sem_id); 

// Creates or opens a named semaphore with the given initial value. Returns its id or -1 on failure
int sem_open(const char *name, int initial_value) {
    int id = find_by_name(name);
    if (id != -1) return id;

    id = find_free_slot();
    if (id == -1) return -1;

    strcpy(sem_table[id].name, name);
    sem_table[id].value = initial_value;
    sem_table[id].wait_count = 0;
    sem_table[id].active = 1;
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

    sem_table[sem_id].active = 0;
    return 0;
}

// Increments the semaphore. Unblocks a waiting process if any
int sem_post(int sem_id){
    if (!is_valid(sem_id)) 
    return -1;

    if(sem_table[sem_id].wait_count > 0){
        int pid = dequeue(sem_id);
        scheduler_unblock(pid);
        return 0;
    }else{
        sem_table[sem_id].value++;
        return 0;
    } 
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

// Decrements the semaphore. Blocks the calling process if value is 0
int sem_wait(int sem_id){
    if (!is_valid(sem_id)) 
    return -1;

    if(sem_table[sem_id].value>0){
        sem_table[sem_id].value--;
        return 0;
    }else {
        int pid = scheduler_getpid();
        enqueue(sem_id,pid);
        scheduler_block(pid);
        return 0;
    }
}

// Adds a PID to the end of the waiting queue
static void enqueue(int sem_id, int pid) {
    sem_table[sem_id].waiting[sem_table[sem_id].wait_count++] = pid;
}

// Returns 1 if the sem_id is valid and active, 0 otherwise
static int is_valid(int sem_id) {
    return sem_id >= 0 && sem_id < MAX_SEMAPHORES && sem_table[sem_id].active == 1;
}