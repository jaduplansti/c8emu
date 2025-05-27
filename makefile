CC = gcc
CFLAGS = -Wall -g -fsanitize=address

c8emu: src/main.c src/emulator.c
	${CC} ${CFLAGS} -o $@ $^
	
run: c8emu
	./c8emu