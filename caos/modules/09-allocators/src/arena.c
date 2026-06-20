// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 09 стали зелёными.
//
// Подсказка по структуре файла: каждая функция уже объявлена в include/arena.h.
// Сейчас тела — заглушки: они КОМПИЛИРУЮТСЯ, но дают неверный результат, поэтому
// тесты красные. Твоя работа — заменить заглушки на настоящую логику bump-аллокатора.

#include "arena.h"
#include <string.h>   // memcpy (копирование заголовка/футера блока в heap_*)

// Привязать арену к буферу [buffer, buffer+size). used обнуляется.
void arena_init(Arena *a, void *buffer, size_t size) {
    // TODO: запомни в полях a, где буфер, какого он размера, и что выдано 0 байт.
    (void)a;
    (void)buffer;
    (void)size;
}

// Выдать блок размером n байт, выровненный по 8, или NULL, если не помещается.
void *arena_alloc(Arena *a, size_t n) {
    // TODO: округли текущую «голову» вверх до кратного 8, проверь, что блок n байт
    //       умещается до конца буфера, сдвинь голову и верни адрес начала блока.
    (void)a;
    (void)n;
    return NULL;  // заглушка: всегда «нет памяти»
}

// Сколько байт ещё можно выдать.
size_t arena_remaining(const Arena *a) {
    // TODO: это size - used.
    (void)a;
    return 0;  // заглушка: будто свободного места нет
}

// Отдать всё разом: следующая выдача снова начнётся с начала буфера.
void arena_reset(Arena *a) {
    // TODO: обнули счётчик выданных байт.
    (void)a;
}

/* ───────────────────────────────────────────────────────────────────────────
 * heap_*: минимальный аллокатор на неявном списке (implicit free list).
 *
 * Раскладка одного блока (всё кратно 8 байтам):
 *
 *     +----------+--------------------------+----------+
 *     |  HEADER  |   payload (>= n байт)    |  FOOTER  |
 *     +----------+--------------------------+----------+
 *     ^block      ^указатель, который видит пользователь
 *
 * HEADER и FOOTER — по одному слову (WSIZE = 8 байт). В каждом лежит одно и то
 * же значение: размер ВСЕГО блока (с заголовком и футером) ИЛИ-нён с битом
 * занятости (младший бит: 1 = занят, 0 = свободен). Размер блока всегда кратен
 * 8, поэтому три младших бита заголовка свободны под флаги — мы используем
 * только бит 0.
 *
 * Футер (boundary tag, граничный тег из CS:APP §9.9.11) — это копия заголовка
 * в конце блока. Он нужен, чтобы при освобождении найти ПРЕДЫДУЩИЙ физический
 * блок за O(1): шагнуть на слово назад и прочитать его размер. Без футера
 * слить блок с левым соседом в неявном списке нельзя — заголовки указывают
 * только вперёд.
 * ───────────────────────────────────────────────────────────────────────── */

#define HEAP_CAP   ((size_t)64 * 1024)   /* 64 KiB — весь буфер кучи */
#define WSIZE      ((size_t)8)            /* слово = заголовок = футер = 8 байт */
#define MIN_BLOCK  ((size_t)32)           /* HEADER + минимум полезной + FOOTER */

/* Буфер кучи. alignas через объединение с указателем гарантирует, что начало
 * выровнено по 8, поэтому payload (= блок + WSIZE) тоже будет кратен 8. */
static union {
    unsigned char bytes[HEAP_CAP];
    void *align_;                  /* форсирует выравнивание буфера по 8 */
} g_heap;

static int g_heap_ready = 0;       /* была ли куча размечена heap_init */

/* ── работа с заголовком/футером блока ─────────────────────────────────────
 * Блок мы адресуем по его НАЧАЛУ (адрес заголовка). Пользователю отдаём
 * payload = блок + WSIZE. */

static size_t pack(size_t block_size, int used) {
    return block_size | (size_t)(used ? 1 : 0);
}
static size_t block_size_of(const unsigned char *block) {
    size_t h;
    memcpy(&h, block, WSIZE);
    return h & ~(size_t)7;          /* стираем 3 младших бита-флага */
}
static int block_used(const unsigned char *block) {
    size_t h;
    memcpy(&h, block, WSIZE);
    return (int)(h & 1u);
}
/* Записать заголовок И футер блока одинаковым значением. */
static void set_block(unsigned char *block, size_t block_size, int used) {
    size_t h = pack(block_size, used);
    memcpy(block, &h, WSIZE);                       /* заголовок */
    memcpy(block + block_size - WSIZE, &h, WSIZE);  /* футер в конце блока */
}

/* Границы кучи как байтовые указатели. */
static unsigned char *heap_lo(void) { return g_heap.bytes; }
static unsigned char *heap_hi(void) { return g_heap.bytes + HEAP_CAP; }

void heap_init(void) {
    // TODO: разметь весь g_heap.bytes как один большой свободный блок размером
    //       HEAP_CAP и взведи g_heap_ready = 1.
    (void)set_block;
    (void)block_used;
    (void)block_size_of;
    (void)pack;
    (void)heap_lo;
    (void)heap_hi;
    (void)g_heap_ready;
}

void *heap_malloc(size_t n) {
    // TODO: если !g_heap_ready — вызови heap_init; округли n до кратного WSIZE,
    //       добавь 2*WSIZE на header+footer (не менее MIN_BLOCK), найди первый
    //       подходящий свободный блок first-fit, расщепи при необходимости,
    //       пометь занятым и верни payload (blk + WSIZE). NULL если не нашёл.
    (void)n;
    return NULL;  // заглушка: всегда «нет памяти»
}

void heap_free(void *p) {
    // TODO: если p == NULL — ничего не делай; иначе найди заголовок блока
    //       (p - WSIZE), пометь свободным, слей с правым соседом, затем с левым
    //       (используй footer левого соседа для coalescing назад).
    (void)p;
}
