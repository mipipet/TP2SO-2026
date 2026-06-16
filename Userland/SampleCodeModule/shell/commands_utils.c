#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/syscall.h"

char *trim_spaces(char *text) {
    while (*text == ' ' || *text == '\t') {
        text++;
    }

    int len = strlen(text);
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[--len] = '\0';
    }

    return text;
}

char **copy_args(int argc, char **argv) {
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

void free_args(int argc, char **argv) {
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
