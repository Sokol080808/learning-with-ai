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
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <tuple>
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
    // Эталонный ответ: одна неделимая операция, возвращает значение ДО прибавления.
    return cnt.fetch_add(delta);
}

// Атомарно обновляет best до max(best, candidate) и больше ничего.
// Должно быть корректно при одновременном вызове из многих потоков:
// итоговое best = максимум из всех candidate и стартового значения.
// Реализуй через цикл compare_exchange_weak (CAS-loop), без мьютекса.
inline void atomic_store_max(std::atomic<long>& best, long candidate) {
    // Эталонный ответ: CAS-loop. Читаем текущее best, и пока candidate больше,
    // пытаемся записать его; compare_exchange_weak обновляет prev при провале.
    long prev = best.load(std::memory_order_relaxed);
    while (candidate > prev &&
           !best.compare_exchange_weak(prev, candidate,
                                       std::memory_order_release,
                                       std::memory_order_relaxed)) {
        // prev обновлён актуальным значением — повторяем проверку и попытку.
    }
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
        // Эталонный ответ: крутимся, пока флаг был уже занят (вернул true).
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // busy-wait: другой поток держит блокировку.
        }
    }

    // Освободить блокировку.
    void unlock() {
        // Эталонный ответ: сбрасываем флаг с release-семантикой.
        flag_.clear(std::memory_order_release);
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
        // Эталонный ответ: кладём под мьютексом, затем будим одного ждущего.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    // Заблокироваться, пока очередь пуста; затем вынуть и вернуть фронт.
    // Используй cv_.wait(lock, predicate) — это защищает от ложных пробуждений.
    T pop() {
        // Эталонный ответ: ждём непустую очередь, затем снимаем фронт.
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    // Неблокирующая попытка. Если очередь пуста — вернуть false и не трогать out.
    // Иначе снять фронт в out и вернуть true.
    bool try_pop(T& out) {
        // Эталонный ответ: без блокировки на условии; пусто -> false.
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop();
        return true;
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
        // Эталонный ответ: поднимаем worker_count потоков на worker_loop().
        workers_.reserve(worker_count);
        for (std::size_t i = 0; i < worker_count; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        // Эталонный ответ: ставим флаг остановки, будим всех, ждём завершения.
        // Воркеры доедают уже принятые задачи (см. условие в worker_loop).
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

    // Поставить задачу f(args...) в очередь. Вернуть future с результатом.
    // Подсказка по типу результата: std::invoke_result_t<F, Args...>.
    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;

        // Эталонный ответ: упаковываем вызов f(args...) в packaged_task<R()>,
        // забираем future до постановки задачи в очередь, кладём обёртку void()
        // в очередь под мьютексом и будим одного воркера.
        auto bound = [func = std::forward<F>(f),
                      tup = std::make_tuple(std::forward<Args>(args)...)]() mutable -> R {
            return std::apply(std::move(func), std::move(tup));
        };
        // shared_ptr, потому что std::function требует копируемости, а
        // packaged_task — move-only. Через shared_ptr обёртка остаётся копируемой.
        auto task = std::make_shared<std::packaged_task<R()>>(std::move(bound));
        std::future<R> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stop_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }
            tasks_.emplace([task] { (*task)(); });
        }
        cv_.notify_one();
        return result;
    }

private:
    void worker_loop() {
        // Эталонный ответ: спим, пока есть работа или пока не попросили остановиться.
        // Останавливаемся ТОЛЬКО когда stop_ выставлен И очередь пуста — это
        // гарантирует, что все уже принятые задачи будут выполнены.
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) {
                    return;
                }
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
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
    // Эталонный ответ: делим вектор на num_threads примерно равных кусков,
    // каждый поток считает свою частичную сумму в СВОЙ слот (без общего
    // состояния), затем складываем слоты в главном потоке.
    const std::size_t n = v.size();
    if (n == 0) {
        return 0;
    }
    if (num_threads < 1) {
        num_threads = 1;
    }
    // Не имеет смысла заводить больше потоков, чем элементов.
    if (num_threads > n) {
        num_threads = static_cast<unsigned>(n);
    }

    std::vector<long> partials(num_threads, 0);
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    const std::size_t base = n / num_threads;
    const std::size_t rem = n % num_threads;

    std::size_t begin = 0;
    for (unsigned t = 0; t < num_threads; ++t) {
        // Первые rem кусков получают на один элемент больше — куски «примерно равны».
        const std::size_t len = base + (t < rem ? 1 : 0);
        const std::size_t end = begin + len;
        threads.emplace_back([&v, &partials, t, begin, end] {
            long s = 0;
            for (std::size_t i = begin; i < end; ++i) {
                s += v[i];
            }
            partials[t] = s;
        });
        begin = end;
    }

    for (auto& th : threads) {
        th.join();
    }

    long total = 0;
    for (long p : partials) {
        total += p;
    }
    return total;
}

}  // namespace m20
