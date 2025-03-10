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
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <records>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));

    const char *filename = argv[1];
    uint64_t num_records = strtoull(argv[2], NULL, 10);

    // Открываем файл для записи
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    // Генерируем записи и записываем их в файл
    index_hdr_s header;
    header.records = num_records;
    header.idx = malloc(num_records * sizeof(index_s));
    if (header.idx == NULL) {
        perror("malloc");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for (uint64_t i = 0; i < num_records; ++i) {
        header.idx[i].time_mark = rand() % 36525 + (rand() % 24) / 24.0;
        header.idx[i].recno = i + 1;
    }

    fwrite(&header, sizeof(index_hdr_s), 1, file);
    fwrite(header.idx, sizeof(index_s), num_records, file);

    free(header.idx);
    fclose(file);


    return 0;
}
