#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "task20.hpp"

using namespace std::chrono_literals;

// ===========================================================================
// Задание 1. atomic_add / atomic_store_max
// ===========================================================================

TEST(AtomicAdd, ReturnsOldValueAndAdds) {
    std::atomic<long> cnt{10};
    EXPECT_EQ(m20::atomic_add(cnt, 5), 10);  // вернуло старое значение
    EXPECT_EQ(cnt.load(), 15);               // и прибавило
    EXPECT_EQ(m20::atomic_add(cnt, -15), 15);
    EXPECT_EQ(cnt.load(), 0);
}

TEST(AtomicAdd, NoLostUpdatesUnderContention) {
    std::atomic<long> cnt{0};
    const unsigned kThreads = 8;
    const long kPer = 100000;
    std::vector<std::thread> ts;
    for (unsigned t = 0; t < kThreads; ++t) {
        ts.emplace_back([&] {
            for (long i = 0; i < kPer; ++i) m20::atomic_add(cnt, 1);
        });
    }
    for (auto& th : ts) th.join();
    EXPECT_EQ(cnt.load(), static_cast<long>(kThreads) * kPer);
}

TEST(AtomicStoreMax, KeepsLargerOnly) {
    std::atomic<long> best{0};
    m20::atomic_store_max(best, 5);
    EXPECT_EQ(best.load(), 5);
    m20::atomic_store_max(best, 3);   // меньше — не меняет
    EXPECT_EQ(best.load(), 5);
    m20::atomic_store_max(best, 42);  // больше — обновляет
    EXPECT_EQ(best.load(), 42);
    m20::atomic_store_max(best, -7);
    EXPECT_EQ(best.load(), 42);
}

TEST(AtomicStoreMax, FindsGlobalMaxUnderContention) {
    std::atomic<long> best{0};
    const unsigned kThreads = 8;
    std::vector<std::thread> ts;
    for (unsigned t = 0; t < kThreads; ++t) {
        ts.emplace_back([&, t] {
            for (long i = 0; i < 50000; ++i) {
                m20::atomic_store_max(best, static_cast<long>(t) * 100000 + i);
            }
        });
    }
    for (auto& th : ts) th.join();
    // Максимум достигается потоком t=7 при i=49999.
    EXPECT_EQ(best.load(), 7L * 100000 + 49999);
}

// ===========================================================================
// Задание 2. SpinLock
// ===========================================================================

TEST(SpinLock, ProtectsSharedCounter) {
    m20::SpinLock lk;
    long counter = 0;  // НЕ atomic — корректность держится только на блокировке
    const unsigned kThreads = 4;
    const long kPer = 50000;
    std::vector<std::thread> ts;
    for (unsigned t = 0; t < kThreads; ++t) {
        ts.emplace_back([&] {
            for (long i = 0; i < kPer; ++i) {
                std::lock_guard<m20::SpinLock> g(lk);
                ++counter;
            }
        });
    }
    for (auto& th : ts) th.join();
    EXPECT_EQ(counter, static_cast<long>(kThreads) * kPer);
}

TEST(SpinLock, LockUnlockIsReusable) {
    m20::SpinLock lk;
    lk.lock();
    lk.unlock();
    // Если unlock реально освобождает флаг, второй захват не зависнет.
    lk.lock();
    lk.unlock();
    SUCCEED();
}

// ===========================================================================
// Задание 3. ThreadSafeQueue
// ===========================================================================

TEST(ThreadSafeQueue, PushThenPopFifo) {
    m20::ThreadSafeQueue<int> q;
    EXPECT_TRUE(q.empty());
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.pop(), 1);
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), 3);
    EXPECT_TRUE(q.empty());
}

TEST(ThreadSafeQueue, TryPopOnEmptyReturnsFalse) {
    m20::ThreadSafeQueue<std::string> q;
    std::string out = "untouched";
    EXPECT_FALSE(q.try_pop(out));
    EXPECT_EQ(out, "untouched");  // out не тронут
    q.push("hello");
    EXPECT_TRUE(q.try_pop(out));
    EXPECT_EQ(out, "hello");
    EXPECT_FALSE(q.try_pop(out));
}

TEST(ThreadSafeQueue, PopBlocksUntilPush) {
    m20::ThreadSafeQueue<int> q;
    std::atomic<bool> got{false};
    int value = 0;
    std::thread consumer([&] {
        value = q.pop();  // должен заблокироваться, пока producer не положит
        got.store(true);
    });
    // Пока ничего не положили — pop обязан спать, got остаётся false.
    std::this_thread::sleep_for(80ms);
    EXPECT_FALSE(got.load());
    q.push(99);
    consumer.join();
    EXPECT_TRUE(got.load());
    EXPECT_EQ(value, 99);
}

TEST(ThreadSafeQueue, ManyProducersConsumersConserveItems) {
    m20::ThreadSafeQueue<int> q;
    const int kItems = 20000;
    std::thread producer([&] {
        for (int i = 0; i < kItems; ++i) q.push(i);
    });
    long long sum = 0;
    std::thread consumer([&] {
        for (int i = 0; i < kItems; ++i) sum += q.pop();
    });
    producer.join();
    consumer.join();
    long long expected = static_cast<long long>(kItems - 1) * kItems / 2;
    EXPECT_EQ(sum, expected);
    EXPECT_TRUE(q.empty());
}

// ===========================================================================
// Задание 4. ThreadPool
// ===========================================================================

TEST(ThreadPool, SubmitReturnsFutureWithResult) {
    m20::ThreadPool pool(4);
    auto f = pool.submit([](int a, int b) { return a + b; }, 20, 22);
    EXPECT_EQ(f.get(), 42);
}

TEST(ThreadPool, RunsManyTasksCorrectly) {
    m20::ThreadPool pool(4);
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 200; ++i) {
        futures.push_back(pool.submit([](int x) { return x * x; }, i));
    }
    long long sum = 0;
    for (int i = 0; i < 200; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
        sum += i * i;
    }
    EXPECT_GT(sum, 0);
}

TEST(ThreadPool, VoidTasksAllRunBeforeDestruction) {
    std::atomic<int> done{0};
    {
        m20::ThreadPool pool(3);
        for (int i = 0; i < 500; ++i) {
            pool.submit([&done] { done.fetch_add(1); });
        }
        // Деструктор pool на выходе из блока должен дождаться доедания очереди.
    }
    EXPECT_EQ(done.load(), 500);
}

TEST(ThreadPool, TasksRunOnMultipleThreads) {
    m20::ThreadPool pool(4);
    auto f = pool.submit([] {
        std::this_thread::sleep_for(20ms);
        return std::this_thread::get_id();
    });
    auto g = pool.submit([] {
        std::this_thread::sleep_for(20ms);
        return std::this_thread::get_id();
    });
    // Сам факт, что оба future доходят до готовности — уже проверка работы пула.
    auto id1 = f.get();
    auto id2 = g.get();
    EXPECT_NE(id1, std::this_thread::get_id());
    EXPECT_NE(id2, std::this_thread::get_id());
}

// ===========================================================================
// Задание 5. parallel_sum
// ===========================================================================

TEST(ParallelSum, MatchesSequentialSum) {
    std::vector<long> v(1000003);
    for (std::size_t i = 0; i < v.size(); ++i) {
        v[i] = static_cast<long>((i % 7) - 3);  // и плюсы, и минусы
    }
    long expected = std::accumulate(v.begin(), v.end(), 0L);
    EXPECT_EQ(m20::parallel_sum(v, 1), expected);
    EXPECT_EQ(m20::parallel_sum(v, 4), expected);
    EXPECT_EQ(m20::parallel_sum(v, 8), expected);
    EXPECT_EQ(m20::parallel_sum(v, 13), expected);  // потоков больше делителей
}

TEST(ParallelSum, EmptyVectorIsZero) {
    std::vector<long> v;
    EXPECT_EQ(m20::parallel_sum(v, 4), 0);
}

TEST(ParallelSum, SmallerThanThreads) {
    std::vector<long> v{10, 20, 30};
    EXPECT_EQ(m20::parallel_sum(v, 8), 60);  // потоков больше, чем элементов
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Каждый тест прогоняет много сгенерированных входов и проверяет ИНВАРИАНТ,
// а не конкретный пример. RNG с фиксированным сидом — результат воспроизводим,
// CI не «мигает». Оракул, где он есть, — это std::-аналог (fetch_add, running
// max, std::accumulate) или однопоточный последовательный результат.

// --- Задание 1: atomic_add как однопоточный оракул std::accumulate ---------
// Инвариант: серия atomic_add(delta_i) на одном потоке оставляет счётчик равным
// сумме всех delta (плюс старт), а каждый вызов возвращает значение ДО прибавки
// (как fetch_add). Покрываем плюсы, минусы и ноль.
TEST(AtomicProps, SingleThreadActsLikeFetchAdd) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<long> start_dist(-1000, 1000);
    std::uniform_int_distribution<long> delta_dist(-500, 500);
    std::uniform_int_distribution<int> count_dist(0, 40);

    for (int iter = 0; iter < 400; ++iter) {
        const long start = start_dist(rng);
        std::atomic<long> cnt{start};
        long mirror = start;  // независимый «обычный» счётчик-оракул
        const int steps = count_dist(rng);
        for (int s = 0; s < steps; ++s) {
            const long delta = delta_dist(rng);
            const long returned = m20::atomic_add(cnt, delta);
            EXPECT_EQ(returned, mirror);  // вернулось значение ДО прибавления
            mirror += delta;
            EXPECT_EQ(cnt.load(), mirror);  // и счётчик догнал оракула
        }
        EXPECT_EQ(cnt.load(), mirror);
    }
}

// Инвариант: atomic_store_max после серии кандидатов = max(старт, всех кандидатов);
// сравниваем с обычным running-max оракулом. И монотонность: best не убывает.
TEST(AtomicProps, StoreMaxEqualsRunningMaximum) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<long> start_dist(-100000, 100000);
    std::uniform_int_distribution<long> cand_dist(-1000000, 1000000);
    std::uniform_int_distribution<int> count_dist(0, 60);

    for (int iter = 0; iter < 400; ++iter) {
        const long start = start_dist(rng);
        std::atomic<long> best{start};
        long oracle = start;
        const int steps = count_dist(rng);
        for (int s = 0; s < steps; ++s) {
            const long cand = cand_dist(rng);
            const long before = best.load();
            m20::atomic_store_max(best, cand);
            oracle = std::max(oracle, cand);
            EXPECT_EQ(best.load(), oracle);     // совпало с оракулом
            EXPECT_GE(best.load(), before);     // монотонно не убывает
            EXPECT_GE(best.load(), cand);       // не меньше только что поданного
        }
        EXPECT_EQ(best.load(), oracle);
    }
}

// Многопоточный инвариант для atomic_store_max: при гонке многих потоков
// итоговый best обязан быть ГЛОБАЛЬНЫМ максимумом всех поданных кандидатов.
TEST(AtomicProps, StoreMaxFindsGlobalMaxUnderContentionRandom) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<long> val_dist(-1000000, 1000000);

    for (int iter = 0; iter < 30; ++iter) {
        const unsigned kThreads = 4;
        const int kPer = 2000;
        std::vector<std::vector<long>> data(kThreads);
        long global_max = std::numeric_limits<long>::min();
        for (unsigned t = 0; t < kThreads; ++t) {
            data[t].reserve(kPer);
            for (int i = 0; i < kPer; ++i) {
                const long x = val_dist(rng);
                data[t].push_back(x);
                global_max = std::max(global_max, x);
            }
        }
        std::atomic<long> best{std::numeric_limits<long>::min()};
        std::vector<std::thread> ts;
        for (unsigned t = 0; t < kThreads; ++t) {
            ts.emplace_back([&best, &data, t] {
                for (long x : data[t]) m20::atomic_store_max(best, x);
            });
        }
        for (auto& th : ts) th.join();
        EXPECT_EQ(best.load(), global_max);
    }
}

// --- Задание 2: SpinLock защищает не-atomic счётчик от потери обновлений -----
// Инвариант: сумма приращений под lock_guard всегда точна, для случайного
// числа потоков и приращений на поток. Без блокировки тест бы «терял» инкременты.
TEST(SpinLockProps, ProtectsCounterForRandomWorkloads) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> thread_dist(1, 6);
    std::uniform_int_distribution<long> per_dist(0, 20000);

    for (int iter = 0; iter < 40; ++iter) {
        const int kThreads = thread_dist(rng);
        const long kPer = per_dist(rng);
        m20::SpinLock lk;
        long counter = 0;  // НЕ atomic: корректность держится только на SpinLock
        std::vector<std::thread> ts;
        for (int t = 0; t < kThreads; ++t) {
            ts.emplace_back([&lk, &counter, kPer] {
                for (long i = 0; i < kPer; ++i) {
                    std::lock_guard<m20::SpinLock> g(lk);
                    ++counter;
                }
            });
        }
        for (auto& th : ts) th.join();
        EXPECT_EQ(counter, static_cast<long>(kThreads) * kPer);
    }
}

// Инвариант реюзабельности: многократный lock/unlock в одном потоке никогда
// не должен зависать (после unlock следующий lock сразу проходит).
TEST(SpinLockProps, ManyLockUnlockCyclesNeverDeadlock) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> cyc_dist(1, 500);
    for (int iter = 0; iter < 200; ++iter) {
        m20::SpinLock lk;
        const int cycles = cyc_dist(rng);
        for (int c = 0; c < cycles; ++c) {
            lk.lock();
            lk.unlock();
        }
        SUCCEED();
    }
}

// --- Задание 3: ThreadSafeQueue round-trip и FIFO --------------------------
// Инвариант round-trip + порядка: что положили в очередь, то и вынимаем — в том
// же FIFO-порядке. Сверяем с std::queue-оракулом на случайной последовательности.
TEST(ThreadSafeQueueProps, PushPopRoundTripIsFifo) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> n_dist(0, 200);
    std::uniform_int_distribution<int> val_dist(-100000, 100000);

    for (int iter = 0; iter < 300; ++iter) {
        const int n = n_dist(rng);
        std::vector<int> items;
        items.reserve(n);
        m20::ThreadSafeQueue<int> q;
        EXPECT_TRUE(q.empty());
        for (int i = 0; i < n; ++i) {
            const int x = val_dist(rng);
            items.push_back(x);
            q.push(x);
        }
        EXPECT_EQ(q.size(), static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(q.pop(), items[i]);  // тот же элемент в том же порядке
        }
        EXPECT_TRUE(q.empty());
    }
}

// Инвариант try_pop: на пустой очереди -> false и out НЕ тронут; на непустой ->
// true и out == ожидаемый фронт. Перемежаем случайные push и try_pop, ведём
// эталонную std::queue.
TEST(ThreadSafeQueueProps, TryPopMatchesReferenceQueue) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> op_dist(0, 1);
    std::uniform_int_distribution<int> val_dist(-1000, 1000);

    for (int iter = 0; iter < 200; ++iter) {
        m20::ThreadSafeQueue<int> q;
        std::queue<int> oracle;
        const int ops = 300;
        for (int o = 0; o < ops; ++o) {
            if (op_dist(rng) == 0) {  // push
                const int x = val_dist(rng);
                q.push(x);
                oracle.push(x);
            } else {  // try_pop
                const int sentinel = 0x7FFFFFFF;
                int out = sentinel;
                const bool ok = q.try_pop(out);
                if (oracle.empty()) {
                    EXPECT_FALSE(ok);
                    EXPECT_EQ(out, sentinel);  // out нетронут
                } else {
                    EXPECT_TRUE(ok);
                    EXPECT_EQ(out, oracle.front());
                    oracle.pop();
                }
            }
            EXPECT_EQ(q.size(), oracle.size());
        }
    }
}

// Многопоточный инвариант сохранения: с произвольным числом продьюсеров и одним
// блокирующим консьюмером сумма и количество вынутых элементов сохраняются.
TEST(ThreadSafeQueueProps, ConservesItemsUnderRandomProducers) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> prod_dist(1, 4);
    std::uniform_int_distribution<int> per_dist(100, 3000);

    for (int iter = 0; iter < 25; ++iter) {
        const int producers = prod_dist(rng);
        const int per = per_dist(rng);
        const int total = producers * per;
        m20::ThreadSafeQueue<long long> q;

        std::vector<std::thread> prods;
        long long expected_sum = 0;
        for (int p = 0; p < producers; ++p) {
            const long long base = static_cast<long long>(p) * 1000000;
            for (int i = 0; i < per; ++i) expected_sum += base + i;
            prods.emplace_back([&q, base, per] {
                for (int i = 0; i < per; ++i) q.push(base + i);
            });
        }
        long long got_sum = 0;
        int got_count = 0;
        std::thread consumer([&] {
            for (int i = 0; i < total; ++i) {
                got_sum += q.pop();  // блокируется, пока продьюсеры не догонят
                ++got_count;
            }
        });
        for (auto& t : prods) t.join();
        consumer.join();
        EXPECT_EQ(got_count, total);
        EXPECT_EQ(got_sum, expected_sum);
        EXPECT_TRUE(q.empty());
    }
}

// --- Задание 4: ThreadPool — future несёт ровно результат задачи -----------
// Инвариант: для пакета задач f(x)=a*x+b каждая future отдаёт ровно
// последовательно вычисленный результат; ничего не теряется и не путается.
TEST(ThreadPoolProps, FuturesCarryExactResults) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<unsigned> wc_dist(1, 6);
    std::uniform_int_distribution<int> n_dist(0, 300);
    std::uniform_int_distribution<long long> coef_dist(-50, 50);

    for (int iter = 0; iter < 40; ++iter) {
        const unsigned workers = wc_dist(rng);
        const int n = n_dist(rng);
        const long long a = coef_dist(rng);
        const long long b = coef_dist(rng);
        m20::ThreadPool pool(workers);
        std::vector<std::future<long long>> futs;
        std::vector<long long> expected;
        futs.reserve(n);
        expected.reserve(n);
        for (int i = 0; i < n; ++i) {
            expected.push_back(a * i + b);
            futs.push_back(pool.submit(
                [a, b](long long x) { return a * x + b; },
                static_cast<long long>(i)));
        }
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(futs[i].get(), expected[i]);
        }
    }
}

// Инвариант сохранения для void-задач: все принятые задачи выполняются до
// разрушения пула — счётчик ровно равен числу submit'ов, при случайном числе
// воркеров и задач. Проверяет, что деструктор «доедает» очередь.
TEST(ThreadPoolProps, AllVoidTasksRunBeforeDestruction) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<unsigned> wc_dist(1, 5);
    std::uniform_int_distribution<int> n_dist(0, 800);

    for (int iter = 0; iter < 30; ++iter) {
        const unsigned workers = wc_dist(rng);
        const int n = n_dist(rng);
        std::atomic<int> done{0};
        std::atomic<long long> sum{0};
        long long expected_sum = 0;
        {
            m20::ThreadPool pool(workers);
            for (int i = 0; i < n; ++i) {
                expected_sum += i;
                pool.submit([&done, &sum, i] {
                    done.fetch_add(1);
                    sum.fetch_add(i);
                });
            }
            // выход из блока -> деструктор обязан дождаться доедания очереди
        }
        EXPECT_EQ(done.load(), n);
        EXPECT_EQ(sum.load(), expected_sum);
    }
}

// --- Задание 5: parallel_sum совпадает с последовательной суммой ------------
// Инвариант-оракул: для любого вектора и любого num_threads >= 1 результат
// равен std::accumulate. Числа подобраны так, чтобы long не переполнялся.
TEST(ParallelSumProps, MatchesAccumulateForRandomVectorsAndThreads) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<std::size_t> size_dist(0, 5000);
    std::uniform_int_distribution<long> val_dist(-100000, 100000);
    std::uniform_int_distribution<unsigned> thr_dist(1, 16);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = size_dist(rng);
        std::vector<long> v(n);
        for (std::size_t i = 0; i < n; ++i) v[i] = val_dist(rng);
        const long expected = std::accumulate(v.begin(), v.end(), 0L);
        const unsigned threads = thr_dist(rng);
        EXPECT_EQ(m20::parallel_sum(v, threads), expected);
    }
}

// Инвариант независимости от числа потоков: один и тот же вектор даёт один и тот
// же результат при любом num_threads (включая > размера и = 1). И равен оракулу.
TEST(ParallelSumProps, ResultIndependentOfThreadCount) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<std::size_t> size_dist(0, 2000);
    std::uniform_int_distribution<long> val_dist(-50000, 50000);

    for (int iter = 0; iter < 200; ++iter) {
        const std::size_t n = size_dist(rng);
        std::vector<long> v(n);
        for (std::size_t i = 0; i < n; ++i) v[i] = val_dist(rng);
        const long expected = std::accumulate(v.begin(), v.end(), 0L);
        // num_threads: 1, несколько, и заведомо больше n (граница «потоков > элементов»)
        EXPECT_EQ(m20::parallel_sum(v, 1), expected);
        EXPECT_EQ(m20::parallel_sum(v, 3), expected);
        EXPECT_EQ(m20::parallel_sum(v, 7), expected);
        EXPECT_EQ(m20::parallel_sum(v, static_cast<unsigned>(n) + 5), expected);
    }
}

// Явные экстремальные краевые случаи для parallel_sum.
TEST(ParallelSumProps, ExtremeEdgeCases) {
    // Пустой вектор -> 0 при любом числе потоков.
    std::vector<long> empty;
    EXPECT_EQ(m20::parallel_sum(empty, 1), 0L);
    EXPECT_EQ(m20::parallel_sum(empty, 8), 0L);

    // Один элемент.
    std::vector<long> one{-12345};
    EXPECT_EQ(m20::parallel_sum(one, 1), -12345L);
    EXPECT_EQ(m20::parallel_sum(one, 4), -12345L);

    // Все нули.
    std::vector<long> zeros(1000, 0L);
    EXPECT_EQ(m20::parallel_sum(zeros, 7), 0L);

    // Сумма ровно ноль (симметричные ±).
    std::vector<long> pm;
    for (long i = 1; i <= 1000; ++i) { pm.push_back(i); pm.push_back(-i); }
    EXPECT_EQ(m20::parallel_sum(pm, 5), 0L);

    // Граница n % num_threads != 0: куски «примерно равны».
    std::vector<long> v(1001);
    std::iota(v.begin(), v.end(), 1L);  // 1..1001
    const long expected = 1001L * 1002L / 2L;
    EXPECT_EQ(m20::parallel_sum(v, 8), expected);
    EXPECT_EQ(m20::parallel_sum(v, 13), expected);
}
