#pragma once
#include <stddef.h>   // size_t
#include <stdint.h>   // привычно держать рядом, пригодится в твоих экспериментах

#ifdef __cplusplus
extern "C" {
#endif

/* Сумма первых n элементов массива a (a[0] + a[1] + ... + a[n-1]).
   Для n == 0 сумма равна 0. */
int sum_first_n(const int* a, int n);

/* Максимальный элемент среди первых n элементов массива a.
   Гарантируется, что n >= 1 (массив непустой). */
int max_in(const int* a, int n);

/* Развернуть первые n элементов массива a НА МЕСТЕ:
   было {1,2,3,4} -> станет {4,3,2,1}. Ничего не возвращает — меняет сам массив. */
void reverse_array(int* a, int n);

/* Сколько раз символ c встречается в си-строке s (до завершающего '\0'). */
size_t count_char(const char* s, char c);

#ifdef __cplusplus
}
#endif
