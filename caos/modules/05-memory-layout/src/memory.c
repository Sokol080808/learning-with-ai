// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 05 стали зелёными.
//
// Подсказка по сборке: это чистый C (C11). Никаких new/cout — только то, что нужно
// для арифметики над числами и указателями. Заглушки ниже КОМПИЛИРУЮТСЯ, но дают
// неверный результат, поэтому тесты сейчас красные. Твоя задача — заменить тела.

#include "memory.h"
#include <stdint.h>  // uintptr_t

// Округлить n вверх до кратного align (align — степень двойки).
// Контракт: align_up(13, 8) == 16; align_up(16, 8) == 16; align_up(0, 8) == 0.
size_t align_up(size_t n, size_t align) {
    // Классический bit-trick для степени двойки:
    // прибавляем (align-1), затем обнуляем младшие биты маской ~(align-1).
    return (n + (align - 1)) & ~(align - 1);
}

// Вернуть 1, если адрес p кратен align, иначе 0 (align — степень двойки).
int is_aligned(const void* p, size_t align) {
    // Преобразуем указатель в целое число, проверяем младшие биты.
    uintptr_t addr = (uintptr_t)p;
    return ((addr & (align - 1)) == 0) ? 1 : 0;
}

// Указатель на элемент base[n] (то есть base + n).
int* nth_element(int* base, size_t n) {
    // Арифметика указателей: шаг — в элементах, не в байтах.
    return base + n;
}

// Расстояние a - b в ЭЛЕМЕНТАХ (int), не в байтах.
long ptr_distance(const int* a, const int* b) {
    // Вычитание указателей одного типа даёт ptrdiff_t в элементах.
    return (long)(a - b);
}

// Вычислить раскладку «виртуальной» структуры по массиву дескрипторов полей.
//
// Алгоритм:
//   1. Обходим поля слева направо, ведя «курсор» — сколько байт занято.
//   2. Перед укладкой поля i выравниваем курсор вверх до fields[i].align.
//   3. Записываем выровненный курсор как смещение, затем продвигаем на size.
//   4. После обхода всех полей выравниваем итоговый курсор до максимального
//      align среди полей — это хвостовой padding, итог и есть sizeof.
size_t layout_compute(const Field* fields, int n, size_t* offsets_out) {
    size_t cursor   = 0;
    size_t max_align = 1;

    for (int i = 0; i < n; i++) {
        size_t a = fields[i].align;
        if (a > max_align) max_align = a;

        // Выравниваем курсор до требования текущего поля.
        cursor = align_up(cursor, a);
        offsets_out[i] = cursor;

        // Продвигаем курсор на размер поля.
        cursor += fields[i].size;
    }

    // Хвостовой padding: sizeof должен быть кратен строжайшему align.
    return align_up(cursor, max_align);
}
