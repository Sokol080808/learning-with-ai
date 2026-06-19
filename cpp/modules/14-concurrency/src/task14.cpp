#include "task14.hpp"
// #include <thread>
// #include <mutex>
// #include <atomic>

long parallel_sum(const std::vector<long>& v, unsigned num_threads) {
    // TODO: раздели массив на num_threads частей, посчитай частичные суммы в потоках,
    // затем сложи их. Подсказка: vector<long> partials(num_threads), каждый поток
    // пишет в СВОЙ partials[k] — общих изменяемых данных нет.
    (void)v; (void)num_threads;
    return 0;
}

long concurrent_increment(unsigned num_threads, unsigned per_thread) {
    // TODO: общий счётчик + std::mutex + std::lock_guard
    (void)num_threads; (void)per_thread;
    return 0;
}

long atomic_increment(unsigned num_threads, unsigned per_thread) {
    // TODO: std::atomic<long> без мьютекса
    (void)num_threads; (void)per_thread;
    return 0;
}
