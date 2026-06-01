#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define MAX_PIPES       32
#define PIPE_BUF_SIZE   512

typedef struct {
    char     buf[PIPE_BUF_SIZE];
    int      read_pos;
    int      write_pos;
    int      count;           // bytes currently in buffer
    int      active;
    int      readers;         // number of open read ends
    int      writers;         // number of open write ends
} Pipe;

// Creates a new pipe, returns its id or -1 on failure
int pipe_open(void);

// Read up to count bytes into buf. Blocks if empty. Returns bytes read or -1
int pipe_read(int pipe_id, char *buf, int count);

// Write count bytes from buf. Blocks if full. Returns bytes written or -1
int pipe_write(int pipe_id, const char *buf, int count);

// Close one end of the pipe (is_write: 1=write end, 0=read end)
int pipe_close(int pipe_id, int is_write);

#endif