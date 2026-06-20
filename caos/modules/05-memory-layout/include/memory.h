#pragma once
#include <stddef.h>   // size_t
#include <stdint.h>   // не обязательно здесь, но привычно держать рядом

#ifdef __cplusplus
extern "C" {
#endif

/* Округлить n ВВЕРХ до ближайшего кратного align.
   align — степень двойки (1, 2, 4, 8, 16, ...).
   Примеры: align_up(13, 8) == 16; align_up(16, 8) == 16; align_up(0, 8) == 0. */
size_t align_up(size_t n, size_t align);

/* Вернуть 1, если адрес p кратен align (то есть p — "выровненный"), иначе 0.
   align — степень двойки. */
int is_aligned(const void* p, size_t align);

/* Вернуть указатель на элемент base[n] (то есть base + n) без разыменования. */
int* nth_element(int* base, size_t n);

/* Вернуть расстояние между указателями в ЭЛЕМЕНТАХ (int), а не в байтах: a - b.
   Может быть отрицательным, если a левее b. */
long ptr_distance(const int* a, const int* b);

/* -------------------------------------------------------------------------
 * Задание 5: инспектор раскладки структур (struct layout inspector)
 * -------------------------------------------------------------------------
 * Field — дескриптор одного поля: имя (только для отладки), размер в байтах
 * и требование выравнивания (степень двойки, например 1/2/4/8).
 */
typedef struct {
    const char* name;   /* имя поля (не используется в вычислениях) */
    size_t      size;   /* sizeof поля в байтах                     */
    size_t      align;  /* alignof поля (степень двойки)            */
} Field;

/* Вычислить раскладку «виртуальной» структуры, заданной массивом из n дескрипторов.
 *
 * Для каждого поля fields[i] вычисляется смещение с учётом natural-alignment padding
 * и записывается в offsets_out[i].
 *
 * Возвращает полный sizeof структуры — включая хвостовой padding до строжайшего
 * (максимального) требования выравнивания среди всех полей.
 *
 * Контракт:
 *   - fields != NULL, n >= 1, offsets_out != NULL.
 *   - Каждый fields[i].align — степень двойки >= 1.
 *   - fields[i].size > 0.
 *
 * Пример: fields = {{"c",1,1}, {"n",4,4}}, n=2
 *   -> offsets_out = {0, 4}, return 8   (struct { char c; int n; })
 */
size_t layout_compute(const Field* fields, int n, size_t* offsets_out);

#ifdef __cplusplus
}
#endif
