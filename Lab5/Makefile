CC = gcc
CFLAGS = -Wall -Wextra -pthread -lrt

all: lab5_1 lab5_2

lab5_1: lab5_1.c functions.c
	$(CC) $(CFLAGS) -o lab5_1 lab5_1.c functions.c

lab5_2: lab5_2.c functions.c
	$(CC) $(CFLAGS) -o lab5_2 lab5_2.c functions.c

run: lab5_2
	./lab5_2

clean:
	rm -f lab5_1 lab5_2
