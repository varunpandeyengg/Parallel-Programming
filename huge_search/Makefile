CC=g++
CC_FLAGS=-std=c++0x -Wall -v

all:clear ctags naive rabinkarp
 
debug:CC_FLAGS += -g -DDEBUG
debug:clear ctags naive rabinkarp

naive:src/naive_main.c	
	${CC} ${CC_FLAGS} -o bin/naive.bin src/naive_main.c

rabinkarp: CC_FLAGS += -pthread -lrt
rabinkarp: src/rk_threaded_main.c
	${CC} ${CC_FLAGS} -o bin/rk_threaded_main.bin src/rk_threaded_main.c

ctags:
	ctags -R *

clear:
	clear

clean:
	rm -rvf bin/*
