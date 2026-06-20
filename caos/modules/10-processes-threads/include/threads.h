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

/* Ограниченный producer-consumer на кольцевом буфере.
 *
 * Запускает n_producers потоков-производителей и n_consumers потоков-
 * потребителей вокруг ОДНОГО кольцевого буфера ёмкости buf_cap (ёмкость
 * задаётся в рантайме). Каждый производитель кладёт в буфер items_per_producer
 * элементов; значение i-го элемента производителя p равно (p * items_per_producer
 * + i + 1) — то есть все элементы вместе образуют числа 1..(n_producers *
 * items_per_producer) в каком-то порядке. Потребители забирают элементы и
 * суммируют их в общий аккумулятор.
 *
 * Буфер ограничен: producer ждёт на условии not_full, если буфер полон;
 * consumer ждёт на условии not_empty, если буфер пуст. Синхронизация —
 * один pthread_mutex_t + два pthread_cond_t (not_full / not_empty).
 *
 * Возвращает сумму всех обработанных потребителями элементов. При корректной
 * реализации она обязана совпасть с последовательным оракулом
 *   sum = T*(T+1)/2,  где T = n_producers * items_per_producer
 * — ни один элемент не потерян и не задвоен.
 *
 * Гарантируется: n_producers >= 1, n_consumers >= 1, buf_cap >= 1,
 * items_per_producer >= 0. */
long producer_consumer(int n_producers, int n_consumers,
                       int buf_cap, int items_per_producer);

#ifdef __cplusplus
}
#endif
