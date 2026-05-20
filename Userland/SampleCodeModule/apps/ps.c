#include "../include/lib.h"
#include "../include/process_syscalls.h"

int ps_main(int argc, char **argv) {
    char buf[4096];
    int n = sys_ps(buf, sizeof(buf));
    sys_write(STDOUT, buf, n);

    return 0;
}