#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#define MAX_WAITING 64
#define NAME_LEN 32

typedef int sem_t;

typedef struct {
    char name[NAME_LEN];       
    int  value;           //counter
    int  waiting[MAX_WAITING];     // PIDs waiting
    int  wait_count;      // how many are waiting
    int  active;          // 1 = exsist, 0 = free
    int  ref_count;
} Semaphore;

int sem_open(const char *name, int initial_value);
int sem_wait(int sem_id);
int sem_post(int sem_id);
int sem_close(int sem_id);

#endif
