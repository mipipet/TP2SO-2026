#include "../include/lib.h"
#include "../include/process_syscalls.h"

int block_main(int argc, char **argv) {

    if (argc < 2) {
    printf("Usage: block <pid>\n");
    return 0;
    }

    int pid = atoi(argv[1]);
    sys_block(pid);

    return 0;
}