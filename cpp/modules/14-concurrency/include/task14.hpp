#pragma once
#include <vector>

// Сумма элементов, посчитанная параллельно в num_threads потоках (num_threads >= 1).
long parallel_sum(const std::vector<long>& v, unsigned num_threads);

// num_threads потоков увеличивают ОБЩИЙ счётчик per_thread раз каждый.
// Синхронизация — через std::mutex. Возвращает итоговое значение счётчика.
long concurrent_increment(unsigned num_threads, unsigned per_thread);

// То же, но через std::atomic (без мьютекса).
long atomic_increment(unsigned num_threads, unsigned per_thread);
