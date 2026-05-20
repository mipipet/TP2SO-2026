#include "../include/lib.h"
#include "../include/process_syscalls.h"

int unblock_main(int argc, char **argv) {

    if (argc < 2) {
    printf("Usage: unblock <pid>\n");
    return 0;
    }

    int pid = atoi(argv[1]);
    sys_unblock(pid);

    return 0;
}