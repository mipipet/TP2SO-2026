#include "../include/lib.h"
#include "../include/process_syscalls.h"

int kill_main(int argc, char **argv) {

    if (argc < 2) {
    printf("Usage: kill <pid>\n");
    return 0;
    }

    int pid = atoi(argv[1]);
    sys_kill(pid);

    return 0;
}