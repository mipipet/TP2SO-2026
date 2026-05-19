#include "../include/lib.h"
#include "../include/process_syscalls.h"

int ps_main(int argc, char **argv) {
    char buffer[1024];
    sys_ps(buffer);
    printf("%s", buffer);

    return 0;
}