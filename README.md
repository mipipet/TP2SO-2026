# TP2 SO 2026 - Group 9

x86_64 monolithic kernel with separated userland, preemptive multitasking,
priority-based Round Robin scheduling, dynamic memory management, semaphores,
blocking pipes, and a user shell.

## Build And Run

The expected build environment is the Docker image provided by the course:

```sh
docker pull agodio/itba-so-multiarch:3.1
```

From the repository root:

```sh
make all
```

By default, the project builds the free-list memory manager
(`Kernel/memory/memManager.c`). To build using the Buddy System memory manager:

```sh
make buddy
```

To remove generated artifacts:

```sh
make clean
```

To run the image:

```sh
./run.sh
```

If `make all` fails while generating `x64BareBonesImage.qcow2` because of a
`qemu-img` lock, close the VM/QEMU process using that image and build again.

## Structure

- `Kernel/`: kernel, syscalls, scheduler, processes, memory, semaphores,
  pipes, drivers, and interrupts.
- `Kernel/memory/`: interchangeable memory manager implementations.
- `Userland/SampleCodeModule/shell/`: shell and command parser.
- `Userland/SampleCodeModule/apps/`: user applications.
- `Userland/SampleCodeModule/tests/`: course tests adapted to this project's
  syscalls.
- `Bootloader/`, `Toolchain/`, `Image/`: boot support, module packing, and the
  final image.

## Shell

The shell can run commands in foreground or background. To run a command in
background, append `&`:

```sh
loop &
test_sync 50000 1 &
```

The shell supports one pipe between two commands using `|`:

```sh
cat | wc
cat | filter
```

Keyboard shortcuts:

- `Ctrl+C`: kills the foreground process, unless it is the shell.
- `Ctrl+D`: sends EOF to the process reading from stdin.

## Commands

| Command      | Parameters                | Description |
| ------------ | --------------------------| --- |
| `help`       | none                      | Lists commands and tests. |
| `clear`      | none                      | Clears the screen. |
| `time`       | none                      | Shows the system time. |
| `font-size`  | interactive               | Changes the font size. |
| `exceptions` | `zero` or `invalidOpcode` | Triggers test exceptions. |
| `regs`       | none                      | Shows the latest register snapshot. |
| `mem`        | none                      | Shows the active memory manager, total, used, free memory, and used percentage. |
| `ps`         | none                      | Lists processes with PID, priority, foreground, stack, and state. |
| `loop`       | none                      | Prints its PID periodically using active waiting. |
| `kill`       | `<pid>`                   | Kills a process by PID. |
| `nice`       | `<pid> <priority>`        | Changes process priority. Valid values: 1 to 5. |
| `block`      | `<pid>`                   | Blocks a process. |
| `unblock`    | `<pid>`                   | Unblocks a process. |
| `cat`        | none                      | Prints stdin exactly as received. |
| `wc`         | none                      | Counts input lines. |
| `filter`     | none                      | Filters vowels from stdin. |
| `mvar`       | `<writers> <readers>`     | Simulates an MVar with writers and readers. |

## Tests

The tests run as user processes, not as shell built-ins. They can be executed in foreground or background.

| Test        | Parameters        | Expected result                                      |
| ----------- | ----------------- | ---------------------------------------------------- |
| `test_mm`   | `<max_bytes>`     | Runs forever. It only prints if it detects an error. |
| `test_proc` | `<max_processes>` | Runs forever, randomly creating, blocking, unblocking, and killing dummy processes. It only prints errors. |
| `test_sync` | `<n> <use_sem>`   | With `use_sem=1`, it must end with `Final value: 0`; with `use_sem=0`, the result may vary. |
| `test_prio` | `<max_value>`     | Shows three phases to compare processes with equal and different priorities. |

Examples:

```sh
test_sync 1000 0
test_sync 1000 1
test_sync 50000 1
test_mm 4096 &
test_proc 4 &
test_prio 100000
```

## Requirement Examples

Physical memory:

```sh
mem
test_mm 4096 &
ps
kill <test_mm_pid>
```

Processes, context switching, and scheduling:

```sh
loop &
loop &
ps
nice <pid> 5
block <pid>
unblock <pid>
kill <pid>
test_prio 100000
```

Synchronization:

```sh
test_sync 10000 0
test_sync 10000 1
```

IPC and pipes:

```sh
cat | wc
cat | filter
```

MVar:

```sh
mvar 2 2
ps
kill <writer_pid>
kill <reader_pid>
```

`mvar` creates readers and writers in background and returns immediately. The shared variable is modeled with a capacity-1 pipe: if it is full, writers block; if it is empty, readers block. Each writer produces a unique letter (`A`, `B`, `C`, etc.), and each reader prints using its own color.

## Missing Or Partial Requirements

No missing mandatory requirements were identified. The shell implements one pipe between two processes, which is the scope required by the assignment.

## Limitations

- `test_mm` and `test_proc` are infinite tests and stay silent while they work correctly.
- `test_sync` uses two fixed process pairs and accepts `<n> <use_sem>` according to the test version integrated into this project.
- `mvar` limits writers to 26 and readers to 20 to keep letter and color assignment simple.
- `kill` kills the selected process, but it does not automatically kill its children. If a parent process is killed after creating children, those children must be cleaned up with `kill` or by restarting the VM.

## References And AI Use

This project is based on x64BareBones and uses Pure64/BMFS for boot support.
The MVar concept is based on Haskell's abstraction:
https://hackage.haskell.org/package/base/docs/Control-Concurrent-MVar.html

AI assistance (ChatGPT/Codex) was used during debugging and code cleanup as a PVS-Studio-style reviewer: it helped inspect suspicious control flows, scheduler and synchronization edge cases, foreground/background process issues, and test behavior inconsistencies. It also helped translate this README to English.
