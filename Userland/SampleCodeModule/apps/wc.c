#include "../include/lib.h"
#include "../include/syscall.h"

#define IO_BUFFER_SIZE 128
#define EOF_CHAR 0x04

int wc_main(int argc, char **argv) {
    char buffer[IO_BUFFER_SIZE];
    int lines = 0;
    int read;

    while ((read = sys_read(STDIN, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < read; i++) {
            if (buffer[i] == EOF_CHAR) {
                printf("%d\n", lines);
                return 0;
            }

            if (buffer[i] == '\n') {
                lines++;
            }
        }
    }

    printf("%d\n", lines);
    return 0;
}
