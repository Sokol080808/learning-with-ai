// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 13
// стали зелёными.
//
// Это потокобезопасный кольцевой буфер (задача производитель-потребитель).
// Тебе понадобятся мьютекс и условные переменные из pthread. Линковка с
// потоками в курсе уже настроена — просто пользуйся заголовками:
//
//   #include <pthread.h>   // уже подтянут через syncbuf.h
//   #include <stdlib.h>    // malloc/free под массив data
//
// Кирпичики (детали — в README модуля и в `man pthread_cond_wait`):
//   pthread_mutex_lock(&b->mutex);   ... pthread_mutex_unlock(&b->mutex);
//   pthread_cond_wait(&cond, &b->mutex);   // отпустить замок и уснуть; проснувшись
//                                          //   — снова взять замок (атомарно)
//   pthread_cond_signal(&cond);            // разбудить ОДНОГО ждущего
//
// ГЛАВНОЕ ПРАВИЛО условных переменных: проверяй условие в ЦИКЛЕ while, а не в
// if. Поток может проснуться «просто так» (ложное пробуждение) или его условие
// мог перехватить другой поток — поэтому после пробуждения условие надо
// перепроверить. Шаблон ожидания:
//
//   while (/* нельзя продолжать */) {
//       pthread_cond_wait(&cond, &b->mutex);
//   }
//
// Заглушки ниже КОМПИЛИРУЮТСЯ, но НИЧЕГО не делают — поэтому тесты красные.
// Важно: они специально НЕ блокируются, чтобы тест не завис на стабе.

#include "syncbuf.h"

#include <stdlib.h>

// Инициализировать буфер ёмкостью capacity. Выдели массив data на capacity
// элементов, обнули head/tail/count, инициализируй мьютекс и обе cond.
void bb_init(BoundedBuffer *b, int capacity) {
    b->data     = malloc(sizeof(int) * (size_t)capacity);
    b->capacity = capacity;
    b->head     = 0;
    b->tail     = 0;
    b->count    = 0;
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->not_full, NULL);
    pthread_cond_init(&b->not_empty, NULL);
}

// Положить item. Под мьютексом: пока count == capacity — ждать not_full;
// записать в tail, сдвинуть tail по кругу, count++, signal not_empty.
void bb_put(BoundedBuffer *b, int item) {
    pthread_mutex_lock(&b->mutex);
    while (b->count == b->capacity) {
        pthread_cond_wait(&b->not_full, &b->mutex);
    }
    b->data[b->tail] = item;
    b->tail = (b->tail + 1) % b->capacity;
    b->count++;
    pthread_cond_signal(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
}

// Забрать элемент (FIFO). Под мьютексом: пока count == 0 — ждать not_empty;
// прочитать из head, сдвинуть head по кругу, count--, signal not_full, вернуть.
int bb_get(BoundedBuffer *b) {
    pthread_mutex_lock(&b->mutex);
    while (b->count == 0) {
        pthread_cond_wait(&b->not_empty, &b->mutex);
    }
    int item = b->data[b->head];
    b->head = (b->head + 1) % b->capacity;
    b->count--;
    pthread_cond_signal(&b->not_full);
    pthread_mutex_unlock(&b->mutex);
    return item;
}

// Освободить ресурсы: free(data), pthread_mutex_destroy, pthread_cond_destroy x2.
void bb_destroy(BoundedBuffer *b) {
    free(b->data);
    b->data = NULL;
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->not_full);
    pthread_cond_destroy(&b->not_empty);
}
