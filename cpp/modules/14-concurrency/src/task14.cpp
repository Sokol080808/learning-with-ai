#include "task14.hpp"
// #include <thread>
// #include <mutex>
// #include <atomic>
// #include <condition_variable>
// #include <future>

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

// ---------------------------- Задание 4 ----------------------------------

void SafeCounter::add(long delta) {
    // TODO: захвати m_ через std::lock_guard и прибавь delta к value_.
    (void)delta;
}

void SafeCounter::increment() {
    // TODO: проще всего — вызвать add(1).
}

long SafeCounter::get() const {
    // TODO: под защитой m_ верни value_. Сейчас возвращаем заведомо неверное.
    return -1;
}

// ---------------------------- Задание 5 ----------------------------------

void BlockingQueue::push(int value) {
    // TODO: под lock_guard положи value в q_ и разбуди один ждущий поток
    // (cv_.notify_one()). notify лучше делать ПОСЛЕ освобождения мьютекса
    // или хотя бы после самого push — но главное не забудь его сделать.
    (void)value;
}

std::optional<int> BlockingQueue::pop() {
    // TODO: возьми std::unique_lock, и cv_.wait(lock, predicate), где predicate —
    // «очередь не пуста ИЛИ закрыта». После пробуждения: если есть элемент —
    // сними его с фронта и верни; если очередь пуста и closed_ — верни nullopt.
    // Сейчас стаб не блокирует и сразу врёт.
    return std::nullopt;
}

void BlockingQueue::close() {
    // TODO: под lock_guard выставь closed_ = true и cv_.notify_all().
}

long producer_consumer_sum(unsigned num_producers, unsigned num_consumers,
                           unsigned items_per_producer) {
    // TODO: создай BlockingQueue. Запусти num_producers потоков, каждый
    // push'ит 1..items_per_producer. Запусти num_consumers потоков, каждый в
    // цикле pop() пока приходит значение, и копит локальную сумму. Когда все
    // producer'ы сделали join() — вызови queue.close(), затем join consumer'ов
    // и сложи их локальные суммы. Верни общий итог.
    (void)num_producers; (void)num_consumers; (void)items_per_producer;
    return -1;
}

// ---------------------------- Задание 6 ----------------------------------

long parallel_sum_balanced(const std::vector<long>& v, unsigned num_threads) {
    // TODO: как parallel_sum, но корректно при ЛЮБОМ num_threads >= 1 (в т.ч.
    // больше size и для пустого вектора). Распредели остаток size % num_threads
    // по первым потокам: первые (size % num_threads) кусков на 1 элемент длиннее.
    // Потоки без элементов просто дают 0. Результат == обычной сумме.
    (void)v; (void)num_threads;
    return -1;
}

// ---------------------------- Задание 7 ----------------------------------

long parallel_sum_async(const std::vector<long>& xs, int parts) {
    // TODO: разбей xs на parts частей (сбалансировано: первые rem кусков длиннее на 1),
    // запусти каждую через std::async(std::launch::async, ...) и собери результаты
    // через future.get(). parts <= 0 приравнивай к 1. Лишние futures
    // (пустые диапазоны) возвращают 0. Подсказка: #include <future> в шапке файла.
    (void)xs; (void)parts;
    return -1;
}
