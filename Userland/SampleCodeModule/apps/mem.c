#include "../include/lib.h"
#include "../include/syscall.h"

int mem_main(int argc, char **argv) {
    uint64_t total, used, free;
    sys_mem_info(&total, &used, &free);

    printf("Memory status:\n");
    printf("  Total:     %d bytes\n", (int)total);
    printf("  Used:      %d bytes\n", (int)used);
    printf("  Free:      %d bytes\n", (int)free);
    printf("  Used %%:    %d%%\n", (int)(total > 0 ? (used * 100) / total : 0));

    return 0;
}