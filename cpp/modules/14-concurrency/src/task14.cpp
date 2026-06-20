#include "task14.hpp"
// Эталонный ключ ответов: исходный, идиоматичный C++20 (НЕ для выдачи учащимся).
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <future>
#include <numeric>

long parallel_sum(const std::vector<long>& v, unsigned num_threads) {
    if (num_threads == 0) num_threads = 1;
    const std::size_t n = v.size();
    // Каждый поток пишет в СВОЙ partials[k] — общих изменяемых данных нет.
    std::vector<long> partials(num_threads, 0);
    std::vector<std::thread> pool;
    pool.reserve(num_threads);

    const std::size_t chunk = n / num_threads;
    const std::size_t rem = n % num_threads;

    std::size_t begin = 0;
    for (unsigned k = 0; k < num_threads; ++k) {
        // Первые rem кусков на 1 элемент длиннее — остаток распределён.
        const std::size_t len = chunk + (k < rem ? 1 : 0);
        const std::size_t end = begin + len;
        pool.emplace_back([&v, &partials, k, begin, end] {
            long s = 0;
            for (std::size_t i = begin; i < end; ++i) s += v[i];
            partials[k] = s;
        });
        begin = end;
    }
    for (auto& th : pool) th.join();

    long total = 0;
    for (long p : partials) total += p;
    return total;
}

long concurrent_increment(unsigned num_threads, unsigned per_thread) {
    long counter = 0;
    std::mutex m;
    std::vector<std::thread> pool;
    pool.reserve(num_threads);
    for (unsigned t = 0; t < num_threads; ++t) {
        pool.emplace_back([&counter, &m, per_thread] {
            for (unsigned i = 0; i < per_thread; ++i) {
                std::lock_guard<std::mutex> lock(m);
                ++counter;
            }
        });
    }
    for (auto& th : pool) th.join();
    return counter;
}

long atomic_increment(unsigned num_threads, unsigned per_thread) {
    std::atomic<long> counter{0};
    std::vector<std::thread> pool;
    pool.reserve(num_threads);
    for (unsigned t = 0; t < num_threads; ++t) {
        pool.emplace_back([&counter, per_thread] {
            for (unsigned i = 0; i < per_thread; ++i)
                counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    for (auto& th : pool) th.join();
    return counter.load();
}

// ---------------------------- Задание 4 ----------------------------------

void SafeCounter::add(long delta) {
    std::lock_guard<std::mutex> lock(m_);
    value_ += delta;
}

void SafeCounter::increment() {
    add(1);
}

long SafeCounter::get() const {
    std::lock_guard<std::mutex> lock(m_);
    return value_;
}

// ---------------------------- Задание 5 ----------------------------------

void BlockingQueue::push(int value) {
    {
        std::lock_guard<std::mutex> lock(m_);
        q_.push(value);
    }
    cv_.notify_one();
}

std::optional<int> BlockingQueue::pop() {
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock, [this] { return !q_.empty() || closed_; });
    if (!q_.empty()) {
        int value = q_.front();
        q_.pop();
        return value;
    }
    // Очередь пуста и закрыта — данных больше не будет.
    return std::nullopt;
}

void BlockingQueue::close() {
    {
        std::lock_guard<std::mutex> lock(m_);
        closed_ = true;
    }
    cv_.notify_all();
}

long producer_consumer_sum(unsigned num_producers, unsigned num_consumers,
                           unsigned items_per_producer) {
    BlockingQueue queue;

    std::vector<std::thread> producers;
    producers.reserve(num_producers);
    for (unsigned p = 0; p < num_producers; ++p) {
        producers.emplace_back([&queue, items_per_producer] {
            for (unsigned i = 1; i <= items_per_producer; ++i)
                queue.push(static_cast<int>(i));
        });
    }

    std::vector<long> partials(num_consumers, 0);
    std::vector<std::thread> consumers;
    consumers.reserve(num_consumers);
    for (unsigned c = 0; c < num_consumers; ++c) {
        consumers.emplace_back([&queue, &partials, c] {
            long local = 0;
            while (auto value = queue.pop())
                local += *value;
            partials[c] = local;
        });
    }

    for (auto& th : producers) th.join();
    queue.close();  // только после того как все producer'ы закончили
    for (auto& th : consumers) th.join();

    long total = 0;
    for (long s : partials) total += s;
    return total;
}

// ---------------------------- Задание 7 ----------------------------------
// Параллельная сумма через std::async + std::future.
// Каждая задача принимает указатели на начало и конец диапазона по значению,
// чтобы лямбда не захватывала итераторы, чей стек-фрейм мог бы устареть.
// Ни мьютексов, ни общих переменных — канал future/promise сам заботится об
// happens-before между вычислителем и f.get().

long parallel_sum_async(const std::vector<long>& xs, int parts) {
    if (parts <= 0) parts = 1;
    const std::size_t n = xs.size();

    // Balanced-разбиение: первые rem кусков на 1 элемент длиннее.
    const auto P = static_cast<std::size_t>(parts);
    const std::size_t base = n / P;
    const std::size_t rem  = n % P;

    std::vector<std::future<long>> futures;
    futures.reserve(P);

    const long* data = xs.data();   // сырой указатель — захватываем по значению
    std::size_t begin = 0;
    for (std::size_t k = 0; k < P; ++k) {
        const std::size_t len = base + (k < rem ? 1u : 0u);
        const std::size_t end = begin + len;
        // Захватываем data, begin, end по значению — поток будет жить дольше итерации.
        futures.push_back(
            std::async(std::launch::async,
                [data, begin, end]() -> long {
                    return std::accumulate(data + begin, data + end, 0L);
                }));
        begin = end;
    }

    long total = 0;
    for (auto& f : futures) total += f.get();
    return total;
}

// ---------------------------- Задание 6 ----------------------------------

long parallel_sum_balanced(const std::vector<long>& v, unsigned num_threads) {
    if (num_threads == 0) num_threads = 1;
    const std::size_t n = v.size();

    std::vector<long> partials(num_threads, 0);
    std::vector<std::thread> pool;
    pool.reserve(num_threads);

    const std::size_t chunk = n / num_threads;
    const std::size_t rem = n % num_threads;

    std::size_t begin = 0;
    for (unsigned k = 0; k < num_threads; ++k) {
        // Первые rem кусков на 1 элемент длиннее; потоки без элементов дают 0.
        const std::size_t len = chunk + (k < rem ? 1 : 0);
        const std::size_t end = begin + len;
        pool.emplace_back([&v, &partials, k, begin, end] {
            long s = 0;
            for (std::size_t i = begin; i < end; ++i) s += v[i];
            partials[k] = s;
        });
        begin = end;
    }
    for (auto& th : pool) th.join();

    long total = 0;
    for (long p : partials) total += p;
    return total;
}
