#include "../include/syscall.h"

#define IO_BUFFER_SIZE 128
#define EOF_CHAR 0x04

int cat_main(int argc, char **argv) {
    char buffer[IO_BUFFER_SIZE];
    int read;

    while ((read = sys_read(STDIN, buffer, sizeof(buffer))) > 0) {
        int eof = 0;
        for (int i = 0; i < read; i++) {
            if (buffer[i] == EOF_CHAR) {
                read = i;
                eof = 1;
                break;
            }
        }

        if (read > 0) {
            sys_write(STDOUT, buffer, read);
        }

        if (eof) {
            break;
        }
    }

    return 0;
}
