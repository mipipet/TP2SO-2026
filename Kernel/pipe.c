#include "include/pipe.h"
#include "include/semaphore.h"
#include "include/scheduler.h"
#include <lib.h>
#include <stddef.h>

static Pipe pipe_table[MAX_PIPES];

static int pipe_sem_data[MAX_PIPES]; // counts available bytes
static int pipe_sem_space[MAX_PIPES]; // counts available space

extern void _hlt(void);

static int wait_sem_blocking(int sem_id) {
    int result = sem_wait(sem_id);
    if (result < 0) {
        return -1;
    }

    while (scheduler_current() != NULL &&
           scheduler_current()->state == PROCESS_BLOCKED) {
        _hlt();
    }

    return 0;
}

// Call after pipe_open to initialize pipe's semaphores
static void pipe_init_sems(int id){
    char name[32]; 

    name[0] = 'd'; name[1] = '0' + id; name[2] = '\0';
    pipe_sem_data[id]  = sem_open(name, 0);

    name[0] = 's';
    pipe_sem_space[id] = sem_open(name, PIPE_BUF_SIZE);
}

// Creates a new pipe, returns its id or -1 on failure
int pipe_open(void){
    for(int i = 0 ; i < MAX_PIPES ; i++){
        if(!pipe_table[i].active){   

            pipe_table[i].read_pos = 0;
            pipe_table[i].write_pos = 0;
            pipe_table[i].count = 0;
            pipe_table[i].readers = 1;
            pipe_table[i].writers = 1;
            pipe_table[i].active = 1;
            pipe_init_sems(i);

            return i;
        }
    }

    return -1;
}

int pipe_attach(int pipe_id, int is_write) {
    if(pipe_id < 0 || pipe_id >= MAX_PIPES || !pipe_table[pipe_id].active){
        return -1;
    }

    if(is_write){
        pipe_table[pipe_id].writers++;
    }else{
        pipe_table[pipe_id].readers++;
    }

    return 0;
}

int pipe_is_open(int pipe_id) {
    return pipe_id >= 0 && pipe_id < MAX_PIPES && pipe_table[pipe_id].active;
}

// Read up to count bytes into buf. Blocks if empty. Returns bytes read or -1
int pipe_read(int pipe_id, char *buf, int count){
    if(pipe_id < 0 || pipe_id >= MAX_PIPES || !pipe_table[pipe_id].active){
        return -1; 
    }

    Pipe *p = &pipe_table[pipe_id]; 
    int read = 0; 

    for(int i = 0 ; i < count ; i++){
        if(p->writers == 0 && p->count == 0){ // EOF
            break; 
        }

        if(wait_sem_blocking(pipe_sem_data[pipe_id]) < 0){
            return -1;
        }

        if(p->writers == 0 && p->count == 0){ // EOF after wakeup
            break;
        }

        buf[i] = p->buf[p->read_pos]; 
        p->read_pos = (p->read_pos + 1) % PIPE_BUF_SIZE;
        p->count--; 
        read++; 

        sem_post(pipe_sem_space[pipe_id]);
    }

    return read; 
}

// Write count bytes from buf. Blocks if full. Returns bytes written or -1
int pipe_write(int pipe_id, const char *buf, int count){
    if(pipe_id < 0 || pipe_id >= MAX_PIPES || !pipe_table[pipe_id].active){
        return -1; 
    }

    Pipe *p = &pipe_table[pipe_id];

    if(p->readers == 0){
        return -1; 
    }

    int written = 0;

    for(int i = 0 ; i < count ; i++){
        if(wait_sem_blocking(pipe_sem_space[pipe_id]) < 0){
            return -1;
        }

        if(p->readers == 0){
            return written > 0 ? written : -1;
        }

        p->buf[p->write_pos] = buf[i]; 
        p->write_pos = (p->write_pos + 1) % PIPE_BUF_SIZE, 
        p->count++; 
        written++;

        sem_post(pipe_sem_data[pipe_id]); 
    }

    return written; 
}

// Close one end of the pipe (is_write: 1=write end, 0=read end)
int pipe_close(int pipe_id, int is_write){
    if(pipe_id < 0 || pipe_id >= MAX_PIPES || !pipe_table[pipe_id].active){
        return -1; 
    }

    Pipe *p = &pipe_table[pipe_id];

    if(is_write){
        if(p->writers > 0){
            p->writers--;
        }
        if(p->writers == 0){
            sem_post(pipe_sem_data[pipe_id]);
        }
    }else{
        if(p->readers > 0){
            p->readers--;
        }
        if(p->readers == 0){
            sem_post(pipe_sem_space[pipe_id]);
        }
    }

    if(p->readers == 0 && p->writers == 0){
        p->active = 0; 
        sem_close(pipe_sem_data[pipe_id]);
        sem_close(pipe_sem_space[pipe_id]);
    }

    return 0; 
}
