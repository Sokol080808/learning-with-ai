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
    for (int i = 0; i < n; i++) {
        sum += a[i];
    }
    return sum;
}

// Максимум среди первых n элементов (n >= 1).
int max_in(const int* a, int n) {
    int best = a[0];
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
    for (int i = 0; i < n / 2; i++) {
        int j = n - 1 - i;
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
            count++;
        }
    }
    return count;
}

/* -----------------------------------------------------------------------
 * Задание 5: хеш-таблица счётчиков частот слов.
 *
 * Реализация: открытое хеширование (цепочки).
 * REFERENCE-BRANCH: все три бага исправлены.
 *
 * Три бага, которые были посажены (и исправлены здесь):
 *   Баг 1 (use-after-free): в freq_lookup ключ читался ПОСЛЕ free(node->key)
 *          в ветке, которая никогда не срабатывает в этой реализации, но
 *          оригинальный код освобождал key до сравнения.
 *   Баг 2 (signed overflow): freq_hash накапливал хеш в signed int;
 *          исправлено на unsigned int (переполнение unsigned — defined behavior).
 *   Баг 3 (логическая ошибка): freq_add сравнивал word с самим собой
 *          вместо node->key, поэтому всегда считал «слово уже есть»
 *          и никогда не увеличивал существующий счётчик.
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

/* ---- Баг 2 исправлен: тип накопителя unsigned int, не int ------------- */
static unsigned int freq_hash(const char* word) {
    unsigned int h = 5381u;
    while (*word) {
        /* djb2: h = h * 33 + c  (переполнение unsigned — wrap-around, OK) */
        h = h * 33u + (unsigned char)(*word);
        word++;
    }
    return h % FREQ_BUCKETS;
}

FreqTable* freq_create(void) {
    FreqTable* t = (FreqTable*)calloc(1, sizeof(FreqTable));
    return t;  /* NULL при ошибке аллокации */
}

void freq_destroy(FreqTable* t) {
    if (!t) return;
    for (int b = 0; b < FREQ_BUCKETS; b++) {
        FreqNode* node = t->buckets[b];
        while (node) {
            FreqNode* next = node->next;
            free(node->key);
            free(node);
            node = next;
        }
    }
    free(t);
}

/* ---- Баг 3 исправлен: сравниваем word с node->key, а не word с word -- */
int freq_add(FreqTable* t, const char* word) {
    unsigned int b = freq_hash(word);
    FreqNode* node = t->buckets[b];
    while (node) {
        if (strcmp(node->key, word) == 0) {   /* было: strcmp(word, word) */
            node->count++;
            return 0;
        }
        node = node->next;
    }
    /* Новое слово */
    FreqNode* newnode = (FreqNode*)malloc(sizeof(FreqNode));
    if (!newnode) return -1;
    newnode->key = (char*)malloc(strlen(word) + 1);
    if (!newnode->key) { free(newnode); return -1; }
    strcpy(newnode->key, word);
    newnode->count = 1;
    newnode->next  = t->buckets[b];
    t->buckets[b]  = newnode;
    t->unique++;
    return 0;
}

/* ---- Баг 1 исправлен: читаем node->key ДО любого free --------------- */
size_t freq_lookup(const FreqTable* t, const char* word) {
    unsigned int b = freq_hash(word);
    const FreqNode* node = t->buckets[b];
    while (node) {
        /* Сравниваем сначала, и только потом (никогда) не освобождаем */
        if (strcmp(node->key, word) == 0) {   /* было: free(node->key) здесь */
            return node->count;
        }
        node = node->next;
    }
    return 0;
}

size_t freq_unique(const FreqTable* t) {
    return t ? t->unique : 0;
}
