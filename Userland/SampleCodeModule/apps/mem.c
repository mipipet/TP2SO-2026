#include "../include/lib.h"
#include "../include/syscall.h"

int mem_main(int argc, char **argv) {
    uint64_t total, used, free;
    uint64_t kind;

    sys_mem_info(&total, &used, &free);
    kind = sys_mem_kind();

    printf("Memory status:\n");
    printf("  Manager:   %s\n", kind == 1 ? "Buddy system" : "Free list");
    printf("  Total:     %d bytes\n", (int)total);
    printf("  Used:      %d bytes\n", (int)used);
    printf("  Free:      %d bytes\n", (int)free);
    printf("  Used %%:    %d%%\n", (int)(total > 0 ? (used * 100) / total : 0));

    return 0;
}
