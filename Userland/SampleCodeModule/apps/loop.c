#include "../include/lib.h"
#include "../include/process_syscalls.h"

int loop_main(int argc, char **argv) {
    int pid = sys_getpid();
    while(1) {
        printf("[loop] PID %d running...\n", pid);
        sleep(500);
    }

    return 0;
}