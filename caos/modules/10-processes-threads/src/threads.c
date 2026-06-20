// ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 10
// стали зелёными.
//
// Тебе понадобится pthread. Линковка с потоками уже настроена в CMake —
// просто подключи заголовок и пользуйся:
//
//   #include <pthread.h>
//
// Полезные кирпичики (загляни в man или в README модуля):
//   pthread_t                          — «ручка» потока
//   pthread_create(&t, NULL, fn, arg)  — запустить поток на функции fn(arg)
//   pthread_join(t, NULL)              — дождаться, пока поток t завершится
//   pthread_mutex_t m;                 — мьютекс (замок)
//   pthread_mutex_init / _lock / _unlock / _destroy
//
// Функция потока ВСЕГДА имеет вид:  void *worker(void *arg) { ... return NULL; }
// Параметр arg — это void*, через него передают данные (обычно — указатель
// на структуру с аргументами и местом под результат).

#include "threads.h"
#include <pthread.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * parallel_sum helpers
 * ---------------------------------------------------------------------- */

typedef struct {
    const long *arr;
    size_t begin;  /* inclusive */
    size_t end;    /* exclusive */
    long result;
} SumPart;

static void *sum_worker(void *arg) {
    SumPart *p = (SumPart *)arg;
    long s = 0;
    for (size_t i = p->begin; i < p->end; ++i) {
        s += p->arr[i];
    }
    p->result = s;
    return NULL;
}

/* Сумма массива arr из n элементов, посчитанная в num_threads потоках. */
long parallel_sum(const long *arr, size_t n, int num_threads) {
    if (n == 0) return 0;

    SumPart *parts = (SumPart *)malloc((size_t)num_threads * sizeof(SumPart));
    pthread_t *threads = (pthread_t *)malloc((size_t)num_threads * sizeof(pthread_t));

    size_t base = n / (size_t)num_threads;
    size_t rem  = n % (size_t)num_threads;
    size_t idx  = 0;

    for (int i = 0; i < num_threads; ++i) {
        size_t chunk = base + (((size_t)i < rem) ? 1 : 0);
        parts[i].arr    = arr;
        parts[i].begin  = idx;
        parts[i].end    = idx + chunk;
        parts[i].result = 0;
        idx += chunk;
        pthread_create(&threads[i], NULL, sum_worker, &parts[i]);
    }

    long total = 0;
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
        total += parts[i].result;
    }

    free(parts);
    free(threads);
    return total;
}

/* -------------------------------------------------------------------------
 * mutex_counter helpers
 * ---------------------------------------------------------------------- */

typedef struct {
    long            *counter;
    pthread_mutex_t *mutex;
    int              per_thread;
} CounterArg;

static void *counter_worker(void *arg) {
    CounterArg *a = (CounterArg *)arg;
    for (int i = 0; i < a->per_thread; ++i) {
        pthread_mutex_lock(a->mutex);
        (*a->counter)++;
        pthread_mutex_unlock(a->mutex);
    }
    return NULL;
}

/* num_threads потоков, каждый увеличивает общий счётчик per_thread раз. */
long mutex_counter(int num_threads, int per_thread) {
    long counter = 0;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    CounterArg *args    = (CounterArg *)malloc((size_t)num_threads * sizeof(CounterArg));
    pthread_t  *threads = (pthread_t  *)malloc((size_t)num_threads * sizeof(pthread_t));

    for (int i = 0; i < num_threads; ++i) {
        args[i].counter    = &counter;
        args[i].mutex      = &mutex;
        args[i].per_thread = per_thread;
        pthread_create(&threads[i], NULL, counter_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    free(args);
    free(threads);
    return counter;
}
