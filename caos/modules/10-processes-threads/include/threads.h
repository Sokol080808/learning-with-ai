#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Сумма массива arr из n элементов, посчитанная в num_threads потоках.
 * Каждый поток складывает свою часть массива, затем главный поток
 * объединяет частичные суммы. Гарантируется: 1 <= num_threads <= n
 * (если n == 0 — корректно вернуть 0). Результат обязан совпадать с
 * обычной последовательной суммой. */
long parallel_sum(const long *arr, size_t n, int num_threads);

/* num_threads потоков, каждый увеличивает ОБЩИЙ счётчик per_thread раз.
 * Доступ к счётчику защищён pthread_mutex, чтобы не было гонки данных.
 * Возвращает итоговое значение счётчика — оно обязано быть ровно
 * num_threads * per_thread. */
long mutex_counter(int num_threads, int per_thread);

#ifdef __cplusplus
}
#endif
