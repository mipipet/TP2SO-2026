#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"
#include "../include/process_syscalls.h"

uint64_t test_sync(uint64_t argc, char *argv[]);
uint64_t test_mm(uint64_t argc, char *argv[]);
uint64_t test_processes(uint64_t argc, char *argv[]);
uint64_t test_prio(uint64_t argc, char *argv[]);
int loop_main(int argc, char **argv);
int ps_main(int argc, char **argv);
int kill_main(int argc, char **argv);
int nice_main(int argc, char **argv);
int block_main(int argc, char **argv);
int unblock_main(int argc, char **argv);
int mem_main(int argc, char **argv);
int cat_main(int argc, char **argv);
int wc_main(int argc, char **argv);
int filter_main(int argc, char **argv);
int mvar_main(int argc, char **argv);
extern void _invalidOp();

typedef struct {
    int argc;
    char **argv;
    int stdin_pipe;
    int stdout_pipe;
} TCommandLaunch;

static int execute_pipeline(char *commandInput);
static char *trim_spaces(char *text);
static int command_exists(const char *name);
static int is_test_command(const char *name);
static int test_sync_cmd(int argc, char *argv[]);
static int test_mm_cmd(int argc, char *argv[]);
static int test_proc_cmd(int argc, char *argv[]);
static int test_prio_cmd(int argc, char *argv[]);
static uint64_t command_process_owned_entry(uint64_t argc, char **argv);
static char **copy_args(int argc, char **argv);
static void free_args(int argc, char **argv);

const TShellCmd shellCmds[] = {
    {"help", helpCmd, ": Muestra los comandos disponibles\n"},
    {"exit", exitCmd, ": Salir del shell\n"},
    {"set-user", setUserCmd, ": Setea el nombre de usuario, con un maximo de 10 caracteres\n"},
    {"clear", clearCmd, ": Limpia la pantalla\n"},
    {"time", timeCmd, ": Muestra la hora actual\n"},
    {"font-size", fontSizeCmd, ": Cambia el tamanio de la fuente\n"},
    {"exceptions", exceptionCmd, ": Testear excepciones. Ingrese: exceptions [zero/invalidOpcode] para testear alguna operacion\n"},
    {"regs", regsCmd, ": Muestra los ultimos 18 registros de la CPU\n"},
    {"loop",  loop_main,  ": Corre un proceso de loop infinito\n"},
    {"ps",    ps_main,    ": Devuelve una lista de todos los procesos activos\n"},
    {"kill",  kill_main,  ": Mata un proceso por PID\n"},
    {"nice",  nice_main,  ": Cambia la prioridad de un proceso\n"},
    {"block", block_main, ": Bloquea un proceso por PID\n"},
    {"unblock", unblock_main, ": Desbloquea un proceso PID\n"},
    {"cat", cat_main, ": Imprime stdin tal como lo recibe\n"},
    {"wc", wc_main, ": Cuenta lineas recibidas por stdin\n"},
    {"filter", filter_main, ": Filtra vocales del stdin\n"},
    {"mvar", mvar_main, ": Simula MVar con lectores/escritores. Uso: mvar <escritores> <lectores>\n"},
    {"mem", mem_main, ": Muestra el estado de la memoria\n"},
    {"test_sync", test_sync_cmd, ": Testea los semaforos. Uso: test_sync <n> <use_sem>\n"},
    {"test_mm",   test_mm_cmd,   ": Testea el memory manager. Uso: test_mm <max_bytes>\n"},
    {"test_proc", test_proc_cmd, ": Testea procesos. Uso: test_proc <max_processes>\n"},
    {"test_prio", test_prio_cmd, ": Testea prioridades. Uso: test_prio <max_value>\n"},
    {NULL, NULL, NULL},
};

static int test_sync_cmd(int argc, char *argv[]) {
    return (int)test_sync((uint64_t)(argc - 1), argv + 1);
}

static int test_mm_cmd(int argc, char *argv[]) {
    return (int)test_mm((uint64_t)(argc - 1), argv + 1);
}

static int test_proc_cmd(int argc, char *argv[]) {
    return (int)test_processes((uint64_t)(argc - 1), argv + 1);
}

static int test_prio_cmd(int argc, char *argv[]) {
    return (int)test_prio((uint64_t)(argc - 1), argv + 1);
}

static int runs_in_shell(const char *name) {
    return strcmp(name, "help") == 0 ||
           strcmp(name, "exit") == 0 ||
           strcmp(name, "set-user") == 0 ||
           strcmp(name, "clear") == 0 ||
           strcmp(name, "time") == 0 ||
           strcmp(name, "font-size") == 0 ||
           strcmp(name, "exceptions") == 0 ||
           strcmp(name, "regs") == 0 ||
           strcmp(name, "mvar") == 0 ||
           strcmp(name, "ps") == 0 ||
           strcmp(name, "kill") == 0 ||
           strcmp(name, "nice") == 0 ||
           strcmp(name, "block") == 0 ||
           strcmp(name, "unblock") == 0;
}

static uint64_t command_process_entry(uint64_t argc, char **argv) {
    if (argc == 0 || argv == NULL || argv[0] == NULL) {
        sys_exit(CMD_ERROR);
        return CMD_ERROR;
    }

    for (int i = 0; shellCmds[i].name; i++) {
        if (strcmp(argv[0], shellCmds[i].name) == 0) {
            int status = shellCmds[i].function((int)argc, argv);
            sys_exit(status);
            return status;
        }
    }

    sys_exit(ERROR);
    return ERROR;
}

static uint64_t command_process_owned_entry(uint64_t argc, char **argv) {
    int status = ERROR;

    if (argc != 0 && argv != NULL && argv[0] != NULL) {
        for (int i = 0; shellCmds[i].name; i++) {
            if (strcmp(argv[0], shellCmds[i].name) == 0) {
                status = shellCmds[i].function((int)argc, argv);
                break;
            }
        }
    }

    free_args((int)argc, argv);
    sys_exit(status);
    return (uint64_t)status;
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

int regsCmd(int argc, char *argv[]) {
    uint64_t snap[18];
    get_regs(snap); 

    CPURegisters *regs = (CPURegisters *)snap; 

    printf("RAX: %llx\tRBX: %llx\n", regs->rax, regs->rbx);
    printf("RCX: %llx\tRDX: %llx\n", regs->rcx, regs->rdx);
    printf("RSI: %llx\tRDI: %llx\n", regs->rsi, regs->rdi);
    printf("RBP: %llx\tR8 : %llx\n", regs->rbp, regs->r8);
    printf("R9 : %llx\tR10: %llx\n", regs->r9, regs->r10);
    printf("R11: %llx\tR12: %llx\n", regs->r11, regs->r12);
    printf("R13: %llx\tR14: %llx\n", regs->r13, regs->r14);
    printf("R15: %llx\tRIP: %llx\n", regs->r15, regs->rip);
    printf("RSP: %llx\tRFLAGS: %llx\n", regs->rsp, regs->rflags);
    return OK;
}


int helpCmd(int argc, char *argv[]){
    printf("%s", "Comandos disponibles:\n");
    for(int i = 1; shellCmds[i].name; i++){
        if (!is_test_command(shellCmds[i].name)) {
            printf("%s", shellCmds[i].name);
            printf("%s", shellCmds[i].help);
        }
    }

    printf("%s", "\nTests provistos:\n");
    for(int i = 1; shellCmds[i].name; i++){
        if (is_test_command(shellCmds[i].name)) {
            printf("%s", shellCmds[i].name);
            printf("%s", shellCmds[i].help);
        }
    }
    return OK;
}

int exitCmd(int argc, char *argv[]) {
    clearScreen();
    printf("%s", "Saliendo del shell...\n");
    sleep(1000);
    shutdown();
    return EXIT_CODE;
}

int setUserCmd(int argc, char *argv[]){
    char newName[MAX_USER_LENGTH + 1];
    
    printf("%s", "Ingrese el nuevo nombre de usuario: ");
    readLine(newName, sizeof(newName));
    
    strncpy(shellUser, newName, MAX_USER_LENGTH);
    printf("Nombre de usuario actualizado a: %s\n", shellUser);
    return OK;
}

int clearCmd(int argc, char *argv[]){
    clearScreen();
    return OK;
}

int timeCmd(int argc, char *argv[]){
    char time[TIME_BUFF];
    getTime(time);
    printf("Hora del sistema: %s\n", time);
    return OK;
}

int fontSizeCmd(int argc, char *argv[]){
    char input[10];
    int size;
    
    printf("Ingrese el nuevo tamanio de la fuente (1-3): ");
    readLine(input, sizeof(input));
    size = atoi(input);
    
    if(size < 1 || size > 3){
        printf("Tamanio invalido. Debe estar entre 1 y 3.\n");
        return CMD_ERROR;
    }
    
    setFontScale(size);
    clearScreen();
    printf("Tamanio de fuente cambiado a: %d\n", size);
    return OK;
}

int CommandParse(char *commandInput){
    if(commandInput == NULL)
        return ERROR;

    for (int i = 0; commandInput[i] != '\0'; i++) {
        if (commandInput[i] == '|') {
            return execute_pipeline(commandInput);
        }
    }
    
    char *args[MAX_ARGS + 1];
    int argc = fillCommandAndArgs(args, commandInput);

    if(argc == 0)
        return ERROR;

    int foreground = 1;
    if (argc > 1 && strcmp(args[argc - 1], "&") == 0) {
        foreground = 0;
        args[--argc] = NULL;
    }

    for(int i = 0; shellCmds[i].name; i++) {
        if(strcmp(args[0], shellCmds[i].name) == 0) {
            if (runs_in_shell(args[0])) {
                return shellCmds[i].function(argc, args);
            }

            char **process_args = copy_args(argc, args);
            if (process_args == NULL) {
                printf("Error: no hay memoria para crear el proceso\n");
                return CMD_ERROR;
            }

            int pid = sys_create((process_func)command_process_owned_entry,
                                 shellCmds[i].name, 3, foreground,
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
    }

    return ERROR;
}

static int execute_pipeline(char *commandInput) {
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

    if (!command_exists(left_args[0]) || !command_exists(right_args[0])) {
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

static char *trim_spaces(char *text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }

    int len = strlen(text);
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[--len] = '\0';
    }

    return text;
}

static int command_exists(const char *name) {
    for (int i = 0; shellCmds[i].name; i++) {
        if (strcmp(name, shellCmds[i].name) == 0) {
            return 1;
        }
    }

    return 0;
}

static int is_test_command(const char *name) {
    return name != NULL &&
           name[0] == 't' &&
           name[1] == 'e' &&
           name[2] == 's' &&
           name[3] == 't' &&
           name[4] == '_';
}

static char **copy_args(int argc, char **argv) {
    char **copy = (char **)sys_mem_alloc(sizeof(char *) * (argc + 1));
    if (copy == NULL) {
        return NULL;
    }

    for (int i = 0; i <= argc; i++) {
        copy[i] = NULL;
    }

    for (int i = 0; i < argc; i++) {
        int len = strlen(argv[i]);
        copy[i] = (char *)sys_mem_alloc((uint64_t)len + 1);
        if (copy[i] == NULL) {
            free_args(argc, copy);
            return NULL;
        }

        for (int j = 0; j <= len; j++) {
            copy[i][j] = argv[i][j];
        }
    }

    copy[argc] = NULL;
    return copy;
}

static void free_args(int argc, char **argv) {
    if (argv == NULL) {
        return;
    }

    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            sys_mem_free(argv[i]);
        }
    }

    sys_mem_free(argv);
}

int fillCommandAndArgs(char *args[], char *input) {
    int argc = 0;
    int inArg = 0;

    for (int i = 0; input[i] != '\0' && argc < MAX_ARGS; i++) {
        if (input[i] == ' ') {
            input[i] = '\0';
            inArg = 0;
        } else if (!inArg) {
            args[argc++] = &input[i];
            inArg = 1;
        }
    }

    args[argc] = NULL;
    return argc;
}

int exceptionCmd(int argc, char * argv[]) {
    if (argc != 2 || argv[1] == NULL) {
        printf("Error: cantidad invalida de argumentos.\nUso: exceptions [zero, invalidOpcode]\n");
        return CMD_ERROR;
    }

    if (strcmp(argv[1], "zero") == 0) {
        int a = 1;
        int b = 0;
        int c = a / b;   
        printf("c: %d\n", c); 
    }
    else if (strcmp(argv[1], "invalidopcode") == 0) {
        printf("Ejecutando invalidOpcode...\n");
        _invalidOp();   
    }
    else {
        printf("Error: tipo de excepcion invalido.\nIngrese exceptions [zero, invalidOpcode] para testear alguna operacion\n");
        return CMD_ERROR;
    }

    return OK;
}
