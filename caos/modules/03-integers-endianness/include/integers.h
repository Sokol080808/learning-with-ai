#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 1, если знаковое сложение a+b переполнит int32_t; иначе 0.
   Само сложение НЕ должно вызывать переполнение int32_t — проверяй заранее. */
int add_overflows(int32_t a, int32_t b);

/* Меняет местами два байта 16-битного значения.
   0xAABB -> 0xBBAA. */
uint16_t swap16(uint16_t x);

/* Меняет порядок четырёх байт 32-битного значения на обратный.
   0xAABBCCDD -> 0xDDCCBBAA. */
uint32_t swap32(uint32_t x);

/* Расширяет знак bits-битного значения, лежащего в младших битах x,
   до полноценного int32_t. bits в диапазоне 1..32. */
int32_t sign_extend(uint32_t x, int bits);

/* 1, если машина little-endian (младший байт лежит первым в памяти);
   0, если big-endian. */
int is_little_endian(void);

#ifdef __cplusplus
}
#endif
