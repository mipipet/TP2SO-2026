include ../Makefile.inc

MODULE=0000-sampleCodeModule.bin
SOURCES=$(wildcard [^_]*.c) $(wildcard shell/*.c)
ASM_SOURCES=asm/syscall.asm asm/commands.asm
ASM_OBJECTS=$(ASM_SOURCES:.asm=.o)

all: $(MODULE)

%.o: %.asm
	nasm -f elf64 $< -o $@

$(MODULE): $(SOURCES) $(ASM_OBJECTS)
	$(GCC) $(GCCFLAGS) -I./include -T sampleCodeModule.ld _loader.c $(SOURCES) $(ASM_OBJECTS) -o ../$(MODULE)

clean:
	rm -rf *.o
	rm -rf asm/*.o

.PHONY: all clean print
