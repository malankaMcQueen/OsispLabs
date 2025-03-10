// #include <csignal>
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

#define BUFFER_SIZE 10
#define MAX_STR_LEN 256
#define MAX_CHILD 20




int queueShm;
void *shm_ptr;
const char* queueShmName = "/queueMem";
int prodCount = -1;
int consCount = -1;

int prodPidMas[MAX_CHILD];
int consPidMas[MAX_CHILD];


struct Message{
    char type;
    unsigned short int hash;
    unsigned char size;
    char* data;    
};

volatile sig_atomic_t sigTERMsignalFlag = 0;

void handlerSIGUSR1(int signum) {
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


struct MessageQueue{
    struct Message* buffer;
    char* head;
    char* tail;
    int countAdd;
    int countGet;
    sem_t empty; // Семафор для пустых слотов
    sem_t full;  // Семафор для заполненных слотов
    sem_t mutex; // Мьютекс для доступа к очереди
};
struct MessageQueue* queue;

void init_queue() {
    queue->buffer = (struct Message*)(queue + 1);//sizeof(struct MessageQueue);
    queue->head = (char*)queue->buffer;
    queue->tail = (char*)queue->buffer;
    queue->countAdd = 0;
    queue->countGet = 0;
    sem_init(&queue->empty, 1, BUFFER_SIZE);
    sem_init(&queue->full, 1, 0);
    sem_init(&queue->mutex,1, 1);
}

void workProducer(){
    while(1){
        struct Message* msg = generateMessage();
        // SEND MESSAGE
        sem_wait(&queue->empty);
        if (sigTERMsignalFlag){
            printf("\nCHILD PROCESS HAS BEEN DESTROYED\n");
            exit(0);
        }
        sem_wait(&queue->mutex);
        queue->countAdd++;
        memcpy(queue->tail, msg, sizeof(struct Message));
        queue->tail += sizeof(struct Message);

        memcpy(queue->tail, msg->data, msg->size != 0 ? ( msg->size + 3 ) / 4 * 4 : 256);
        queue->tail += MAX_STR_LEN;

        if (((queue->tail - (char*)queue->buffer) / (MAX_STR_LEN + sizeof(struct Message)) == 10))
            queue->tail = (char*)queue->buffer;
        
        sem_post(&queue->full);
        sem_post(&queue->mutex);

        printf("\nMSG ADD: %d // %d",msg->hash,msg->size); readData(*msg);
        printf("COUNTER ADD: %d\n", queue->countAdd);
        fflush(stdout);
        // sleep(5);
    
    }
}

void createProducer(){
    pid_t chpid = fork();
    if (chpid < 0) {
        // Ошибка при создании дочернего процесса
        fprintf(stderr, "Ошибка при вызове fork()\n");
    }
    if (chpid == 0) {
        setHandlerSIGTERM();
        workProducer();
    }
    prodPidMas[prodCount] = chpid;
}

void workConsumer(){
    while(1){
        // READ MESSAGE
        struct Message* msg = (struct Message*)malloc(sizeof(struct Message));
        sem_wait(&queue->full);
        if (sigTERMsignalFlag){
            printf("\nCHILD PROCESS HAS BEEN DESTROYED\n");
            exit(0);
        sem_wait(&queue->mutex);

        queue->countGet++;
        memcpy(msg, queue->head, sizeof(struct Message));
        memset(queue->head, 0, sizeof(struct Message));
        queue->head += sizeof(struct Message);
        
        int tmpSize = msg->size;
        if (tmpSize == 0)
            tmpSize = 256;
        char* newMsg = (char*)malloc(sizeof(char) * (((tmpSize + 3)/4) * 4));
        msg->data = newMsg;
        memcpy(newMsg, queue->head, (msg->size != 0 ? ( msg->size + 3 ) / 4 * 4 : 256));
        memset(queue->head, 0, (msg->size != 0 ? msg->size : 256));
        queue->head += MAX_STR_LEN;

        if (((queue->head - (char*)queue->buffer) / (MAX_STR_LEN + sizeof(struct Message) ) == 10))
            queue->head = (char*)queue->buffer;

        sem_post(&queue->empty);
        sem_post(&queue->mutex);


        printf("\nMSG READ: %d // %d",msg->hash,msg->size); readData(*msg);
        printf("\nCOUNTER GET: %d\n", queue->countGet);
        
        
        sleep(4);
        }
    }
}

void createConsumer(){
    pid_t chpid = fork();
    if (chpid < 0) {
        fprintf(stderr, "Ошибка при вызове fork()\n");
    }
    if (chpid == 0) {
        workConsumer();
    }
    consPidMas[consCount] = chpid;
}

bool destroyChildProcess(int* childPidMas, int childCount){
    if (childCount != -1){
        kill(childPidMas[childCount],SIGUSR1);
        return true;
    }
    else {
        printf("There are no child processes\n");
        return false;
    }
}


int main() {
    setHandlerSIGTERM();

    srand(time(NULL));
    queueShm = shm_open(queueShmName, O_CREAT | O_RDWR, 0666);
    if (queueShm == -1) {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(queueShm, sizeof(struct MessageQueue) + sizeof(struct Message) + (MAX_STR_LEN * BUFFER_SIZE) ) == -1) {
        perror("ftruncate");
        exit(1);
    }

    queue = mmap(NULL, sizeof(struct MessageQueue) + (sizeof(struct Message) + MAX_STR_LEN * BUFFER_SIZE),
     PROT_READ | PROT_WRITE, MAP_SHARED, queueShm, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    init_queue();
    
    char symbols[20];
    while (1){
        printf(" 'p+' - create producer\n'p-' - destroy producer\n'c+' - create consumer\n'c-' - destroy consumer\ni-info\nq-end\n");
        fgets(symbols, 20, stdin);
        switch (symbols[0])
        {
        case 'p':
            if(symbols[1] == '+'){
                ++prodCount;
                createProducer();
            }
            else if (symbols[1] == '-'){
                if (destroyChildProcess(prodPidMas,prodCount))
                --prodCount;
            }
            break;
        case 'c':
            if(symbols[1] == '+'){
                ++consCount;
                createConsumer();
            }
            else if (symbols[1] == '-'){
                if (destroyChildProcess(consPidMas,consCount))
                    --consCount;
            }
            break;
        case 'i':
            printf("\nNumber of producer: %d", prodCount+1);
            printf("\nNumber of consumer: %d", consCount+1);
            printf("\nQueue size: %d", BUFFER_SIZE);
            int fullValue;
            int emptyValue;
            sem_getvalue(&queue->full, &fullValue);
            sem_getvalue(&queue->empty, &emptyValue);
            printf("\nFull count: %d",fullValue);
            printf("\nEmpty count: %d",emptyValue);


        }

    if (symbols[0] == 'q')
        break;
    }
    close(queueShm);
    shm_unlink(queueShmName);
    return 0;
}
