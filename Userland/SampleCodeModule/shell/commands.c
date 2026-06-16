#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"
#include "../include/process_syscalls.h"
#include "commands_internal.h"

static uint64_t command_process_owned_entry(uint64_t argc, char **argv);

int CommandParse(char *commandInput) {
    if (commandInput == NULL)
        return ERROR;

    for (int i = 0; commandInput[i] != '\0'; i++) {
            if (commandInput[i] == '|') {
                return execute_pipeline(commandInput);
        }
    }

    char *args[MAX_ARGS + 1];
    int argc = fillCommandAndArgs(args, commandInput);

    if (argc == 0)
        return ERROR;

    int foreground = 1;
    if (argc > 1 && strcmp(args[argc - 1], "&") == 0) {
        foreground = 0;
        args[--argc] = NULL;
    }

    const TShellCmd *command = find_command(args[0]);
    if (command == NULL) {
        return ERROR;
    }

    if (command->runs_in_shell) {
        return command->function(argc, args);
    }

    char **process_args = copy_args(argc, args);
    if (process_args == NULL) {
        printf("Error: no hay memoria para crear el proceso\n");
        return CMD_ERROR;
    }

    int pid = sys_create((process_func)command_process_owned_entry,
                         command->name, 3, foreground,
                         argc, process_args);
    if (pid < 0) {
        free_args(argc, process_args);
        printf("Error: no se pudo crear el proceso\n");
        return CMD_ERROR;
    }

    if (foreground) {
        sys_wait(pid);
    } else {
        printf("[%d]\n", pid);
    }

    return OK;
}

static uint64_t command_process_owned_entry(uint64_t argc, char **argv) {
    int status = ERROR;

    if (argc != 0 && argv != NULL && argv[0] != NULL) {
        const TShellCmd *command = find_command(argv[0]);
        if (command != NULL) {
            status = command->function((int)argc, argv);
        }
    }

    free_args((int)argc, argv);
    sys_exit(status);
    return (uint64_t)status;
}
