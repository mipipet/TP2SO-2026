#include "../include/syscall.h"

#define IO_BUFFER_SIZE 128
#define EOF_CHAR 0x04

static int is_vowel(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
           c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
}

int filter_main(int argc, char **argv) {
    char input[IO_BUFFER_SIZE];
    char output[IO_BUFFER_SIZE];
    int read;

    while ((read = sys_read(STDIN, input, sizeof(input))) > 0) {
        int out = 0;

        for (int i = 0; i < read; i++) {
            if (input[i] == EOF_CHAR) {
                if (out > 0) {
                    sys_write(STDOUT, output, out);
                }
                return 0;
            }

            if (!is_vowel(input[i])) {
                output[out++] = input[i];
            }
        }

        if (out > 0) {
            sys_write(STDOUT, output, out);
        }
    }

    return 0;
}
