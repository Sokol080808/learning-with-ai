#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Сырые 32 бита числа float, как они лежат в памяти (через memcpy). */
uint32_t float_bits(float f);

/* Знаковый бит: 0 для неотрицательных, 1 для отрицательных. */
int float_sign(float f);

/* Поле экспоненты как целое 0..255 (8 бит «как есть», без вычитания смещения). */
int float_raw_exponent(float f);

/* 1, если f — это NaN (Not a Number), иначе 0. */
int my_isnan(float f);

/* Собрать float из трёх полей IEEE 754 (одинарная точность) — операция,
 * обратная к разбору выше.
 *   sign       — знаковый бит: используется только младший бит (0 или 1);
 *   biased_exp — поле экспоненты 0..255 («сырое», со смещением 127);
 *   mantissa   — 23 бита мантиссы (старшие биты сверх 23 игнорируются).
 * Биты складываются как (sign<<31)|(biased_exp<<23)|(mantissa & 0x7FFFFF)
 * и копируются в float через memcpy. Гарантируется обратимость:
 * float_pack(float_sign(f), float_raw_exponent(f), float_bits(f)) воссоздаёт f. */
float float_pack(int sign, int biased_exp, uint32_t mantissa);

/* Округлить x до ближайшего целого по правилу «ties-to-even» (round half to
 * even, «банковское округление») — режим округления IEEE 754 по умолчанию.
 * Половинки (x.5) уходят к ЧЁТНОМУ соседу: 0.5->0, 1.5->2, 2.5->2, 3.5->4,
 * -0.5->-0, -2.5->-2. Не-половинки округляются к ближайшему как обычно.
 * Специальные значения проходят насквозь: ±0 -> ±0, ±Inf -> ±Inf, NaN -> NaN. */
double round_half_to_even(double x);

#ifdef __cplusplus
}
#endif
