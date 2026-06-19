#pragma once
#include <vector>
#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>

// Сумма элементов, посчитанная параллельно в num_threads потоках (num_threads >= 1).
long parallel_sum(const std::vector<long>& v, unsigned num_threads);

// num_threads потоков увеличивают ОБЩИЙ счётчик per_thread раз каждый.
// Синхронизация — через std::mutex. Возвращает итоговое значение счётчика.
long concurrent_increment(unsigned num_threads, unsigned per_thread);

// То же, но через std::atomic (без мьютекса).
long atomic_increment(unsigned num_threads, unsigned per_thread);

// -------------------------------------------------------------------------
// Задание 4. Потокобезопасный счётчик на mutex.
//
// Класс инкапсулирует общее значение и мьютекс. Любое число потоков может
// вызывать add()/get() одновременно — внутри стоит блокировка, поэтому гонки
// данных нет. В отличие от свободной функции concurrent_increment, здесь
// синхронизация — ОТВЕТСТВЕННОСТЬ САМОГО ОБЪЕКТА (инвариант инкапсуляции).
class SafeCounter {
public:
    SafeCounter() = default;

    // Прибавить delta к счётчику атомарно относительно других потоков.
    void add(long delta);

    // Увеличить на 1 (удобный частный случай).
    void increment();

    // Текущее значение (тоже под защитой — иначе читали бы «полузаписанное»).
    long get() const;

private:
    mutable std::mutex m_;   // mutable: get() помечен const, но мьютекс менять надо
    long value_ = 0;
};

// -------------------------------------------------------------------------
// Задание 5. Producer–consumer на mutex + condition_variable.
//
// Ограниченная по логике очередь: producer'ы кладут значения через push(),
// consumer'ы забирают через pop(). pop() БЛОКИРУЕТСЯ (засыпает на cv), пока
// очередь пуста и не объявлено о завершении. Когда все producer'ы закончили,
// вызывается close(): после этого pop() отдаёт остаток, а на пустой закрытой
// очереди возвращает std::nullopt (сигнал «данных больше не будет»).
class BlockingQueue {
public:
    BlockingQueue() = default;

    // Положить значение и разбудить один ждущий consumer.
    void push(int value);

    // Забрать значение. Блокирует, пока нет данных и очередь не закрыта.
    // Возвращает nullopt, только если очередь пуста И закрыта.
    std::optional<int> pop();

    // Объявить, что producer'ы закончили. Будит ВСЕХ ждущих consumer'ов.
    void close();

private:
    mutable std::mutex m_;
    std::condition_variable cv_;
    std::queue<int> q_;
    bool closed_ = false;
};

// Удобная обёртка для теста producer-consumer: num_producers потоков кладут
// в общую очередь числа 1..items_per_producer каждый, num_consumers потоков
// разбирают очередь и суммируют забранное. Возвращает суммарную сумму всех
// забранных значений (должна совпасть с суммой всего, что положили).
long producer_consumer_sum(unsigned num_producers, unsigned num_consumers,
                           unsigned items_per_producer);

// -------------------------------------------------------------------------
// Задание 6. Параллельная сумма с балансировкой остатка.
//
// Как parallel_sum, но БЕЗ допущения num_threads <= размера: должна работать
// при любом num_threads >= 1 (в т.ч. больше размера вектора и для пустого
// вектора). Результат обязан в точности совпадать с обычной (последовательной)
// суммой long. Разбивай так, чтобы остаток v.size() % num_threads
// распределялся по первым потокам (никакой элемент не потерян и не посчитан
// дважды).
long parallel_sum_balanced(const std::vector<long>& v, unsigned num_threads);
