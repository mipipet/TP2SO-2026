#include "../include/lib.h"
#include "../include/process_syscalls.h"

int kill_main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: kill <pid>\n");
        return -1;
    }

    int pid = atoi(argv[1]);
    if (sys_kill(pid) < 0) {
        printf("Error: could not kill process %d.\n", pid);
        return -1;
    }

    return 0;
}
