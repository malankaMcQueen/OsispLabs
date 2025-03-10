#include <fcntl.h>
#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>


#define MAX_THREADS 128 // Максимальное количество потоков


int *map;                   // Карта отсортированных блоков
pthread_mutex_t map_mutex; // Мьютекс для доступа к карте
pthread_barrier_t barrier; // Барьер для синхронизации
int blockSize;                 // Размер блока
const size_t BUFFER_SIZE = 333;         // Размер буфера
char* tmpStart;
int blockNum;             // Количество блоков
int* blockMap;
long memsize;
int threads;

// Структура для передачи аргументов в потоки
typedef struct {
    int thread_id;              // Идентификатор потока
    char *buffer;           // Адрес буфера
    int memsize;                // Размер блока
} ThreadArgs;

typedef struct {
    double time_mark; // временная метка (модифицированная юлианская дата)
    uint64_t recno;   // первичный индекс в таблице БД
} index_s;

// Структура заголовка индексного файла
typedef struct {
    uint64_t records;    // количество записей
    index_s *idx;   // массив записей
} index_hdr_s;



int compare_index_s(const void *a, const void *b) {
    double time_markA = ((index_s *)a)->time_mark;
    double time_markB = ((index_s *)b)->time_mark;

    if (time_markA < time_markB) return -1;
    if (time_markA > time_markB) return 1;
    return 0;
}

int find_next_free_block(int current_block) {
    // printf("%d", current_block);
    for (int i = 0; i < blockNum; i++) {
        printf("blockMap[%d] = %d\n", i, blockMap[i]);
        if (blockMap[i] == 0) {
            return i;
        }
    }
    return -1; // Все блоки отсортированы
}


void merge(index_s *start1, index_s *start2, int count) {
    index_s *temp = malloc(count * 2 * sizeof(index_s));
    int i = 0, j = 0, k = 0;
    while (i < count && j < count) {
        if (start1[i].time_mark < start2[j].time_mark) {
            temp[k++] = start1[i++];
        } else {
            temp[k++] = start2[j++];
        }
    }
    while (i < count) {
        temp[k++] = start1[i++];
    }
    while (j < count) {
        temp[k++] = start2[j++];
    }

    memcpy(start1, temp, count * 2 * sizeof(index_s));
    free(temp);
}

void merge_all(index_s *tmpStart, int blocks, int blockSize) {
    int mergeSize = blockSize;
    while (blocks > 1) {
        for (int i = 0; i < blocks / 2; i++) {
            merge(tmpStart + i * 2 * mergeSize, tmpStart + (i * 2 + 1) * mergeSize, mergeSize);
        }
        blocks /= 2;
        mergeSize *= 2;
    }
}

// Функция для потока сортировки
void *sort_thread(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    int threadId = threadArgs->thread_id;

    printf("thread_id = %d WAIT BARIER\n", threadId);
    pthread_barrier_wait(&barrier);
    printf("thread_id = %d GO BARIER\n", threadId);
    
    char* startSort = tmpStart + (threadId) * blockSize;
    qsort(startSort, blockSize / sizeof(index_s), sizeof(index_s), compare_index_s);
    int next_block = 0;
    while (1) {
        pthread_mutex_lock(&map_mutex);
        next_block = find_next_free_block(next_block);
        printf("thread_id = %d, next_block = %d\n", threadId, next_block);
        if (next_block == -1) {
            printf("WAIT MERGE\n");
            fflush(stdout);
            pthread_mutex_unlock(&map_mutex);
            pthread_barrier_wait(&barrier);
            break;
        }
        blockMap[next_block] = 1;
        pthread_mutex_unlock(&map_mutex);
        startSort = tmpStart + next_block * blockSize;
        qsort(startSort, blockSize / sizeof(index_s), sizeof(index_s), compare_index_s);
        printf("qsort end\n");
        fflush(stdout);
    }

    // Фаза слияния
    int merge_blocks = blockNum;
    int tmpBlockSize = blockSize;
    while (merge_blocks > 1) {
        for (int tmpId = threadId; tmpId < (merge_blocks / 2); tmpId += threads){
            if (threadId < merge_blocks / 2) { 
                int block1 = tmpId * 2;
                int block2 = block1 + 1;

                char* startMerge1 = tmpStart + block1 * tmpBlockSize;
                char* startMerge2 = tmpStart + block2 * tmpBlockSize;
                
                merge((index_s*)startMerge1,(index_s*)startMerge2, tmpBlockSize / sizeof(index_s));
            }
        }
        pthread_barrier_wait(&barrier);
        tmpBlockSize *= 2;
        merge_blocks /= 2;
    }
    
    pthread_barrier_wait(&barrier);
    return NULL;
}


void lastMerge(char* start, long filesize){
    int merge_blocks = (filesize - sizeof(index_hdr_s)) / (memsize * sizeof(index_s));
    int tmpBlockSize = memsize * sizeof(index_s);
    while (merge_blocks > 1) {
        for (int tmpBlockId = 0; tmpBlockId < (merge_blocks / 2); ++tmpBlockId){ 
            int block1 = tmpBlockId * 2;
            int block2 = block1 + 1;

            char* startMerge1 = start + block1 * tmpBlockSize;
            char* startMerge2 = start + block2 * tmpBlockSize;
                
            merge((index_s*)startMerge1,(index_s*)startMerge2, tmpBlockSize / sizeof(index_s));
        }
        tmpBlockSize *= 2;
        merge_blocks /= 2;
    }
}




int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "NEED 5 ARGS: memsize granul threads filename\n");
        exit(EXIT_FAILURE);
    }
    memsize = atoi(argv[1]);
    blockNum = atoi(argv[2]);
    // int threads = atoi(argv[3]);
    threads = atoi(argv[3]);
    char *filename = argv[4];

    int fd = open(filename, O_RDWR | O_CREAT , 0666);
    struct stat st;
    fstat(fd, &st);
    long filesize = st.st_size; // размер файла
    long countSortIndex = 0;
    if (memsize * sizeof(index_s) > filesize - sizeof(index_hdr_s)) {
        fprintf(stderr, "memsize must be less than or equal to the file size\n");
        exit(EXIT_FAILURE);
    }
    if (memsize % blockNum != 0) {
        exit(EXIT_FAILURE);
        // memsize = (memsize / blockNum) * blockNum;
    }
    blockSize = memsize/blockNum * sizeof(index_s);
    if(blockSize % sizeof(index_s) != 0) {
        exit(EXIT_FAILURE);
        // blockSize = (blockSize / sizeof(index_s)) * sizeof(index_s);
        // memsize = blockSize * blockNum;
    }

    blockMap = malloc(blockNum * sizeof(int));
    

    // Проверяем, что количество потоков находится в допустимом диапазоне
    // if (threads < 8 || threads > MAX_THREADS) {
    //     fprintf(stderr, "Number of threads must be between 1 and %d\n", MAX_THREADS);
    //     exit(EXIT_FAILURE);
    // }
    pthread_barrier_init(&barrier, NULL, threads);
    pthread_mutex_init(&map_mutex, NULL);

    // Создаем потоки
    pthread_t threadPids[MAX_THREADS];
    ThreadArgs threadArgs[MAX_THREADS];
    char threadBuffer[MAX_THREADS][BUFFER_SIZE];
    // Запускаем потоки
    char* start;
    start = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    tmpStart = start + sizeof(index_hdr_s);
    do {
    for (int i = 0; i < blockNum; i++) {
        blockMap[i] = 0;
    }
    for (int i = 1; i < threads; ++i) {
        threadArgs[i].thread_id = i;
        threadArgs[i].buffer = threadBuffer[i];
        threadArgs[i].memsize = memsize;        ////////////
        blockMap[i] = 1;
        pthread_create(&threadPids[i], NULL, sort_thread, (void *)&threadArgs[i]);
    }
    
    // tmpStart = mmap(NULL, memsize * sizeof(index_s) + sizeof(index_hdr_s), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    blockMap[0] = 1;
    threadArgs[0].thread_id = 0;
    threadArgs[0].buffer = tmpStart;
    threadArgs[0].memsize = memsize;
    sort_thread(&threadArgs[0]);
    countSortIndex += memsize;
    for (int i = 1; i < threads; ++i) {
        pthread_join(threadPids[i], NULL);
    }
    tmpStart += sizeof(index_s) * memsize;
    } while (countSortIndex != ((filesize - sizeof(index_hdr_s)) / sizeof(index_s)));
    if (memsize * sizeof(index_s) != filesize - sizeof(index_hdr_s))
        lastMerge(start + sizeof(index_hdr_s),filesize);
    munmap(start, filesize);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&map_mutex);

    return 0;
}
