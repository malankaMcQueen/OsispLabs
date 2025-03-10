#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// Структура для представления записи в индексном файле
typedef struct {
    double time_mark; // временная метка (модифицированная юлианская дата)
    uint64_t recno;   // первичный индекс в таблице БД
} index_s;

// Структура заголовка индексного файла
typedef struct {
    uint64_t records;    // количество записей
    index_s *idx;   // массив записей
} index_hdr_s;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *filename = argv[1];
    // Открываем файл для чтения
    FILE *file = fopen(filename, "rb+");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    index_hdr_s header;
    fread(&header, sizeof(index_hdr_s), 1, file);
    header.idx = malloc(header.records * sizeof(index_s));
    if (header.idx == NULL) {
        perror("malloc");
        fclose(file);
        exit(EXIT_FAILURE);
    }
    fread(header.idx, sizeof(index_s), header.records, file);
    
    printf("%lu\n", header.records);
    for (uint64_t i = 0; i < header.records; ++i) {
        printf("%f %lu\n", header.idx[i].time_mark, header.idx[i].recno);
    }

    free(header.idx);
    fclose(file);


    return 0;
}
