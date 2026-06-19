#pragma once

// Сумма n элементов массива. const — потому что мы только читаем массив.
int sum_array(const int* arr, int n);

// Поменять местами значения по двум адресам.
void swap_ints(int* a, int* b);

// Сколько раз value встречается среди n элементов.
int count_value(const int* arr, int n, int value);

// Указатель на максимальный элемент. Если n == 0 — вернуть nullptr.
const int* max_element_ptr(const int* arr, int n);

// Развернуть массив на месте (in place), без выделения нового массива.
void reverse_in_place(int* arr, int n);
