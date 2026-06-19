#pragma once
//
// Модуль 20 — Многопоточность: atomics, future/promise, пул потоков.
//
// Весь код этого модуля — шаблонный или inline, поэтому он живёт прямо в этом
// заголовке (как в модуле 07). Реализуй тела, помеченные // TODO, и сделай
// тесты зелёными.
//
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace m20 {

// ===========================================================================
// Задание 1. Атомарные операции.
// ===========================================================================

// Атомарно прибавляет delta к счётчику cnt (cnt += delta как ОДНА неделимая
// операция). Возвращает значение счётчика ДО прибавления (как fetch_add).
// Несколько потоков могут звать это одновременно — потерь быть не должно.
inline long atomic_add(std::atomic<long>& cnt, long delta) {
    // TODO: используй cnt.fetch_add(...)
    (void)delta;
    return cnt.load();  // заглушка: ничего не прибавляет
}

// Атомарно обновляет best до max(best, candidate) и больше ничего.
// Должно быть корректно при одновременном вызове из многих потоков:
// итоговое best = максимум из всех candidate и стартового значения.
// Реализуй через цикл compare_exchange_weak (CAS-loop), без мьютекса.
inline void atomic_store_max(std::atomic<long>& best, long candidate) {
    // TODO: CAS-loop на compare_exchange_weak.
    (void)best;
    (void)candidate;
    // заглушка: не обновляет best
}

// ===========================================================================
// Задание 2. SpinLock на std::atomic_flag.
// ===========================================================================

// Простейший мьютекс, который НЕ усыпляет поток, а «крутится» в цикле, пока
// не захватит флаг. Должен удовлетворять требованиям BasicLockable
// (методы lock()/unlock()), чтобы работать с std::lock_guard.
class SpinLock {
public:
    SpinLock() = default;
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

    // Захватить блокировку: крутиться, пока test_and_set возвращает true.
    void lock() {
        // TODO: цикл на flag_.test_and_set(std::memory_order_acquire)
    }

    // Освободить блокировку.
    void unlock() {
        // TODO: flag_.clear(std::memory_order_release)
    }

private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

// ===========================================================================
// Задание 3. ThreadSafeQueue<T> — потокобезопасная очередь.
// ===========================================================================

// Очередь FIFO на std::mutex + std::condition_variable.
// push() кладёт элемент и будит одного ждущего.
// pop() БЛОКИРУЕТСЯ, пока очередь пуста, затем снимает и возвращает фронт.
// try_pop() не блокируется: возвращает false, если очередь пуста.
template <class T>
class ThreadSafeQueue {
public:
    // Положить элемент в конец и разбудить одного ждущего в pop().
    void push(T value) {
        // TODO: захвати мьютекс, положи в очередь, notify_one().
        (void)value;
    }

    // Заблокироваться, пока очередь пуста; затем вынуть и вернуть фронт.
    // Используй cv_.wait(lock, predicate) — это защищает от ложных пробуждений.
    T pop() {
        // TODO: wait на условие !queue_.empty(), затем снять фронт.
        return T{};  // заглушка: ничего не ждёт и не снимает
    }

    // Неблокирующая попытка. Если очередь пуста — вернуть false и не трогать out.
    // Иначе снять фронт в out и вернуть true.
    bool try_pop(T& out) {
        // TODO
        (void)out;
        return false;  // заглушка: всегда «пусто»
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex      mutex_;
    std::condition_variable cv_;
    std::queue<T>           queue_;
};

// ===========================================================================
// Задание 4. ThreadPool — пул потоков с submit() -> std::future.
// ===========================================================================

// Создаёт worker_count потоков, которые разбирают задачи из общей очереди.
// submit(f, args...) кладёт задачу в очередь и возвращает std::future с её
// результатом. Деструктор должен корректно завершить все потоки (дождаться,
// пока они доедят очередь / выйдут), без зависаний и без отбрасывания
// уже принятых задач.
class ThreadPool {
public:
    explicit ThreadPool(std::size_t worker_count) {
        // TODO: запусти worker_count потоков, каждый крутит worker_loop().
        (void)worker_count;
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        // TODO: попроси потоки остановиться, разбуди их, join() каждого.
    }

    // Поставить задачу f(args...) в очередь. Вернуть future с результатом.
    // Подсказка по типу результата: std::invoke_result_t<F, Args...>.
    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;

        // TODO:
        //  1) сделай std::packaged_task<R()>, связав f с аргументами
        //     (например, через лямбду, захватывающую их по значению);
        //  2) возьми future из task до того, как отдашь task в очередь;
        //  3) положи задачу в очередь под мьютексом и notify_one();
        //  4) верни future.
        (void)f;
        ((void)args, ...);

        std::promise<R> p;
        // заглушка: возвращаем future от пустого promise (никогда не будет готов
        // с правильным значением) — тест на это и ловится.
        return p.get_future();
    }

private:
    void worker_loop() {
        // TODO: пока не остановлены ИЛИ очередь не пуста — бери задачу и зови её.
    }

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        mutex_;
    std::condition_variable           cv_;
    bool                              stop_ = false;
};

// ===========================================================================
// Задание 5. parallel_sum — параллельная сумма большого вектора.
// ===========================================================================

// Сумма всех элементов v, посчитанная в num_threads потоках. Должна СОВПАДАТЬ
// с обычной последовательной суммой. num_threads >= 1. Пустой вектор -> 0.
// Раздели вектор на num_threads примерно равных кусков, каждый поток считает
// свою частичную сумму в СВОЮ переменную (без общего состояния), затем сложи.
long parallel_sum(const std::vector<long>& v, unsigned num_threads);

inline long parallel_sum(const std::vector<long>& v, unsigned num_threads) {
    // TODO: раздели на куски, посчитай в потоках, сложи частичные суммы.
    (void)v;
    (void)num_threads;
    return 0;  // заглушка
}

}  // namespace m20
