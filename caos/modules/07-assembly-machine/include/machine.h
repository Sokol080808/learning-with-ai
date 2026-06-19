#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* asm_a: вычисляет x*x + 1. */
int asm_a(int x);

/* asm_b: возвращает максимум из a и b. */
int asm_b(int a, int b);

/* asm_c: сумма целых от 1 до n (для n <= 0 сумма равна 0). */
int asm_c(int n);

#ifdef __cplusplus
}
#endif
