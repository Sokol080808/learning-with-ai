#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Разобрать строку из символов '0'/'1' в беззнаковое число.
   Пример: "1011" -> 11. Пустую строку считаем нулём. */
uint32_t parse_binary(const char* s);

/* Записать ровно bits символов ('0' или '1') младших разрядов числа x
   в буфер out, затем нуль-терминатор. Старший бит — слева.
   Буфер out должен вмещать минимум bits + 1 символов.
   Пример: x = 5, bits = 8 -> "00000101". */
void format_binary(uint32_t x, int bits, char* out);

/* Сколько единичных битов в двоичной записи x.
   Пример: 11 (1011) -> 3. */
int count_set_bits(uint32_t x);

/* Разобрать шестнадцатеричную строку в беззнаковое число.
   Допустимы цифры 0-9, буквы a-f и A-F (регистр не важен).
   Пример: "1A" -> 26. Пустую строку считаем нулём. */
uint32_t parse_hex(const char* s);

/* ---- Задание 5: Endianness-сериализатор ---- */

/* Записать v в out[4] в big-endian порядке (out[0] = старший байт). */
void pack_u32_be(uint32_t v, uint8_t out[4]);

/* Прочитать uint32_t из in[4] в big-endian порядке (in[0] = старший байт). */
uint32_t unpack_u32_be(const uint8_t in[4]);

/* Записать v в out[4] в little-endian порядке (out[0] = младший байт). */
void pack_u32_le(uint32_t v, uint8_t out[4]);

/* Прочитать uint32_t из in[4] в little-endian порядке (in[0] = младший байт). */
uint32_t unpack_u32_le(const uint8_t in[4]);

/* Вернуть 1 если хост использует little-endian, 0 если big-endian. */
int host_is_little_endian(void);

#ifdef __cplusplus
}
#endif
