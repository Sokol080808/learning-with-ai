// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 04 стали зелёными.
//
// Подсказка по инструментам: чтобы «посмотреть на float как на набор битов», НЕ кастуй
// указатель (float* -> uint32_t*) — это нарушает строгий алиасинг и формально UB.
// Правильный, переносимый способ — побайтово скопировать через memcpy в uint32_t.

#include "floats.h"
#include <string.h>   // memcpy
#include <math.h>     // floor, isnan, isinf, signbit

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

// Контракт: собрать float из трёх полей (операция, обратная к разбору выше).
// Складываем 32-битный образ из полей и копируем его в float через memcpy.
// Маски нужны, чтобы «лишние» биты аргументов не залезли в чужие поля:
//   - от sign берём только младший бит (& 1u), сдвигаем на 31;
//   - от biased_exp берём 8 бит (& 0xFFu), сдвигаем на 23;
//   - от mantissa берём 23 младших бита (& 0x7FFFFFu).
float float_pack(int sign, int biased_exp, uint32_t mantissa) {
    uint32_t bits = ((uint32_t)(sign & 1) << 31)
                  | (((uint32_t)biased_exp & 0xFFu) << 23)
                  | (mantissa & 0x7FFFFFu);
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

// Контракт: округлить x к ближайшему целому, половинки — к чётному соседу.
double round_half_to_even(double x) {
    // Специальные значения пропускаем как есть: NaN, ±Inf, а также ±0
    // (floor сохранил бы знак нуля, но явная проверка делает контракт очевидным).
    if (isnan(x) || isinf(x) || x == 0.0) {
        return x;
    }
    double lo = floor(x);       // ближайшее целое снизу
    double diff = x - lo;       // дробная часть в [0, 1)
    double result;
    if (diff < 0.5) {
        result = lo;            // ближе к нижнему целому
    } else if (diff > 0.5) {
        result = lo + 1.0;      // ближе к верхнему целому
    } else {
        // Ровно половина: выбираем ЧЁТНОГО соседа.
        // fmod(lo, 2.0) == 0 означает, что нижнее целое чётное.
        result = (fmod(lo, 2.0) == 0.0) ? lo : lo + 1.0;
    }
    // Сохраняем знак нуля: round_half_to_even(-0.5) должно дать -0.0, не +0.0.
    if (result == 0.0 && signbit(x)) {
        return -0.0;
    }
    return result;
}
