// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 04 стали зелёными.
//
// Подсказка по инструментам: чтобы «посмотреть на float как на набор битов», НЕ кастуй
// указатель (float* -> uint32_t*) — это нарушает строгий алиасинг и формально UB.
// Правильный, переносимый способ — побайтово скопировать через memcpy в uint32_t.

#include "floats.h"
#include <string.h>   // memcpy

// Контракт: вернуть сырые 32 бита числа f, как они лежат в памяти,
// упакованные в uint32_t (бит 31 — знак, биты 30..23 — экспонента, биты 22..0 — мантисса).
uint32_t float_bits(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return bits;
}

// Контракт: вернуть знаковый бит числа f — 0 для неотрицательных, 1 для отрицательных.
int float_sign(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return (int)(bits >> 31);
}

// Контракт: вернуть 8 бит поля экспоненты как целое число 0..255
// (именно «сырое» поле, без вычитания смещения 127).
int float_raw_exponent(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return (int)((bits >> 23) & 0xFFu);
}

// Контракт: вернуть 1, если f — это NaN, иначе 0.
int my_isnan(float f) {
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));
    uint32_t exponent = (bits >> 23) & 0xFFu;
    uint32_t mantissa = bits & 0x7FFFFFu;
    return (exponent == 255u && mantissa != 0u) ? 1 : 0;
}
