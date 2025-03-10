CC=gcc
CFLAGS=-D_POSIX_C_SOURCE=200809L -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic

.PHONY: clean

export CHILD_PATH=./child

all: lab3 child run

lab3: lab3.c
	$(CC) $(CFLAGS) -o lab3 lab3.c
	
child: child.c
	$(CC) $(CFLAGS) -o child child.c 

run: lab3
	./lab3 
