#include "../include/lib.h"
#include "../include/process_syscalls.h"

void kill_main(int argc, char **argv) {

    if (argc < 2) {
    printf("Usage: kill <pid>\n");
    return;
    }

    int pid = atoi(argv[1]);
    sys_kill(pid);
}