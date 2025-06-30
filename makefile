CC = gcc
CFLAGS = -Wall -g -fsanitize=address

all: c8emu c8asm

c8emu: src/emulator/main.c src/emulator/emulator.c
	${CC} ${CFLAGS} -o $@ $^

c8asm: src/assembler/main.c src/assembler/assembler.c
	${CC} ${CFLAGS} -o $@ $^

run: c8emu
	./c8emu

clean:
	rm c8emu c8asm