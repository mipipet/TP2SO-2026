CROSS_PREFIX ?= $(if $(shell command -v x86_64-linux-gnu-gcc 2>/dev/null),x86_64-linux-gnu-,x86_64-elf-)
GCC = $(CROSS_PREFIX)gcc
GPP = $(CROSS_PREFIX)g++
LD = $(CROSS_PREFIX)ld
AR = $(CROSS_PREFIX)ar
ASM = nasm

GCCFLAGS=-m64 -fno-exceptions -std=c99 -Wall -ffreestanding -nostdlib -fno-common -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-builtin-malloc -fno-builtin-free -fno-builtin-realloc -fno-pie
ARFLAGS=rvs
ASMFLAGS=-felf64
