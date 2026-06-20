#pragma once
// include/warmup.h — интерфейс модуля 00 на C.
// Обёртка extern "C" нужна, чтобы C++-тест (test_warmup.cpp) увидел эти
// функции под их «настоящими» C-именами, а не под искажёнными C++-именами.

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Возвращает сумму a и b. */
int add(int a, int b);

/* Возвращает число секунд в указанном количестве часов.
   Тип long — потому что секунд может быть много (часы * 3600). */
long seconds_in(int hours);

/* ---- Задание 3: байты, структуры и порядок памяти ---- */

/* Структура с полями разных размеров.
   Компилятор вставляет padding (дырки) для выравнивания — это интересно! */
struct PackedRecord {
    uint8_t  a;   /* 1 байт  */
    uint32_t b;   /* 4 байта — выравнивание по 4: перед b будет 3 байта padding */
    uint16_t c;   /* 2 байта */
};

/* Возвращает размер структуры PackedRecord, вычисленный через offsetof плюс
   sizeof последнего поля (а не через sizeof напрямую).
   Результат совпадёт с sizeof(struct PackedRecord), включая финальный padding. */
size_t packed_record_size(void);

/* Раскладывает 32-битное целое по байтам в порядке хранения в памяти.
   out[0] — байт по наименьшему адресу (на x86/ARM little-endian это младший байт).
   Пример: uint32_to_bytes(0x01020304, out) → out = {0x04, 0x03, 0x02, 0x01}
   на little-endian машине. */
void uint32_to_bytes(uint32_t value, unsigned char out[4]);

/* Возвращает 1, если машина little-endian; 0 — если big-endian.
   Реализация через union или указатель; без __BYTE_ORDER__ и системных макросов. */
int is_little_endian(void);

#ifdef __cplusplus
}
#endif
