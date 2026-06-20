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

/* -------------------------------------------------------------------------
 * producer_consumer: ограниченный кольцевой буфер
 * ---------------------------------------------------------------------- */

/* Общее «рабочее место» всех потоков: сам кольцевой буфер, его геометрия
 * (head/tail/count/cap) и средства синхронизации (один замок + два условия).
 * Всё это — разделяемое состояние, поэтому трогать его можно только под mutex. */
typedef struct {
    long           *buf;       /* кольцевой буфер на cap элементов            */
    int             cap;       /* ёмкость (buf_cap), задана в рантайме        */
    int             head;      /* откуда читает consumer                      */
    int             tail;      /* куда пишет producer                         */
    int             count;     /* сколько элементов сейчас в буфере           */

    pthread_mutex_t mutex;
    pthread_cond_t  not_full;  /* «появилось свободное место»                 */
    pthread_cond_t  not_empty; /* «появился элемент»                          */

    /* Сколько ещё элементов всего будет произведено. Когда дойдёт до 0 и
     * буфер опустеет — работать больше не над чем, consumer'ы выходят. */
    long            remaining;

    long            sum;       /* общий аккумулятор результата                */
} PCQueue;

typedef struct {
    PCQueue *q;
    int      id;               /* номер производителя p (0..n_producers-1)    */
    int      items;            /* items_per_producer                          */
} ProducerArg;

static void *pc_producer(void *arg) {
    ProducerArg *pa = (ProducerArg *)arg;
    PCQueue *q = pa->q;

    for (int i = 0; i < pa->items; ++i) {
        long value = (long)pa->id * pa->items + i + 1;

        pthread_mutex_lock(&q->mutex);
        /* Буфер полон — ждём, пока consumer освободит место. while, а не if:
         * после пробуждения условие надо перепроверить (ложные побудки и
         * гонка за замок с другими producer'ами). */
        while (q->count == q->cap) {
            pthread_cond_wait(&q->not_full, &q->mutex);
        }
        q->buf[q->tail] = value;
        q->tail = (q->tail + 1) % q->cap;
        q->count++;
        /* Разбудить одного ждущего consumer'а: появился элемент. */
        pthread_cond_signal(&q->not_empty);
        pthread_mutex_unlock(&q->mutex);
    }
    return NULL;
}

static void *pc_consumer(void *arg) {
    PCQueue *q = (PCQueue *)arg;

    for (;;) {
        pthread_mutex_lock(&q->mutex);
        /* Ждём, пока в буфере что-то появится. Но если работы больше не
         * осталось (remaining == 0) и буфер пуст — выходим: иначе зависнем
         * на not_empty навсегда. */
        while (q->count == 0 && q->remaining > 0) {
            pthread_cond_wait(&q->not_empty, &q->mutex);
        }
        if (q->count == 0 && q->remaining == 0) {
            pthread_mutex_unlock(&q->mutex);
            break;
        }
        long value = q->buf[q->head];
        q->head = (q->head + 1) % q->cap;
        q->count--;
        q->remaining--;
        q->sum += value;

        /* Освободилось место — будим producer'а. Если работа кончилась
         * (remaining дошёл до 0), будим ВСЕХ consumer'ов broadcast'ом, чтобы
         * спящие на not_empty проснулись и увидели условие выхода. */
        pthread_cond_signal(&q->not_full);
        if (q->remaining == 0) {
            pthread_cond_broadcast(&q->not_empty);
        }
        pthread_mutex_unlock(&q->mutex);
    }
    return NULL;
}

/* Ограниченный producer-consumer на кольцевом буфере. */
long producer_consumer(int n_producers, int n_consumers,
                       int buf_cap, int items_per_producer) {
    PCQueue q;
    q.buf       = (long *)malloc((size_t)buf_cap * sizeof(long));
    q.cap       = buf_cap;
    q.head      = 0;
    q.tail      = 0;
    q.count     = 0;
    q.remaining = (long)n_producers * items_per_producer;
    q.sum       = 0;
    pthread_mutex_init(&q.mutex, NULL);
    pthread_cond_init(&q.not_full, NULL);
    pthread_cond_init(&q.not_empty, NULL);

    pthread_t  *prod_t = (pthread_t *)malloc((size_t)n_producers * sizeof(pthread_t));
    pthread_t  *cons_t = (pthread_t *)malloc((size_t)n_consumers * sizeof(pthread_t));
    ProducerArg *pargs = (ProducerArg *)malloc((size_t)n_producers * sizeof(ProducerArg));

    for (int p = 0; p < n_producers; ++p) {
        pargs[p].q     = &q;
        pargs[p].id    = p;
        pargs[p].items = items_per_producer;
        pthread_create(&prod_t[p], NULL, pc_producer, &pargs[p]);
    }
    for (int c = 0; c < n_consumers; ++c) {
        pthread_create(&cons_t[c], NULL, pc_consumer, &q);
    }

    for (int p = 0; p < n_producers; ++p) {
        pthread_join(prod_t[p], NULL);
    }
    /* Все producer'ы закончили. На случай, если consumer'ы уже спят на
     * not_empty (буфер пуст, но они ещё не видели remaining): когда
     * remaining дойдёт до 0 внутри consumer'а, он сам сделает broadcast.
     * Здесь дополнительный broadcast не повредит — если remaining уже 0,
     * он гарантированно разбудит всех спящих. */
    pthread_mutex_lock(&q.mutex);
    pthread_cond_broadcast(&q.not_empty);
    pthread_mutex_unlock(&q.mutex);

    for (int c = 0; c < n_consumers; ++c) {
        pthread_join(cons_t[c], NULL);
    }

    long result = q.sum;

    pthread_mutex_destroy(&q.mutex);
    pthread_cond_destroy(&q.not_full);
    pthread_cond_destroy(&q.not_empty);
    free(q.buf);
    free(prod_t);
    free(cons_t);
    free(pargs);
    return result;
}
