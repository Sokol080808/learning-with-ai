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

#ifdef __cplusplus
}
#endif
