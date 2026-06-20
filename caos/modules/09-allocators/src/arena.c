// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 09 стали зелёными.
//
// Подсказка по структуре файла: каждая функция уже объявлена в include/arena.h.
// Сейчас тела — заглушки: они КОМПИЛИРУЮТСЯ, но дают неверный результат, поэтому
// тесты красные. Твоя работа — заменить заглушки на настоящую логику bump-аллокатора.

#include "arena.h"

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
