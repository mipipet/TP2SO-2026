#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/syscall.h"
#include "../include/process_syscalls.h"

typedef struct {
    int argc;
    char **argv;
    int stdin_pipe;
    int stdout_pipe;
} TCommandLaunch;

static uint64_t command_process_entry(uint64_t argc, char **argv);
static uint64_t command_launch_entry(uint64_t unused, char **raw_context);

static uint64_t command_process_entry(uint64_t argc, char **argv) {
    if (argc == 0 || argv == NULL || argv[0] == NULL) {
        sys_exit(CMD_ERROR);
        return CMD_ERROR;
    }

    const TShellCmd *command = find_command(argv[0]);
    if (command != NULL) {
        int status = command->function((int)argc, argv);
        sys_exit(status);
        return status;
    }

    sys_exit(ERROR);
    return ERROR;
}

static uint64_t command_launch_entry(uint64_t unused, char **raw_context) {
    TCommandLaunch *ctx = (TCommandLaunch *)raw_context;
    int pid = sys_getpid();

    if (ctx->stdin_pipe >= 0 &&
        sys_pipe_set_fd(pid, STDIN, ctx->stdin_pipe, 0) < 0) {
        sys_exit(CMD_ERROR);
        return CMD_ERROR;
    }

    if (ctx->stdout_pipe >= 0 &&
        sys_pipe_set_fd(pid, STDOUT, ctx->stdout_pipe, 1) < 0) {
        sys_exit(CMD_ERROR);
        return CMD_ERROR;
    }

    return command_process_entry((uint64_t)ctx->argc, ctx->argv);
}

int execute_pipeline(char *commandInput) {
    char *pipe_pos = NULL;

    for (int i = 0; commandInput[i] != '\0'; i++) {
        if (commandInput[i] == '|') {
            if (pipe_pos != NULL) {
                printf("Error: solo se soporta un pipe\n");
                return CMD_ERROR;
            }
            pipe_pos = &commandInput[i];
        }
    }

    if (pipe_pos == NULL) {
        return ERROR;
    }

    *pipe_pos = '\0';
    char *left = trim_spaces(commandInput);
    char *right = trim_spaces(pipe_pos + 1);

    if (left[0] == '\0' || right[0] == '\0') {
        printf("Error: pipe incompleto\n");
        return CMD_ERROR;
    }

    char *left_args[MAX_ARGS + 1];
    char *right_args[MAX_ARGS + 1];
    int left_argc = fillCommandAndArgs(left_args, left);
    int right_argc = fillCommandAndArgs(right_args, right);

    if (left_argc == 0 || right_argc == 0) {
        printf("Error: pipe incompleto\n");
        return CMD_ERROR;
    }

    if (strcmp(right_args[right_argc - 1], "&") == 0) {
        printf("Error: pipes en background no soportados\n");
        return CMD_ERROR;
    }

    if (find_command(left_args[0]) == NULL ||
        find_command(right_args[0]) == NULL) {
        return ERROR;
    }

    int pipe_id = sys_pipe_open();
    if (pipe_id < 0) {
        printf("Error: no se pudo crear el pipe\n");
        return CMD_ERROR;
    }

    TCommandLaunch left_ctx = {left_argc, left_args, -1, pipe_id};
    TCommandLaunch right_ctx = {right_argc, right_args, pipe_id, -1};

    int left_pid = sys_create((process_func)command_launch_entry,
                              left_args[0], 3, 1, 0, (char **)&left_ctx);
    if (left_pid < 0) {
        sys_pipe_close(pipe_id, 0);
        sys_pipe_close(pipe_id, 1);
        printf("Error: no se pudo crear el proceso izquierdo\n");
        return CMD_ERROR;
    }

    int right_pid = sys_create((process_func)command_launch_entry,
                               right_args[0], 3, 1, 0, (char **)&right_ctx);
    if (right_pid < 0) {
        sys_kill(left_pid);
        sys_pipe_close(pipe_id, 0);
        sys_pipe_close(pipe_id, 1);
        printf("Error: no se pudo crear el proceso derecho\n");
        return CMD_ERROR;
    }

    sys_wait(left_pid);
    sys_wait(right_pid);
    return OK;
}
