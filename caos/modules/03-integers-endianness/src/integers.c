// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 03 стали зелёными.
//
// Подключаем СВОЙ заголовок — там лежат сигнатуры. Если тип в .c и в .h разойдётся,
// компилятор сразу заругается. Это полезно: он ловит расхождения за тебя.
#include "integers.h"
#include <stdint.h>
#include <string.h>

// 1, если знаковое сложение a+b переполнит int32_t; иначе 0.
//
// Подвох: НЕЛЬЗЯ просто посчитать a+b и посмотреть на результат — знаковое
// переполнение в C это неопределённое поведение (UB), компилятор вправе сделать
// что угодно. Переполнение нужно предсказать ЗАРАНЕЕ, не выходя за границы int32_t.
int add_overflows(int32_t a, int32_t b) {
    if (b > 0 && a > INT32_MAX - b) return 1;
    if (b < 0 && a < INT32_MIN - b) return 1;
    return 0;
}

// Меняет местами два байта 16-битного значения: 0xAABB -> 0xBBAA.
uint16_t swap16(uint16_t x) {
    return (uint16_t)(((x & 0xFFu) << 8) | ((x >> 8) & 0xFFu));
}

// Меняет порядок четырёх байт на обратный: 0xAABBCCDD -> 0xDDCCBBAA.
uint32_t swap32(uint32_t x) {
    return ((x & 0xFFu)       << 24) |
           ((x & 0xFF00u)     <<  8) |
           ((x & 0xFF0000u)   >>  8) |
           ((x & 0xFF000000u) >> 24);
}

// Расширяет знак bits-битного значения (в младших битах x) до int32_t.
// bits в диапазоне 1..32.
int32_t sign_extend(uint32_t x, int bits) {
    // Маска: оставляем только младшие bits бит.
    uint32_t mask = (bits == 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u);
    x = x & mask;
    // Знаковый бит — бит с номером (bits - 1).
    uint32_t m = 1u << (bits - 1);
    // Формула XOR + вычитание даёт расширение знака без UB.
    return (int32_t)((x ^ m) - m);
}

// 1, если машина little-endian; 0, если big-endian.
int is_little_endian(void) {
    uint32_t probe = 1u;
    unsigned char first_byte;
    memcpy(&first_byte, &probe, 1);
    return (first_byte == 1) ? 1 : 0;
}

// Кодирует *pkt в big-endian байты buf[0..11] (12 байт: 2+2+4+4).
// Возвращает 12 при успехе; -1, если buf == NULL или buf_size < 12.
int packet_encode(const PacketHeader *pkt, uint8_t *buf, size_t buf_size) {
    if (buf == NULL || buf_size < 12) return -1;
    /* version (2 bytes, big-endian): bytes 0-1 */
    buf[0] = (uint8_t)((pkt->version >> 8) & 0xFFu);
    buf[1] = (uint8_t)(pkt->version        & 0xFFu);
    /* port (2 bytes, big-endian): bytes 2-3 */
    buf[2] = (uint8_t)((pkt->port >> 8) & 0xFFu);
    buf[3] = (uint8_t)(pkt->port        & 0xFFu);
    /* length (4 bytes, big-endian): bytes 4-7 */
    buf[4] = (uint8_t)((pkt->length >> 24) & 0xFFu);
    buf[5] = (uint8_t)((pkt->length >> 16) & 0xFFu);
    buf[6] = (uint8_t)((pkt->length >>  8) & 0xFFu);
    buf[7] = (uint8_t)(pkt->length         & 0xFFu);
    /* checksum (4 bytes, big-endian): bytes 8-11 */
    buf[8]  = (uint8_t)((pkt->checksum >> 24) & 0xFFu);
    buf[9]  = (uint8_t)((pkt->checksum >> 16) & 0xFFu);
    buf[10] = (uint8_t)((pkt->checksum >>  8) & 0xFFu);
    buf[11] = (uint8_t)(pkt->checksum         & 0xFFu);
    return 12;
}

// Декодирует big-endian байты из buf в *out.
// Возвращает 12 при успехе; -1, если buf == NULL, out == NULL или buf_size < 12.
int packet_decode(const uint8_t *buf, size_t buf_size, PacketHeader *out) {
    if (buf == NULL || out == NULL || buf_size < 12) return -1;
    out->version  = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
    out->port     = ((uint16_t)buf[2] << 8) | (uint16_t)buf[3];
    out->length   = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16)
                  | ((uint32_t)buf[6] <<  8) |  (uint32_t)buf[7];
    out->checksum = ((uint32_t)buf[8] << 24) | ((uint32_t)buf[9] << 16)
                  | ((uint32_t)buf[10] <<  8) | (uint32_t)buf[11];
    return 12;
}
