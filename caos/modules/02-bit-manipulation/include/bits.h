#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Вернуть значение i-го бита числа x (0 или 1). Биты нумеруются с 0 справа. */
int get_bit(uint32_t x, int i);

/* Вернуть x, в котором i-й бит установлен в 1 (остальные биты не трогаем). */
uint32_t set_bit(uint32_t x, int i);

/* Вернуть x, в котором i-й бит сброшен в 0 (остальные биты не трогаем). */
uint32_t clear_bit(uint32_t x, int i);

/* Вернуть x, в котором i-й бит инвертирован (0->1, 1->0; остальные не трогаем). */
uint32_t toggle_bit(uint32_t x, int i);

/* Вернуть 1, если x — степень двойки и не ноль (ровно один бит установлен), иначе 0. */
int is_power_of_two(uint32_t x);

/* Вернуть x с переставленным порядком 4 байт (byte 0 <-> byte 3, byte 1 <-> byte 2). */
uint32_t reverse_bytes(uint32_t x);

#ifdef __cplusplus
}
#endif
