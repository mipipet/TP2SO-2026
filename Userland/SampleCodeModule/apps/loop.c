#include "../include/lib.h"
#include "../include/process_syscalls.h"
#include "../include/syscall.h"

#define LOOP_WAIT_ITERATIONS 120000000UL

static void active_wait(void) {
    for (volatile uint64_t i = 0; i < LOOP_WAIT_ITERATIONS; i++) {
    }
}

int loop_main(int argc, char **argv) {
    int pid = sys_getpid();
    char line[64];

    while(1) {
        int len = sprintf(line, "[loop] PID %d running...\n", pid);
        sys_write(STDOUT, line, len);
        active_wait();
    }

    return 0;
}
