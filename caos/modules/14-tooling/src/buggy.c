// ВНИМАНИЕ: здесь пишешь ТЫ. Но этот модуль — ОСОБЫЙ.
//
// Обычно в этом курсе тебе дают заглушки, и ты пишешь логику с нуля. ЗДЕСЬ НЕ ТАК.
// Все четыре функции уже НАПИСАНЫ и выглядят почти правильно — но в каждой спрятан
// БАГ. Код компилируется, запускается и... выдаёт неверный результат (а одна функция
// ещё и лезет за границу массива — это поймает санитайзер/valgrind).
//
// Твоя задача — НАЙТИ и ПОЧИНИТЬ баги, чтобы тесты модуля 14 стали зелёными.
// Готовых ответов в README нет — но есть инструменты (gdb, ASan, valgrind, perf) и
// ступенчатые подсказки. Сначала запусти тесты, посмотри, ЧТО именно не сходится,
// потом залезай под отладчик. Это и есть настоящая работа программиста.

#include "buggy.h"

// Сумма первых n элементов: a[0] + a[1] + ... + a[n-1].
int sum_first_n(const int* a, int n) {
    int sum = 0;
    // Идём по индексам цикла и складываем элементы.
    for (int i = 0; i < n - 1; i++) {   // <-- посмотри на условие очень внимательно
        sum += a[i];
    }
    return sum;
}

// Максимум среди первых n элементов (n >= 1).
int max_in(const int* a, int n) {
    int best = 0;                       // <-- с чего начинать поиск максимума?
    for (int i = 0; i < n; i++) {
        if (a[i] > best) {
            best = a[i];
        }
    }
    return best;
}

// Развернуть первые n элементов массива на месте: {1,2,3,4} -> {4,3,2,1}.
void reverse_array(int* a, int n) {
    // Меняем местами симметричные элементы, сходясь от краёв к центру.
    for (int i = 0; i <= n / 2; i++) {  // <-- сколько обменов нужно и какой парный индекс?
        int j = n - i;                  // <-- индекс «зеркального» элемента
        int tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}

// Сколько раз символ c встречается в строке s.
size_t count_char(const char* s, char c) {
    size_t count = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        if (s[i] == c) {
            count = 1;                  // <-- мы СЧИТАЕМ или просто ставим флажок?
        }
    }
    return count;
}

/* -----------------------------------------------------------------------
 * Задание 5: хеш-таблица счётчиков частот слов.
 *
 * Реализация: открытое хеширование (цепочки).
 * В коде намеренно посажены три бага — найди и почини их с помощью
 * ASan / valgrind / gdb, чтобы тесты стали зелёными.
 * ----------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

#define FREQ_BUCKETS 64

typedef struct FreqNode {
    char*           key;
    size_t          count;
    struct FreqNode* next;
} FreqNode;

struct FreqTable {
    FreqNode* buckets[FREQ_BUCKETS];
    size_t    unique;
};

/* Баг 2 намеренно посажен здесь: накопитель signed int вместо unsigned. */
static unsigned int freq_hash(const char* word) {
    int h = 5381;                       // <-- TODO: найди переполнение
    while (*word) {
        h = h * 33 + (unsigned char)(*word);
        word++;
    }
    return (unsigned int)(h < 0 ? -h : h) % FREQ_BUCKETS;
}

FreqTable* freq_create(void) {
    // TODO: реализовать
    return NULL;
}

void freq_destroy(FreqTable* t) {
    // TODO: реализовать
    (void)t;
}

/* Баг 3 намеренно посажен здесь: сравнение word с самим собой. */
int freq_add(FreqTable* t, const char* word) {
    // TODO: реализовать
    (void)t; (void)word;
    return -1;
}

/* Баг 1 намеренно посажен здесь: use-after-free при сравнении ключей. */
size_t freq_lookup(const FreqTable* t, const char* word) {
    // TODO: реализовать
    (void)t; (void)word;
    return 0;
}

size_t freq_unique(const FreqTable* t) {
    // TODO: реализовать
    (void)t;
    return 0;
}
