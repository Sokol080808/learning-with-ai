// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 05 стали зелёными.
//
// Подсказка по сборке: это чистый C (C11). Никаких new/cout — только то, что нужно
// для арифметики над числами и указателями. Заглушки ниже КОМПИЛИРУЮТСЯ, но дают
// неверный результат, поэтому тесты сейчас красные. Твоя задача — заменить тела.

#include "memory.h"

// Округлить n вверх до кратного align (align — степень двойки).
// Контракт: align_up(13, 8) == 16; align_up(16, 8) == 16; align_up(0, 8) == 0.
size_t align_up(size_t n, size_t align) {
    (void)n;
    (void)align;
    return 0;  // TODO: верни правильно округлённое значение
}

// Вернуть 1, если адрес p кратен align, иначе 0 (align — степень двойки).
int is_aligned(const void* p, size_t align) {
    (void)p;
    (void)align;
    return 0;  // TODO: проверь, кратен ли адрес align
}

// Указатель на элемент base[n] (то есть base + n).
int* nth_element(int* base, size_t n) {
    (void)base;
    (void)n;
    return NULL;  // TODO: верни указатель на нужный элемент
}

// Расстояние a - b в ЭЛЕМЕНТАХ (int), не в байтах.
long ptr_distance(const int* a, const int* b) {
    (void)a;
    (void)b;
    return 0;  // TODO: верни разницу указателей в элементах
}
