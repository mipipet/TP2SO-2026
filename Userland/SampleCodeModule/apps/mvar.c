#include "../include/lib.h"
#include "../include/process_syscalls.h"
#include "../include/syscall.h"

#define MVAR_MAX_WRITERS 26
#define MVAR_MAX_READERS 20
#define MVAR_PIPE_FD 2
#define MVAR_WORKER_DELAY_MS 350
#define MVAR_MAX_DELAY_FACTOR 3

typedef struct {
    int pipe_id;
    int delay_ms;
    int worker_id;
} MVarWorkerArgs;

static uint64_t mvar_writer_entry(uint64_t argc, char **raw_args);
static uint64_t mvar_reader_entry(uint64_t argc, char **raw_args);
static uint64_t mvar_exit(int status);
static int spawn_mvar_worker(process_func entry, const char *name,
                             int pipe_id, int worker_id, int delay_ms);

int mvar_main(int argc, char **argv) {
    if (argc != 3) {
        printf("Uso: mvar <escritores> <lectores>\n");
        printf("  escritores: cantidad de procesos escritores (1-26)\n");
        printf("  lectores: cantidad de procesos lectores (1-20)\n");
        return -2;
    }

    int num_writers = atoi(argv[1]);
    int num_readers = atoi(argv[2]);

    if (num_writers <= 0 || num_writers > MVAR_MAX_WRITERS) {
        printf("Error: escritores debe estar entre 1 y 26.\n");
        return -2;
    }

    if (num_readers <= 0 || num_readers > MVAR_MAX_READERS) {
        printf("Error: lectores debe estar entre 1 y 20.\n");
        return -2;
    }

    int pipe_id = sys_pipe_open_capacity(1);
    if (pipe_id < 0) {
        printf("Error: no se pudo crear el pipe.\n");
        return -2;
    }

    printf("Iniciando MVar con %d escritores y %d lectores.\n",
           num_writers, num_readers);
    printf("Pipe ID: %d\n", pipe_id);

    int created_writers = 0;
    for (int i = 0; i < num_writers; i++) {
        char name[32];
        sprintf(name, "mvar_writer_%c", 'A' + i);
        int pid = spawn_mvar_worker(mvar_writer_entry, name,
                                    pipe_id, 'A' + i, MVAR_WORKER_DELAY_MS);
        if (pid < 0) {
            printf("Error: no se pudo crear el escritor %c.\n", 'A' + i);
        } else {
            created_writers++;
        }
    }

    int created_readers = 0;
    int reader_delay = MVAR_WORKER_DELAY_MS *
                       (num_readers < MVAR_MAX_DELAY_FACTOR ?
                        num_readers : MVAR_MAX_DELAY_FACTOR);

    for (int i = 0; i < num_readers; i++) {
        char name[32];
        sprintf(name, "mvar_reader_%d", i);
        int pid = spawn_mvar_worker(mvar_reader_entry, name,
                                    pipe_id, i, reader_delay);
        if (pid < 0) {
            printf("Error: no se pudo crear el lector %d.\n", i);
        } else {
            created_readers++;
        }
    }

    printf("Creados %d/%d escritores y %d/%d lectores.\n",
           created_writers, num_writers, created_readers, num_readers);
    printf("La simulacion queda corriendo en background.\n");

    return 0;
}

static uint64_t mvar_writer_entry(uint64_t argc, char **raw_args) {
    (void)argc;

    if (raw_args == NULL) {
        return mvar_exit(1);
    }

    MVarWorkerArgs cfg = *(MVarWorkerArgs *)raw_args;
    sys_mem_free(raw_args);

    if (sys_pipe_set_fd((int)sys_getpid(), MVAR_PIPE_FD, cfg.pipe_id, 1) < 0) {
        return mvar_exit(1);
    }

    char value = (char)cfg.worker_id;

    while (1) {
        int written = (int)sys_write(MVAR_PIPE_FD, &value, 1);
        if (written != 1) {
            break;
        }

        sys_yield();
        sleep(cfg.delay_ms);
    }

    return mvar_exit(0);
}

static uint64_t mvar_reader_entry(uint64_t argc, char **raw_args) {
    (void)argc;

    if (raw_args == NULL) {
        return mvar_exit(1);
    }

    MVarWorkerArgs cfg = *(MVarWorkerArgs *)raw_args;
    sys_mem_free(raw_args);

    if (sys_pipe_set_fd((int)sys_getpid(), MVAR_PIPE_FD, cfg.pipe_id, 0) < 0) {
        return mvar_exit(1);
    }

    uint32_t reader_colors[] = {
        0xFF4D4D, 0x00D26A, 0x0096FF, 0xFFD23F, 0xD66BFF,
        0x00E5E5, 0xFF8A00, 0x8AFF00, 0x4D79FF, 0xFF66B3,
        0x66FF99, 0xB366FF, 0xFF3333, 0x33FFCC, 0xCCFF33,
        0xFF33CC, 0x33CCFF, 0xFFCC33, 0x99FF33, 0x3399FF,
    };
    uint32_t color = reader_colors[cfg.worker_id % MVAR_MAX_READERS];

    while (1) {
        char value = '?';

        int read = (int)sys_read(MVAR_PIPE_FD, &value, 1);
        if (read == 1) {
            video_putChar(value, color, 0x000000);
            sys_yield();
            sleep(cfg.delay_ms);
        } else {
            break;
        }
    }

    return mvar_exit(0);
}

static uint64_t mvar_exit(int status) {
    sys_exit(status);
    return (uint64_t)status;
}

static int spawn_mvar_worker(process_func entry, const char *name,
                             int pipe_id, int worker_id, int delay_ms) {
    MVarWorkerArgs *args = (MVarWorkerArgs *)sys_mem_alloc(sizeof(MVarWorkerArgs));
    if (args == NULL) {
        return -1;
    }

    args->pipe_id = pipe_id;
    args->delay_ms = delay_ms;
    args->worker_id = worker_id;

    int pid = (int)sys_create(entry, name, 3, 0, 0, (char **)args);
    if (pid < 0) {
        sys_mem_free(args);
        return -1;
    }

    return pid;
}
