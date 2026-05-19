#include "../include/lib.h"
#include "../include/process_syscalls.h"

void block_main(int argc, char **argv) {

    if (argc < 2) {
    printf("Usage: block <pid>\n");
    return;
    }

    int pid = atoi(argv[1]);
    sys_block(pid);
}