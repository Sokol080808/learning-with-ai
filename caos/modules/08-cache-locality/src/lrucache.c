// src/lrucache.c
// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 08
// стали зелёными. Сейчас это заглушки: они КОМПИЛИРУЮТСЯ, но ведут себя
// неверно (lru_create отдаёт NULL, lru_get всегда -1, lru_put ничего не
// делает), поэтому тесты падают. Запусти `./caos/run.sh 08`, прочитай вывод
// GoogleTest и реализуй LRU-кэш.
//
// Подход разрешён самый простой: массив записей фиксированной длины +
// «часы» (счётчик), который растёт при каждом обращении. У каждой записи
// храни её время последнего использования. «Наименее недавно использованная»
// запись — та, у которой это время минимально. Эффективность здесь не важна:
// линейный поиск по массиву — нормально для учебной задачи.

#include "lrucache.h"
#include <stdlib.h>   // malloc, free

// Одна запись в кэше.
typedef struct {
    int key;
    int value;
    unsigned long used_at; // время последнего использования
} LruEntry;

// Определение структуры кэша.
struct LruCache {
    size_t capacity;      // сколько записей максимум
    size_t size;          // сколько записей сейчас занято
    LruEntry *arr;        // массив записей, выделенный через malloc
    unsigned long clock;  // глобальные «часы»: увеличивается при каждом доступе
};

// Создать пустой кэш на capacity записей; при нехватке памяти NULL.
LruCache* lru_create(size_t capacity) {
    LruCache *c = (LruCache *)malloc(sizeof(LruCache));
    if (c == NULL) return NULL;

    c->arr = (LruEntry *)malloc(capacity * sizeof(LruEntry));
    if (c->arr == NULL) {
        free(c);
        return NULL;
    }

    c->capacity = capacity;
    c->size = 0;
    c->clock = 0;
    return c;
}

// Вернуть значение по ключу и пометить запись «свежей»; иначе -1.
int lru_get(LruCache* c, int key) {
    for (size_t i = 0; i < c->size; i++) {
        if (c->arr[i].key == key) {
            c->arr[i].used_at = ++c->clock;
            return c->arr[i].value;
        }
    }
    return -1;
}

// Вставить/обновить (key, value); при переполнении вытеснить LRU.
void lru_put(LruCache* c, int key, int value) {
    // 1) Ключ уже есть — обновить value и время использования.
    for (size_t i = 0; i < c->size; i++) {
        if (c->arr[i].key == key) {
            c->arr[i].value = value;
            c->arr[i].used_at = ++c->clock;
            return;
        }
    }

    // 2) Есть свободное место — добавить новую запись.
    if (c->size < c->capacity) {
        c->arr[c->size].key = key;
        c->arr[c->size].value = value;
        c->arr[c->size].used_at = ++c->clock;
        c->size++;
        return;
    }

    // 3) Мест нет — найти LRU запись (с минимальным used_at) и перезаписать.
    size_t victim = 0;
    for (size_t i = 1; i < c->size; i++) {
        if (c->arr[i].used_at < c->arr[victim].used_at) {
            victim = i;
        }
    }
    c->arr[victim].key = key;
    c->arr[victim].value = value;
    c->arr[victim].used_at = ++c->clock;
}

// Освободить кэш и всю его внутреннюю память. lru_free(NULL) безопасен.
void lru_free(LruCache* c) {
    if (c == NULL) return;
    free(c->arr);
    free(c);
}
