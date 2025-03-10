#include "functions.h"
#include <bits/pthreadtypes.h>



volatile sig_atomic_t sigTERMsignalFlag = 0;

pthread_t prodIdMas[MAX_CHILD];
pthread_t consIdMas[MAX_CHILD];

int sizeQueue = 10;

unsigned char generateRandSize(){
    short int randSize;
    for(randSize = rand() % 257; randSize == 0; );
    if (randSize == 256)
        randSize = 0;
    return randSize;
}

char* generateDataMessage(short int size){
    if (size == 0)
        size = 256;
    char* newMsg = (char*)malloc(sizeof(char) * (((size + 3)/4) * 4));
    for(int i = 0; i < ((size + 3)/4) * 4; i++){
        newMsg[i] = 33 + rand() % 93;
    }
    return newMsg;
}

void readData(struct Message msg){
    printf("\n");
    for (int i = 0; i < (msg.size != 0 ? ((msg.size + 3) / 4 * 4) : 256); i++){
        printf("%c", msg.data[i]);
    }
    printf("\n");
}

struct Message* generateMessage(){
    srand(time(NULL));
    struct Message* newMessage = (struct Message*)malloc(sizeof(struct Message*));
    newMessage->type = 'S';
    newMessage->size = generateRandSize();
    newMessage->data = generateDataMessage(newMessage->size);
    unsigned short int hash = newMessage->type + newMessage->size;
    for (int i = 0; i < (newMessage->size != 0 ? ((newMessage->size + 3) / 4 * 4) : 256); i++){
        hash += newMessage->data[i];
    }
    newMessage->hash = hash;
    return newMessage;
}



void handlerSIGUSR1(__attribute__((unused)) int signum) {
    sigTERMsignalFlag = 1;
    printf("\nworkSIgnal\n");
}

void setHandlerSIGTERM(){
    struct sigaction saSIGUSR1;
    saSIGUSR1.sa_handler = handlerSIGUSR1;
    if (sigaction(SIGUSR1, &saSIGUSR1, NULL) == -1) {
        perror("sigaction SIGTUSR1");
        exit(EXIT_FAILURE);
    }
}
