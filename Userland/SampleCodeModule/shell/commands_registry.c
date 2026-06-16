#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/commands.h"
#include "commands_internal.h"

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

const TShellCmd shellCmds[] = {
    {"help", helpCmd, ": Muestra los comandos disponibles\n", 1, 0},
    {"exit", exitCmd, ": Salir del shell\n", 1, 0},
    {"set-user", setUserCmd, ": Setea el nombre de usuario, con un maximo de 10 caracteres\n", 1, 0},
    {"clear", clearCmd, ": Limpia la pantalla\n", 1, 0},
    {"time", timeCmd, ": Muestra la hora actual\n", 1, 0},
    {"font-size", fontSizeCmd, ": Cambia el tamanio de la fuente\n", 1, 0},
    {"exceptions", exceptionCmd, ": Testear excepciones. Ingrese: exceptions [zero/invalidOpcode] para testear alguna operacion\n", 1, 0},
    {"regs", regsCmd, ": Muestra los ultimos 18 registros de la CPU\n", 1, 0},
    {"loop",  loop_main,  ": Corre un proceso de loop infinito\n", 0, 0},
    {"ps",    ps_main,    ": Devuelve una lista de todos los procesos activos\n", 1, 0},
    {"kill",  kill_main,  ": Mata un proceso por PID\n", 1, 0},
    {"nice",  nice_main,  ": Cambia la prioridad de un proceso\n", 1, 0},
    {"block", block_main, ": Bloquea un proceso por PID\n", 1, 0},
    {"unblock", unblock_main, ": Desbloquea un proceso PID\n", 1, 0},
    {"cat", cat_main, ": Imprime stdin tal como lo recibe\n", 0, 0},
    {"wc", wc_main, ": Cuenta lineas recibidas por stdin\n", 0, 0},
    {"filter", filter_main, ": Filtra vocales del stdin\n", 0, 0},
    {"mvar", mvar_main, ": Simula MVar con lectores/escritores. Uso: mvar <escritores> <lectores>\n", 1, 0},
    {"mem", mem_main, ": Muestra el estado de la memoria\n", 0, 0},
    {"test_sync", test_sync_cmd, ": Testea los semaforos. Uso: test_sync <n> <use_sem>\n", 0, 1},
    {"test_mm",   test_mm_cmd,   ": Testea el memory manager. Uso: test_mm <max_bytes>\n", 0, 1},
    {"test_proc", test_proc_cmd, ": Testea procesos. Uso: test_proc <max_processes>\n", 0, 1},
    {"test_prio", test_prio_cmd, ": Testea prioridades. Uso: test_prio <max_value>\n", 0, 1},
    {NULL, NULL, NULL, 0, 0},
};

const TShellCmd *find_command(const char *name) {
    if (name == NULL) {
        return NULL;
    }

    for (int i = 0; shellCmds[i].name; i++) {
        if (strcmp(name, shellCmds[i].name) == 0) {
            return &shellCmds[i];
        }
    }

    return NULL;
}

int command_is_test(const TShellCmd *command) {
    return command != NULL && command->is_test;
}
