CC=g++
CC_FLAGS=-std=c++0x -Wall -v

all:clear ctags naive
 
debug:CC_FLAGS += -g -DDEBUG
debug:clear ctags naive

naive:src/naive_main.c
	${CC} ${CC_FLAGS} -o bin/naive.bin src/naive_main.c

ctags:
	ctags -R *

clear:
	clear

clean:
	rm -rvf bin/*
