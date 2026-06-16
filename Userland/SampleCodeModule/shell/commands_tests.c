#include <stdint.h>
#include "../include/commands.h"
#include "commands_internal.h"

uint64_t test_sync(uint64_t argc, char *argv[]);
uint64_t test_mm(uint64_t argc, char *argv[]);
uint64_t test_processes(uint64_t argc, char *argv[]);
uint64_t test_prio(uint64_t argc, char *argv[]);

int test_sync_cmd(int argc, char *argv[]) {
    return (int)test_sync((uint64_t)(argc - 1), argv + 1);
}

int test_mm_cmd(int argc, char *argv[]) {
    return (int)test_mm((uint64_t)(argc - 1), argv + 1);
}

int test_proc_cmd(int argc, char *argv[]) {
    return (int)test_processes((uint64_t)(argc - 1), argv + 1);
}

int test_prio_cmd(int argc, char *argv[]) {
    return (int)test_prio((uint64_t)(argc - 1), argv + 1);
}