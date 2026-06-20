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

// Сумма массива arr из n элементов, посчитанная в num_threads потоках.
// Разбей массив на num_threads примерно равных кусков, посчитай сумму
// каждого куска в своём потоке, дождись всех (join) и сложи частичные суммы.
long parallel_sum(const long *arr, size_t n, int num_threads) {
    (void)arr;
    (void)n;
    (void)num_threads;
    return 0; // TODO: посчитай сумму массива в num_threads потоках
}

// num_threads потоков, каждый увеличивает общий счётчик per_thread раз.
// Заведи ОДИН общий счётчик (long) и ОДИН мьютекс. Каждый поток в цикле
// per_thread раз: lock -> counter++ -> unlock. Дождись всех и верни counter.
long mutex_counter(int num_threads, int per_thread) {
    (void)num_threads;
    (void)per_thread;
    return 0; // TODO: посчитай под мьютексом, верни итог счётчика
}

// Ограниченный producer-consumer на кольцевом буфере.
// n_producers потоков-производителей каждый кладёт items_per_producer элементов
// в общий кольцевой буфер ёмкостью buf_cap; n_consumers потоков-потребителей
// читают из буфера и суммируют значения. Верни итоговую сумму всех элементов.
// Подсказка: тебе понадобится мьютекс и два условия (not_full, not_empty).
long producer_consumer(int n_producers, int n_consumers,
                       int buf_cap, int items_per_producer) {
    (void)n_producers;
    (void)n_consumers;
    (void)buf_cap;
    (void)items_per_producer;
    return 0; // TODO: реализуй producer-consumer с кольцевым буфером
}
