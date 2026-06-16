CROSS_PREFIX ?= $(if $(shell command -v x86_64-linux-gnu-gcc 2>/dev/null),x86_64-linux-gnu-,x86_64-elf-)
GCC = $(CROSS_PREFIX)gcc
LD = $(CROSS_PREFIX)ld
AR = $(CROSS_PREFIX)ar
ASM = nasm

GCCFLAGS=-m64 -fno-exceptions -fno-asynchronous-unwind-tables -mno-mmx -mno-sse -mno-sse2 -fno-builtin-malloc -fno-builtin-free -fno-builtin-realloc -mno-red-zone -Wall -ffreestanding -nostdlib -fno-common -std=c99 -fno-pie
ARFLAGS=rvs
ASMFLAGS=-felf64
LDFLAGS=--warn-common -z max-page-size=0x1000
