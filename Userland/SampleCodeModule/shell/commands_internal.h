#ifndef COMMANDS_INTERNAL_H
#define COMMANDS_INTERNAL_H

int helpCmd(int argc, char *argv[]);
int exitCmd(int argc, char *argv[]);
int setUserCmd(int argc, char *argv[]);
int clearCmd(int argc, char *argv[]);
int timeCmd(int argc, char *argv[]);
int fontSizeCmd(int argc, char *argv[]);
int exceptionCmd(int argc, char *argv[]);
int regsCmd(int argc, char *argv[]);

int test_sync_cmd(int argc, char *argv[]);
int test_mm_cmd(int argc, char *argv[]);
int test_proc_cmd(int argc, char *argv[]);
int test_prio_cmd(int argc, char *argv[]);

#endif