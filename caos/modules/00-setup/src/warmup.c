// src/warmup.c
// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 00
// стали зелёными. Сейчас это заглушки: они КОМПИЛИРУЮТСЯ, но дают неверный
// ответ, поэтому тесты падают. Запусти `./caos/run.sh 00`, прочитай вывод
// GoogleTest (что ожидалось / что получилось) и почини обе функции.

#include "warmup.h"
#include <stddef.h>
#include <string.h>

// Контракт: вернуть сумму a и b.
int add(int a, int b) {
    return a + b;
}

// Контракт: вернуть число секунд в `hours` часах.
// Подумай: сколько секунд в одном часе? Умножь на число часов.
long seconds_in(int hours) {
    return (long)hours * 3600L;
}

/* ---- Задание 3: байты, структуры и порядок памяти ---- */

/*
 * packed_record_size — размер структуры через offsetof + sizeof последнего поля.
 *
 * Раскладка (типичный x86-64 / ARM64, ABI System V):
 *   offset 0: a  (uint8_t,  1 байт)
 *   offset 1..3: padding (3 байта), чтобы b был выровнен по 4
 *   offset 4: b  (uint32_t, 4 байта)
 *   offset 8: c  (uint16_t, 2 байта)
 *   offset 10..11: финальный padding до кратного выравниванию (4), итого 12 байт.
 *
 * offsetof(struct PackedRecord, c) + sizeof(uint16_t) = 8 + 2 = 10.
 * Но sizeof(struct PackedRecord) == 12, потому что компилятор добавляет финальный
 * padding. Вот почему мы используем sizeof напрямую для финального результата —
 * и именно это интересно сравнить с «ручным» расчётом.
 *
 * Формула: offsetof последнего поля + sizeof последнего поля даёт размер *без*
 * финального padding. sizeof структуры — с ним. На этой разнице и построено
 * упражнение: наблюдаем padding своими руками.
 */
size_t packed_record_size(void) {
    return offsetof(struct PackedRecord, c) + sizeof(((struct PackedRecord *)0)->c);
}

/*
 * uint32_to_bytes — побайтовое разложение в порядке памяти.
 *
 * Берём адрес value, интерпретируем его как массив из 4 байт и копируем.
 * На little-endian (x86, ARM в стандартном режиме) младший байт лежит
 * по наименьшему адресу: для 0x01020304 out = {0x04, 0x03, 0x02, 0x01}.
 * На big-endian порядок обратный: {0x01, 0x02, 0x03, 0x04}.
 */
void uint32_to_bytes(uint32_t value, unsigned char out[4]) {
    memcpy(out, &value, sizeof(uint32_t));
}

/*
 * is_little_endian — определяет порядок байт через union.
 *
 * union — несколько полей по одному адресу. Записываем 1 в uint32_t,
 * читаем первый байт через unsigned char[]. Если там 1 — младший байт
 * лежит по меньшему адресу → little-endian.
 */
int is_little_endian(void) {
    union {
        uint32_t      i;
        unsigned char b[4];
    } u;
    u.i = 1u;
    return u.b[0] == 1 ? 1 : 0;
}
