#include "../include/lib.h"
#include "../include/process_syscalls.h"

int nice_main(int argc, char **argv) {

    if (argc != 3) {
        printf("Usage: nice <pid> <priority>\n");
        return -1;
    }

    int pid = atoi(argv[1]);
    int newPriority = atoi(argv[2]);
    if (sys_nice(pid, newPriority) < 0) {
        printf("Error: could not change process %d priority to %d.\n",
               pid, newPriority);
        return -1;
    }

    return 0;
}
