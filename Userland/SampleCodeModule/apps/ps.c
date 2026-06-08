#include "../include/lib.h"
#include "../include/process_syscalls.h"
#include "../include/syscall.h"

int ps_main(int argc, char **argv) {
    static char buf[4096];
    int n = sys_ps(buf, sizeof(buf));
    sys_write(1, buf, n);

    return 0;
}
