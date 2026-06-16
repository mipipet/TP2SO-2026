#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

#define MAX_ARGS 8
#define OK 0
#define ERROR -1
#define EXIT_CODE 1
#define CMD_ERROR -2

typedef int (*cmd_fn)(int argc, char *argv[]);

typedef struct{
    const char *name;
    cmd_fn function;
    const char *help;
    int runs_in_shell;
    int is_test;
}TShellCmd;

extern const TShellCmd shellCmds[];

int CommandParse(char *commandInput);
int execute_pipeline(char *commandInput);

char *trim_spaces(char *text);
char **copy_args(int argc, char **argv);
void free_args(int argc, char **argv);
int fillCommandAndArgs(char *args[], char *input);

const TShellCmd *find_command(const char *name);
int command_is_test(const TShellCmd *command);

#endif
