#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h> // константы для mode
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>


#define BUFFER_SIZE 10
#define MAX_STR_LEN 256
#define MAX_CHILD 20

struct Message{
    char type;
    unsigned short int hash;
    unsigned char size;
    char* data;    
};

extern pthread_t prodIdMas[];
extern pthread_t consIdMas[];
extern volatile sig_atomic_t sigTERMsignalFlag;
extern int sizeQueue;

unsigned char generateRandSize();
char* generateDataMessage(short int size);
void readData(struct Message msg);
struct Message* generateMessage();

void handlerSIGUSR1(__attribute__((unused)) int signum);
void setHandlerSIGTERM();

// Ваши прототипы функций здесь

#endif // FUNCTIONS_H
