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

#ifdef __cplusplus
}
#endif
