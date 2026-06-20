// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 01 стали зелёными.
//
// Файл на чистом C (C11). Заглушки ниже КОМПИЛИРУЮТСЯ, но дают неверный результат —
// поэтому тесты пока красные. Замени каждое тело на настоящую логику.
// Подсказки и объяснения — в README.md этого модуля.

#include "numrep.h"

// Контракт: строка из символов '0'/'1' -> беззнаковое число.
// Пример: "1011" -> 11. Пустая строка -> 0.
uint32_t parse_binary(const char* s) {
    (void)s; // TODO: убери, когда начнёшь использовать s
    return 0; // TODO: реализуй разбор двоичной строки
}

// Контракт: записать bits символов ('0'/'1') младших разрядов x, потом '\0'.
// Старший бит — слева. Пример: x=5, bits=8 -> "00000101".
void format_binary(uint32_t x, int bits, char* out) {
    (void)x;    // TODO
    (void)bits; // TODO
    (void)out;  // TODO: реализуй запись битов в out
}

// Контракт: число единичных битов в x. Пример: 11 (1011) -> 3.
int count_set_bits(uint32_t x) {
    (void)x; // TODO
    return 0; // TODO: посчитай единичные биты
}

// Контракт: шестнадцатеричная строка -> беззнаковое число (регистр не важен).
// Пример: "1A" -> 26. Пустая строка -> 0.
uint32_t parse_hex(const char* s) {
    (void)s; // TODO
    return 0; // TODO: реализуй разбор hex-строки
}

/* ---- Задание 5: Endianness-сериализатор ---- */

// Записать v в out[4] в big-endian порядке (out[0] = старший байт).
void pack_u32_be(uint32_t v, uint8_t out[4]) {
    (void)v;   // TODO
    (void)out; // TODO: реализуй запись в big-endian порядке
}

// Прочитать uint32_t из in[4] в big-endian порядке (in[0] = старший байт).
uint32_t unpack_u32_be(const uint8_t in[4]) {
    (void)in; // TODO
    return 0; // TODO: реализуй чтение в big-endian порядке
}

// Записать v в out[4] в little-endian порядке (out[0] = младший байт).
void pack_u32_le(uint32_t v, uint8_t out[4]) {
    (void)v;   // TODO
    (void)out; // TODO: реализуй запись в little-endian порядке
}

// Прочитать uint32_t из in[4] в little-endian порядке (in[0] = младший байт).
uint32_t unpack_u32_le(const uint8_t in[4]) {
    (void)in; // TODO
    return 0; // TODO: реализуй чтение в little-endian порядке
}

// Вернуть 1 если хост использует little-endian, 0 если big-endian.
int host_is_little_endian(void) {
    return 0; // TODO: определи порядок байт хоста
}
