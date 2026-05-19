#include "../include/lib.h"
#include "../include/process_syscalls.h"

int nice_main(int argc, char **argv) {

    if (argc < 3) {
    printf("Usage: nice <pid> <priority>\n");
    return 0;
    }

    int pid = atoi(argv[1]);
    int newPriority = atoi(argv[2]);
    sys_nice(pid, newPriority);

    return 0;
}