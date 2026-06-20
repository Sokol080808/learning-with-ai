// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 01 стали зелёными.
//
// Файл на чистом C (C11). Заглушки ниже КОМПИЛИРУЮТСЯ, но дают неверный результат —
// поэтому тесты пока красные. Замени каждое тело на настоящую логику.
// Подсказки и объяснения — в README.md этого модуля.

#include "numrep.h"

// Контракт: строка из символов '0'/'1' -> беззнаковое число.
// Пример: "1011" -> 11. Пустая строка -> 0.
uint32_t parse_binary(const char* s) {
    uint32_t acc = 0;
    for (; *s != '\0'; s++) {
        acc = (acc << 1) | (uint32_t)(*s - '0');
    }
    return acc;
}

// Контракт: записать bits символов ('0'/'1') младших разрядов x, потом '\0'.
// Старший бит — слева. Пример: x=5, bits=8 -> "00000101".
void format_binary(uint32_t x, int bits, char* out) {
    for (int i = bits - 1; i >= 0; i--) {
        *out++ = (char)('0' + ((x >> i) & 1u));
    }
    *out = '\0';
}

// Контракт: число единичных битов в x. Пример: 11 (1011) -> 3.
int count_set_bits(uint32_t x) {
    int count = 0;
    while (x) {
        count += (int)(x & 1u);
        x >>= 1;
    }
    return count;
}

// Контракт: шестнадцатеричная строка -> беззнаковое число (регистр не важен).
// Пример: "1A" -> 26. Пустая строка -> 0.
uint32_t parse_hex(const char* s) {
    uint32_t acc = 0;
    for (; *s != '\0'; s++) {
        char c = *s;
        uint32_t digit;
        if (c >= '0' && c <= '9') {
            digit = (uint32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = (uint32_t)(c - 'a') + 10u;
        } else {
            digit = (uint32_t)(c - 'A') + 10u;
        }
        acc = (acc << 4) | digit;
    }
    return acc;
}

/* ---- Задание 5: Endianness-сериализатор ---- */

// Записать v в out[4] в big-endian порядке (out[0] = старший байт).
void pack_u32_be(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t)(v >> 24);
    out[1] = (uint8_t)(v >> 16);
    out[2] = (uint8_t)(v >>  8);
    out[3] = (uint8_t)(v      );
}

// Прочитать uint32_t из in[4] в big-endian порядке (in[0] = старший байт).
uint32_t unpack_u32_be(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24)
         | ((uint32_t)in[1] << 16)
         | ((uint32_t)in[2] <<  8)
         |  (uint32_t)in[3];
}

// Записать v в out[4] в little-endian порядке (out[0] = младший байт).
void pack_u32_le(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t)(v      );
    out[1] = (uint8_t)(v >>  8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}

// Прочитать uint32_t из in[4] в little-endian порядке (in[0] = младший байт).
uint32_t unpack_u32_le(const uint8_t in[4]) {
    return  (uint32_t)in[0]
         | ((uint32_t)in[1] <<  8)
         | ((uint32_t)in[2] << 16)
         | ((uint32_t)in[3] << 24);
}

// Вернуть 1 если хост использует little-endian, 0 если big-endian.
// Используем union — законно в C11 (type-punning через union определено).
int host_is_little_endian(void) {
    union { uint32_t u32; uint8_t b[4]; } probe;
    probe.u32 = 1u;
    return (int)(probe.b[0] == 1u);
}
