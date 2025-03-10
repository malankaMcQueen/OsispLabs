#include "functions.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef struct {
    struct Message* buffer;
    int head;
    int tail;
    int countAdd;
    int countGet;
    pthread_cond_t empty; // Семафор для пустых слотов
    pthread_cond_t full;  // Семафор для заполненных слотов
    int countMsg;
    pthread_mutex_t mutex; // Мьютекс для доступа к очереди
} MessageQueue;

MessageQueue queue;

void cleanup_handler() {
    pthread_mutex_unlock(&queue.mutex);
}

void init_queue() {
    queue.buffer = (struct Message*)malloc(sizeof(struct Message) * sizeQueue);
    queue.head = 0;
    queue.tail = 0;
    queue.countAdd = 0;
    queue.countGet = 0;
    queue.countMsg = 0;
    pthread_cond_init(&queue.empty, NULL);
    pthread_cond_init(&queue.full, NULL);
    pthread_mutex_init(&queue.mutex, NULL);
}

void* workProducer(){
    pthread_cleanup_push(cleanup_handler, NULL);
    while(1){
        struct Message* msg = generateMessage();
        
        pthread_mutex_lock(&queue.mutex);
        
        while(queue.countMsg >= sizeQueue)
            pthread_cond_wait(&queue.empty, &queue.mutex);
        
        queue.countAdd++;
        memcpy((queue.buffer + queue.tail), msg, sizeof(struct Message));
        ++queue.tail;

        if (queue.tail == sizeQueue)
            queue.tail = 0;
        
        ++queue.countMsg;
        pthread_cond_signal(&queue.full);
        pthread_mutex_unlock(&queue.mutex);

        printf("\nMSG ADD: %d // %d",msg->hash ,msg->size);
        readData(*msg);
        printf("COUNTER ADD: %d\n", queue.countAdd);
        fflush(stdout);
        sleep(5);
    }
    pthread_cleanup_pop(1);
}

void createProducer(int prodCount){
    int flError = pthread_create(prodIdMas + prodCount, NULL, workProducer, NULL);
    if (flError) {
        printf("Ошибка  при создании потока \n");
    }
}

void* workConsumer(){
    pthread_cleanup_push(cleanup_handler, NULL);
    while(1){
        struct Message* msg = (struct Message*)malloc(sizeof(struct Message));

        pthread_mutex_lock(&queue.mutex);
        while(queue.countMsg == 0)
            pthread_cond_wait(&queue.full, &queue.mutex);

        queue.countGet++;
        memcpy(msg, (queue.buffer + queue.head), sizeof(struct Message));
        memset(queue.buffer + queue.head, 0, sizeof(struct Message));
        ++queue.head;

        if (queue.head == sizeQueue)
            queue.head = 0;
        pthread_cond_signal(&queue.empty);

        --queue.countMsg;
        pthread_mutex_unlock(&queue.mutex);

        printf("\nMSG READ: %d // %d",msg->hash,msg->size);
        readData(*msg);
        printf("\nCOUNTER GET: %d\n", queue.countGet);
        sleep(4);
    }
    pthread_cleanup_pop(1); // 1 указывает, что нужно выполнить функцию очистки
}

void createConsumer(int consCount){
    int flError = pthread_create(consIdMas + consCount, NULL, workConsumer, NULL);
    if (flError) {
        printf("Ошибка  при создании потока \n");
    }
}

bool destroyThread(pthread_t* childIdMas, int idThread){
    if (idThread != -1){
        pthread_cancel(childIdMas[idThread]);
        pthread_join(childIdMas[idThread], NULL);
        return true;
    }
    else {
        printf("There are no child processes\n");
        return false;
    }
}

void incremSize(){
    pthread_mutex_lock(&queue.mutex);
    sizeQueue++;
    queue.buffer = realloc(queue.buffer, sizeQueue * sizeof(struct Message));
    pthread_cond_signal(&queue.empty);
    if (queue.head > queue.tail){
        for(int i = sizeQueue - 1; i > queue.head; i--)
            queue.buffer[i] = queue.buffer[i - 1];
        ++queue.head;
    }
    pthread_mutex_unlock(&queue.mutex);
}

void discrSize(){
    pthread_mutex_lock(&queue.mutex);
    // int countMsg;
    // sem_getvalue(&queue.full, &countMsg);
    if (queue.countMsg < sizeQueue){
        if (queue.head < queue.tail){    // Записить больше чтения
            for(int i = 0; queue.tail - queue.head - i > 0; i++)
                queue.buffer[i] = queue.buffer[queue.head + i];
            while (queue.head != 0){
                --queue.head;
                --queue.tail;
            }
        }
        else if (queue.head == queue.tail){
            queue.head = queue.tail = 0;
        }
        else if (queue.head > queue.tail){    // Записить меньше чтения
            for(int i = 0; queue.head + i < sizeQueue; i++){
                queue.buffer[queue.head + i - 1] = queue.buffer[queue.head + i];
            }
            --queue.head;
        }
        queue.buffer = realloc(queue.buffer, --sizeQueue * sizeof(struct Message));
        // sem_wait(&queue.empty);      
    }
    // queue.countMsg;
    pthread_mutex_unlock(&queue.mutex);
}

int main() {
    srand(time(NULL));
    init_queue();
    int prodCount = -1;
    int consCount = -1;
    char symbols[20];
    while (1){
        printf(" 'p+' - create producer\n'p-' - destroy producer\n'c+' - create consumer\n'c-' - destroy consumer\ni-info\nq-end\n");
        fgets(symbols, 20, stdin);
        switch (symbols[0])
        {
        case 'p':
            if(symbols[1] == '+'){
                ++prodCount;
                createProducer(prodCount);
            }
            else if (symbols[1] == '-'){
                if (destroyThread(prodIdMas,prodCount))
                --prodCount;
            }
            break;
        case 'c':
            if(symbols[1] == '+'){
                ++consCount;
                createConsumer(consCount);
            }
            else if (symbols[1] == '-'){
                if (destroyThread(consIdMas,consCount))
                    --consCount;
            }
            break;
        case 's':
            if (symbols[1] == '+')
                incremSize();
            else if (symbols[1] == '-')
                discrSize();
            break;
        case 'i':
            printf("\nNumber of producer: %d", prodCount + 1);
            printf("\nNumber of consumer: %d", consCount + 1);
            printf("\nQueue size: %d", sizeQueue);
            printf("\nCount messages: %d", queue.countMsg);
            // int fullValue;
            // int emptyValue;
            // sem_getvalue(&queue.full, &fullValue);
            // sem_getvalue(&queue.empty, &emptyValue);
            // printf("\nFull count: %d",fullValue);
            // printf("\nEmpty count: %d",emptyValue);


        }
    if (symbols[0] == 'q')
        break;
    }
    return 0;
}
