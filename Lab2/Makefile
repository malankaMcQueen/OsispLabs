CC=gcc
CFLAGS=-D_POSIX_C_SOURCE=200809L -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic

.PHONY: clean

export CHILD_PATH=./child

all: lab2 child run

lab2: lab2.c
	$(CC) $(CFLAGS) -o lab2 lab2.c
	
child: childProgramm.c
	$(CC) $(CFLAGS) -o child childProgramm.c 

run: lab2
	./lab2 text.txt
