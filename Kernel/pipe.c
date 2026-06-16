#include "include/pipe.h"
#include "include/semaphore.h"
#include "include/scheduler.h"
#include "include/interrupts.h"
#include <lib.h>
#include <stddef.h>

static Pipe pipe_table[MAX_PIPES];

static int pipe_sem_data[MAX_PIPES]; // counts available bytes
static int pipe_sem_space[MAX_PIPES]; // counts available space
static int pipe_reserved_readers[MAX_PIPES];
static int pipe_reserved_writers[MAX_PIPES];

static int wait_sem_blocking(int sem_id) {
    _cli();
    int result = sem_wait(sem_id);
    if (result < 0) {
        _sti();
        return -1;
    }

    while (result > 0 &&
           scheduler_current() != NULL &&
           scheduler_current()->state == PROCESS_BLOCKED) {
        _hlt();
    }

    _cli();
    return 0;
}

// Call after pipe_open to initialize pipe's semaphores
static void pipe_init_sems(int id){
    char name[32]; 

    name[0] = 'd'; name[1] = '0' + id; name[2] = '\0';
    pipe_sem_data[id]  = sem_open(name, 0);

    name[0] = 's';
    pipe_sem_space[id] = sem_open(name, 0);
}

// Creates a new pipe, returns its id or -1 on failure
int pipe_open(void){
    return pipe_open_with_capacity(PIPE_BUF_SIZE);
}

int pipe_open_with_capacity(int capacity){
    if(capacity <= 0 || capacity > PIPE_BUF_SIZE){
        return -1;
    }

    for(int i = 0 ; i < MAX_PIPES ; i++){
        if(!pipe_table[i].active){   

            pipe_table[i].read_pos = 0;
            pipe_table[i].write_pos = 0;
            pipe_table[i].count = 0;
            pipe_table[i].capacity = capacity;
            pipe_table[i].readers = 1;
            pipe_table[i].writers = 1;
            pipe_table[i].active = 1;
            pipe_reserved_readers[i] = 1;
            pipe_reserved_writers[i] = 1;
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

    Pipe *p = &pipe_table[pipe_id];

    if(is_write){
        if(pipe_reserved_writers[pipe_id] > 0){
            pipe_reserved_writers[pipe_id]--;
        }else{
            p->writers++;
        }
    }else{
        if(pipe_reserved_readers[pipe_id] > 0){
            pipe_reserved_readers[pipe_id]--;
        }else{
            p->readers++;
        }
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
        _cli();

        while(p->writers > 0 && p->count == 0){
            if(wait_sem_blocking(pipe_sem_data[pipe_id]) < 0){
                _sti();
                return read > 0 ? read : -1;
            }
        }

        if(p->count == 0){
            _sti();
            break;
        }

        buf[i] = p->buf[p->read_pos]; 
        p->read_pos = (p->read_pos + 1) % p->capacity;
        p->count--; 
        read++; 

        sem_post(pipe_sem_space[pipe_id]);
        _sti();
    }

    return read; 
}

// Write count bytes from buf. Blocks if full. Returns bytes written or -1
int pipe_write(int pipe_id, const char *buf, int count){
    if(pipe_id < 0 || pipe_id >= MAX_PIPES || !pipe_table[pipe_id].active){
        return -1; 
    }

    Pipe *p = &pipe_table[pipe_id];

    int written = 0;

    for(int i = 0 ; i < count ; i++){
        _cli();

        while(p->readers > 0 && p->count >= p->capacity){
            if(wait_sem_blocking(pipe_sem_space[pipe_id]) < 0){
                _sti();
                return written > 0 ? written : -1;
            }
        }

        if(p->readers == 0){
            _sti();
            return written > 0 ? written : -1;
        }

        p->buf[p->write_pos] = buf[i];
        p->write_pos = (p->write_pos + 1) % p->capacity;
        p->count++;
        written++;

        sem_post(pipe_sem_data[pipe_id]);
        _sti();
    }

    return written; 
}

// Close one end of the pipe (is_write: 1=write end, 0=read end)
int pipe_close(int pipe_id, int is_write){
    if(pipe_id < 0 || pipe_id >= MAX_PIPES || !pipe_table[pipe_id].active){
        return -1; 
    }

    _cli();
    Pipe *p = &pipe_table[pipe_id];

    if(is_write){
        if(p->writers > 0){
            p->writers--;
        }
        if(pipe_reserved_writers[pipe_id] > 0){
            pipe_reserved_writers[pipe_id]--;
        }
        if(p->writers == 0){
            sem_post_all(pipe_sem_data[pipe_id]);
        }else if(p->count < p->capacity){
            sem_post(pipe_sem_space[pipe_id]);
        }
    }else{
        if(p->readers > 0){
            p->readers--;
        }
        if(pipe_reserved_readers[pipe_id] > 0){
            pipe_reserved_readers[pipe_id]--;
        }
        if(p->readers == 0){
            sem_post_all(pipe_sem_space[pipe_id]);
        }else if(p->count > 0){
            sem_post(pipe_sem_data[pipe_id]);
        }
    }

    if(p->readers == 0 && p->writers == 0){
        p->active = 0; 
        pipe_reserved_readers[pipe_id] = 0;
        pipe_reserved_writers[pipe_id] = 0;
        sem_close(pipe_sem_data[pipe_id]);
        sem_close(pipe_sem_space[pipe_id]);
    }

    _sti();
    return 0; 
}
