#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"
#include "commands_internal.h"

extern void _invalidOp();

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

int helpCmd(int argc, char *argv[]) {
    printf("%s", "Comandos disponibles:\n");
    for (int i = 1; shellCmds[i].name; i++) {
        if (!command_is_test(&shellCmds[i])) {
            printf("%s", shellCmds[i].name);
            printf("%s", shellCmds[i].help);
        }
    }

    printf("%s", "\nTests provistos:\n");
    for (int i = 1; shellCmds[i].name; i++) {
        if (command_is_test(&shellCmds[i])) {
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

int setUserCmd(int argc, char *argv[]) {
    char newName[MAX_USER_LENGTH + 1];

    printf("%s", "Ingrese el nuevo nombre de usuario: ");
    readLine(newName, sizeof(newName));

    strncpy(shellUser, newName, MAX_USER_LENGTH);
    printf("Nombre de usuario actualizado a: %s\n", shellUser);
    return OK;
}

int clearCmd(int argc, char *argv[]) {
    clearScreen();
    return OK;
}

int timeCmd(int argc, char *argv[]) {
    char time[TIME_BUFF];
    getTime(time);
    printf("Hora del sistema: %s\n", time);
    return OK;
}

int fontSizeCmd(int argc, char *argv[]) {
    char input[10];
    int size;

    printf("Ingrese el nuevo tamanio de la fuente (1-3): ");
    readLine(input, sizeof(input));
    size = atoi(input);

    if (size < 1 || size > 3) {
        printf("Tamanio invalido. Debe estar entre 1 y 3.\n");
        return CMD_ERROR;
    }

    setFontScale(size);
    clearScreen();
    printf("Tamanio de fuente cambiado a: %d\n", size);
    return OK;
}

int exceptionCmd(int argc, char *argv[]) {
    if (argc != 2 || argv[1] == NULL) {
        printf("Error: cantidad invalida de argumentos.\nUso: exceptions [zero, invalidOpcode]\n");
        return CMD_ERROR;
    }

    if (strcmp(argv[1], "zero") == 0) {
        int a = 1;
        int b = 0;
        int c = a / b;
        printf("c: %d\n", c);
    } else if (strcmp(argv[1], "invalidopcode") == 0) {
        printf("Ejecutando invalidOpcode...\n");
        _invalidOp();
    } else {
        printf("Error: tipo de excepcion invalido.\nIngrese exceptions [zero, invalidOpcode] para testear alguna operacion\n");
        return CMD_ERROR;
    }

    return OK;
}
