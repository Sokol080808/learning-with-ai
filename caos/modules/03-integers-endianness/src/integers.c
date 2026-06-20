// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 03 стали зелёными.
//
// Подключаем СВОЙ заголовок — там лежат сигнатуры. Если тип в .c и в .h разойдётся,
// компилятор сразу заругается. Это полезно: он ловит расхождения за тебя.
#include "integers.h"
#include <stdint.h>
#include <string.h>

// 1, если знаковое сложение a+b переполнит int32_t; иначе 0.
//
// Подвох: НЕЛЬЗЯ просто посчитать a+b и посмотреть на результат — знаковое
// переполнение в C это неопределённое поведение (UB), компилятор вправе сделать
// что угодно. Переполнение нужно предсказать ЗАРАНЕЕ, не выходя за границы int32_t.
int add_overflows(int32_t a, int32_t b) {
    if (b > 0 && a > INT32_MAX - b) return 1;
    if (b < 0 && a < INT32_MIN - b) return 1;
    return 0;
}

// Меняет местами два байта 16-битного значения: 0xAABB -> 0xBBAA.
uint16_t swap16(uint16_t x) {
    return (uint16_t)(((x & 0xFFu) << 8) | ((x >> 8) & 0xFFu));
}

// Меняет порядок четырёх байт на обратный: 0xAABBCCDD -> 0xDDCCBBAA.
uint32_t swap32(uint32_t x) {
    return ((x & 0xFFu)       << 24) |
           ((x & 0xFF00u)     <<  8) |
           ((x & 0xFF0000u)   >>  8) |
           ((x & 0xFF000000u) >> 24);
}

// Расширяет знак bits-битного значения (в младших битах x) до int32_t.
// bits в диапазоне 1..32.
int32_t sign_extend(uint32_t x, int bits) {
    // Маска: оставляем только младшие bits бит.
    uint32_t mask = (bits == 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
    x = x & mask;
    // Знаковый бит — бит с номером (bits - 1).
    uint32_t m = 1u << (bits - 1);
    // Формула XOR + вычитание даёт расширение знака без UB.
    return (int32_t)((x ^ m) - m);
}

// 1, если машина little-endian; 0, если big-endian.
int is_little_endian(void) {
    uint32_t probe = 1u;
    unsigned char first_byte;
    memcpy(&first_byte, &probe, 1);
    return (first_byte == 1) ? 1 : 0;
}
