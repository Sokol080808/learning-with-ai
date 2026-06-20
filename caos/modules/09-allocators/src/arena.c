// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 09 стали зелёными.
//
// Подсказка по структуре файла: каждая функция уже объявлена в include/arena.h.
// Сейчас тела — заглушки: они КОМПИЛИРУЮТСЯ, но дают неверный результат, поэтому
// тесты красные. Твоя работа — заменить заглушки на настоящую логику bump-аллокатора.

#include "arena.h"
#include <string.h>   // memcpy (копирование заголовка/футера блока в heap_*)

// Выравнивание блоков: каждый выданный адрес кратен 8 (покрывает double и
// указатели на 64-битной машине).
#define ARENA_ALIGN ((size_t)8)

// Округлить смещение вверх до кратного ARENA_ALIGN.
// Для степени двойки: (x + (A-1)) & ~(A-1).
static size_t align_up(size_t x) {
    return (x + (ARENA_ALIGN - 1)) & ~(ARENA_ALIGN - 1);
}

// Привязать арену к буферу [buffer, buffer+size). used обнуляется.
void arena_init(Arena *a, void *buffer, size_t size) {
    a->buf = (unsigned char *)buffer;
    a->size = size;
    a->used = 0;
}

// Выдать блок размером n байт, выровненный по 8, или NULL, если не помещается.
void *arena_alloc(Arena *a, size_t n) {
    // Начало блока — текущая «голова», округлённая вверх до кратного 8.
    size_t start = align_up(a->used);

    // Само выравнивание уже могло вылезти за конец буфера — тогда места нет.
    if (start > a->size) {
        return NULL;
    }

    // Проверка «помещается» без сложения start + n, чтобы не переполнить size_t
    // на гигантских n: эквивалентно start + n <= size при start <= size.
    if (n > a->size - start) {
        return NULL;
    }

    // Сдвигаем голову на конец выданного блока и возвращаем его начало.
    a->used = start + n;
    return a->buf + start;
}

// Сколько байт ещё можно выдать.
size_t arena_remaining(const Arena *a) {
    return a->size - a->used;
}

// Отдать всё разом: следующая выдача снова начнётся с начала буфера.
void arena_reset(Arena *a) {
    a->used = 0;
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
    /* Один большой свободный блок на весь буфер. Размер буфера (65536) уже
     * кратен 8, так что один блок занимает его целиком: header + payload +
     * footer = HEAP_CAP. */
    set_block(heap_lo(), HEAP_CAP, /*used=*/0);
    g_heap_ready = 1;
}

void *heap_malloc(size_t n) {
    if (!g_heap_ready) {
        heap_init();
    }

    /* Сколько байт payload реально нужно: округляем запрос вверх до кратного 8
     * (payload всегда выровнен), но не меньше WSIZE, чтобы освобождённый блок
     * мог хранить служебные данные и не схлопывался в ноль. */
    size_t need_payload = (n + (WSIZE - 1)) & ~(WSIZE - 1);
    if (need_payload < WSIZE) {
        need_payload = WSIZE;
    }
    size_t need_block = need_payload + 2 * WSIZE;   /* + header + footer */
    if (need_block < MIN_BLOCK) {
        need_block = MIN_BLOCK;
    }

    /* first-fit: линейно идём по неявному списку и берём ПЕРВЫЙ свободный блок,
     * в который влезает need_block. */
    for (unsigned char *blk = heap_lo(); blk < heap_hi(); blk += block_size_of(blk)) {
        size_t bs = block_size_of(blk);
        if (!block_used(blk) && bs >= need_block) {
            size_t remainder = bs - need_block;
            if (remainder >= MIN_BLOCK) {
                /* Расщепляем: занятый блок нужного размера + хвост-остаток. */
                set_block(blk, need_block, /*used=*/1);
                set_block(blk + need_block, remainder, /*used=*/0);
            } else {
                /* Остаток слишком мал под отдельный блок — отдаём весь блок
                 * целиком (внутренняя фрагментация). */
                set_block(blk, bs, /*used=*/1);
            }
            return blk + WSIZE;     /* payload */
        }
    }
    return NULL;                    /* подходящего свободного блока нет */
}

void heap_free(void *p) {
    if (p == NULL) {
        return;
    }
    unsigned char *blk = (unsigned char *)p - WSIZE;   /* назад к заголовку */
    size_t bs = block_size_of(blk);

    /* Помечаем свободным. */
    set_block(blk, bs, /*used=*/0);

    /* Coalescing вперёд: если СЛЕДУЮЩИЙ блок существует и свободен — сливаем. */
    unsigned char *next = blk + bs;
    if (next < heap_hi() && !block_used(next)) {
        bs += block_size_of(next);
        set_block(blk, bs, /*used=*/0);
    }

    /* Coalescing назад: по футеру предыдущего блока узнаём его размер и, если
     * он свободен, переносим начало объединённого блока на него. */
    if (blk > heap_lo()) {
        size_t prev_h;
        memcpy(&prev_h, blk - WSIZE, WSIZE);    /* футер левого соседа */
        if ((prev_h & 1u) == 0u) {              /* сосед свободен */
            size_t prev_size = prev_h & ~(size_t)7;
            unsigned char *prev = blk - prev_size;
            bs += prev_size;
            set_block(prev, bs, /*used=*/0);
        }
    }
}
