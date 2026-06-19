#include <gtest/gtest.h>
#include <numeric>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <optional>
#include <random>
#include <algorithm>
#include "task14.hpp"

TEST(Concurrency, ParallelSum) {
    std::vector<long> v(1000);
    std::iota(v.begin(), v.end(), 1);  // 1..1000
    long expected = 1000L * 1001L / 2;  // 500500
    EXPECT_EQ(parallel_sum(v, 1), expected);
    EXPECT_EQ(parallel_sum(v, 4), expected);
    EXPECT_EQ(parallel_sum(v, 8), expected);
}

TEST(Concurrency, ParallelSumUneven) {
    std::vector<long> v{1, 2, 3, 4, 5, 6, 7};  // размер не делится на 3
    EXPECT_EQ(parallel_sum(v, 3), 28);
}

TEST(Concurrency, MutexCounter) {
    EXPECT_EQ(concurrent_increment(4, 50000), 200000L);
    EXPECT_EQ(concurrent_increment(1, 1000), 1000L);
}

TEST(Concurrency, AtomicCounter) {
    EXPECT_EQ(atomic_increment(8, 25000), 200000L);
    EXPECT_EQ(atomic_increment(2, 100), 200L);
}

// --------------------------- Задание 4: SafeCounter ---------------------------

TEST(SafeCounter, SingleThreadBasics) {
    SafeCounter c;
    EXPECT_EQ(c.get(), 0L);          // начальное значение
    c.increment();
    c.increment();
    EXPECT_EQ(c.get(), 2L);
    c.add(40);
    EXPECT_EQ(c.get(), 42L);
    c.add(-42);
    EXPECT_EQ(c.get(), 0L);
}

TEST(SafeCounter, ConcurrentIncrementsAreNotLost) {
    SafeCounter c;
    const unsigned threads = 8;
    const unsigned per = 50000;
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t) {
        pool.emplace_back([&c] {
            for (unsigned i = 0; i < per; ++i) c.increment();
        });
    }
    for (auto& th : pool) th.join();
    EXPECT_EQ(c.get(), static_cast<long>(threads) * per);  // 400000, ни один не потерян
}

TEST(SafeCounter, ConcurrentAddMixedSigns) {
    SafeCounter c;
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < 4; ++t)
        pool.emplace_back([&c] { for (int i = 0; i < 10000; ++i) c.add(3); });
    for (unsigned t = 0; t < 4; ++t)
        pool.emplace_back([&c] { for (int i = 0; i < 10000; ++i) c.add(-1); });
    for (auto& th : pool) th.join();
    // 4*10000*3 - 4*10000*1 = 120000 - 40000 = 80000
    EXPECT_EQ(c.get(), 80000L);
}

// ----------------------- Задание 5: producer-consumer -------------------------

TEST(BlockingQueue, PushThenPopReturnsValues) {
    BlockingQueue q;
    q.push(10);
    q.push(20);
    q.close();
    auto a = q.pop();
    auto b = q.pop();
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(a.value(), 10);   // FIFO порядок
    EXPECT_EQ(b.value(), 20);
    EXPECT_FALSE(q.pop().has_value());  // пусто и закрыто -> nullopt
}

TEST(BlockingQueue, PopUnblocksOnPush) {
    BlockingQueue q;
    std::optional<int> got;
    std::thread consumer([&] { got = q.pop(); });  // блокируется: очередь пуста
    // даём consumer'у уснуть, затем кладём значение
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    q.push(777);
    consumer.join();
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got.value(), 777);
}

TEST(BlockingQueue, PopUnblocksOnClose) {
    BlockingQueue q;
    std::atomic<bool> returned{false};
    std::optional<int> got{123};  // намеренно не nullopt, чтобы проверить запись
    std::thread consumer([&] {
        got = q.pop();            // обязан блокироваться, пока не вызван close()
        returned.store(true);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    // На пустой ОТКРЫТОЙ очереди pop() ещё не должен был вернуться.
    EXPECT_FALSE(returned.load());
    q.close();                    // должен разбудить и вернуть nullopt
    consumer.join();
    EXPECT_TRUE(returned.load());
    EXPECT_FALSE(got.has_value());
}

TEST(ProducerConsumer, TotalSumMatches) {
    // 3 producer'а, каждый кладёт 1..1000 -> сумма одного = 500500
    long expected = 3L * (1000L * 1001L / 2);  // 1501500
    EXPECT_EQ(producer_consumer_sum(3, 4, 1000), expected);
}

TEST(ProducerConsumer, SingleProducerSingleConsumer) {
    long expected = 100L * 101L / 2;  // 5050
    EXPECT_EQ(producer_consumer_sum(1, 1, 100), expected);
}

TEST(ProducerConsumer, MoreConsumersThanWork) {
    // потребителей больше, чем элементов — лишние должны корректно получить nullopt
    long expected = 5L * 6L / 2;  // 15
    EXPECT_EQ(producer_consumer_sum(1, 8, 5), expected);
}

// ------------------------- Задание 6: parallel_sum_balanced -------------------

TEST(ParallelSumBalanced, MatchesSequential) {
    std::vector<long> v(1000);
    std::iota(v.begin(), v.end(), 1);
    long expected = 1000L * 1001L / 2;  // 500500
    EXPECT_EQ(parallel_sum_balanced(v, 1), expected);
    EXPECT_EQ(parallel_sum_balanced(v, 3), expected);
    EXPECT_EQ(parallel_sum_balanced(v, 7), expected);
    EXPECT_EQ(parallel_sum_balanced(v, 16), expected);
}

TEST(ParallelSumBalanced, MoreThreadsThanElements) {
    std::vector<long> v{10, 20, 30};
    EXPECT_EQ(parallel_sum_balanced(v, 10), 60L);  // 7 потоков получают пустые куски
}

TEST(ParallelSumBalanced, EmptyVector) {
    std::vector<long> v;
    EXPECT_EQ(parallel_sum_balanced(v, 4), 0L);
    EXPECT_EQ(parallel_sum_balanced(v, 1), 0L);
}

TEST(ParallelSumBalanced, NegativeValues) {
    std::vector<long> v{-5, 3, -2, 10, -1, -1};  // сумма = 4
    EXPECT_EQ(parallel_sum_balanced(v, 4), 4L);
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Здесь мы НЕ проверяем конкретные числа, а формулируем ИНВАРИАНТЫ, которые
// обязаны выполняться для ЛЮБОГО входа:
//   * параллельная сумма == std::accumulate (оракул) и не зависит от числа потоков;
//   * счётчики не теряют и не дублируют инкременты (итог == точное произведение);
//   * producer-consumer переносит ровно столько, сколько положили (сохранение суммы);
//   * BlockingQueue сохраняет FIFO-порядок и отдаёт nullopt на пустой закрытой очереди.
// Все генераторы детерминированы: один и тот же сид -> один и тот же прогон, CI не «мигает».

namespace {

// Граница диапазона значений выбрана так, чтобы суммы заведомо не переполняли long
// даже на самых длинных векторах в этих тестах.
constexpr long kValMax = 1'000'000L;

// Оракул: честная последовательная сумма в long через std::accumulate.
long seq_sum(const std::vector<long>& v) {
    return std::accumulate(v.begin(), v.end(), 0L);
}

}  // namespace

// --- Задания 1 и 6: параллельная сумма как инвариант относительно оракула ---

// parallel_sum: для случайных векторов (без перекоса по потокам) результат
// в точности равен std::accumulate, при ЛЮБОМ корректном числе потоков 1..size.
TEST(ConcurrencyProps, ParallelSumEqualsAccumulate) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<long> val(-kValMax, kValMax);
    for (int iter = 0; iter < 250; ++iter) {
        // parallel_sum предполагает num_threads <= размера, поэтому size >= 1.
        std::uniform_int_distribution<int> sz(1, 400);
        const int n = sz(rng);
        std::vector<long> v(static_cast<std::size_t>(n));
        for (long& x : v) x = val(rng);

        const long oracle = seq_sum(v);
        std::uniform_int_distribution<int> th(1, n);  // 1..n потоков
        for (int t = 0; t < 4; ++t) {
            const unsigned nt = static_cast<unsigned>(th(rng));
            EXPECT_EQ(parallel_sum(v, nt), oracle)
                << "n=" << n << " threads=" << nt;
        }
    }
}

// parallel_sum_balanced: тот же инвариант, но число потоков НЕ ограничено размером —
// допускаются пустой вектор и потоков больше, чем элементов (лишние дают 0).
TEST(ConcurrencyProps, ParallelSumBalancedEqualsAccumulate) {
    std::mt19937 rng(0xBA1A11CEu);
    std::uniform_int_distribution<long> val(-kValMax, kValMax);
    std::uniform_int_distribution<int> sz(0, 300);    // в т.ч. 0 (пустой)
    std::uniform_int_distribution<int> th(1, 64);     // в т.ч. потоков > size
    for (int iter = 0; iter < 300; ++iter) {
        const int n = sz(rng);
        std::vector<long> v(static_cast<std::size_t>(n));
        for (long& x : v) x = val(rng);

        const long oracle = seq_sum(v);
        const unsigned nt = static_cast<unsigned>(th(rng));
        EXPECT_EQ(parallel_sum_balanced(v, nt), oracle)
            << "n=" << n << " threads=" << nt;
    }
}

// Инвариант независимости от разбиения: для фиксированного входа все варианты
// числа потоков дают ОДИН И ТОТ ЖЕ результат (распараллеливание не меняет сумму).
TEST(ConcurrencyProps, BalancedInvariantOverThreadCount) {
    std::mt19937 rng(0xD15EA5Eu);
    std::uniform_int_distribution<long> val(-kValMax, kValMax);
    std::uniform_int_distribution<int> sz(0, 200);
    for (int iter = 0; iter < 120; ++iter) {
        const int n = sz(rng);
        std::vector<long> v(static_cast<std::size_t>(n));
        for (long& x : v) x = val(rng);

        const long base = parallel_sum_balanced(v, 1);
        EXPECT_EQ(base, seq_sum(v));
        for (unsigned nt : {2u, 3u, 5u, 8u, 13u, 32u, 100u}) {
            EXPECT_EQ(parallel_sum_balanced(v, nt), base)
                << "n=" << n << " threads=" << nt;
        }
    }
}

// Явный экстремальный край: одинаковые элементы и потоков сильно больше размера.
TEST(ConcurrencyProps, BalancedEdgeManyThreadsFewElements) {
    std::mt19937 rng(0x5EEDu);
    std::uniform_int_distribution<long> val(-1000, 1000);
    for (int iter = 0; iter < 80; ++iter) {
        const int n = std::uniform_int_distribution<int>(0, 5)(rng);
        const long fill = val(rng);
        std::vector<long> v(static_cast<std::size_t>(n), fill);
        const long oracle = static_cast<long>(n) * fill;
        for (unsigned nt : {1u, 4u, 16u, 64u, 256u}) {
            EXPECT_EQ(parallel_sum_balanced(v, nt), oracle)
                << "n=" << n << " threads=" << nt;
        }
    }
}

// --- Задания 2 и 3: счётчики не теряют инкременты ---

// concurrent_increment (mutex) и atomic_increment должны дать ТОЧНО
// num_threads * per_thread — ни одного потерянного или лишнего инкремента —
// и совпасть друг с другом для одинаковых параметров.
TEST(ConcurrencyProps, IncrementCountersExactAndAgree) {
    std::mt19937 rng(0xC0DE1234u);
    std::uniform_int_distribution<unsigned> th(1, 8);
    std::uniform_int_distribution<unsigned> per(0, 5000);  // в т.ч. 0 раз
    for (int iter = 0; iter < 60; ++iter) {
        const unsigned nt = th(rng);
        const unsigned pk = per(rng);
        const long expected = static_cast<long>(nt) * static_cast<long>(pk);
        EXPECT_EQ(concurrent_increment(nt, pk), expected)
            << "threads=" << nt << " per=" << pk;
        EXPECT_EQ(atomic_increment(nt, pk), expected)
            << "threads=" << nt << " per=" << pk;
    }
}

// --- Задание 4: SafeCounter ---

// Последовательный инвариант: get() равен сумме всех применённых delta (оракул).
TEST(SafeCounterProps, SingleThreadEqualsRunningSum) {
    std::mt19937 rng(0x5AFEC0DEu);
    std::uniform_int_distribution<long> delta(-10'000, 10'000);
    for (int iter = 0; iter < 200; ++iter) {
        SafeCounter c;
        long oracle = 0;
        const int ops = std::uniform_int_distribution<int>(0, 50)(rng);
        for (int i = 0; i < ops; ++i) {
            const long d = delta(rng);
            c.add(d);
            oracle += d;
        }
        EXPECT_EQ(c.get(), oracle) << "ops=" << ops;
    }
}

// Параллельный инвариант: каждый поток прибавляет известную локальную сумму
// случайных delta; итоговое get() обязано равняться сумме этих локальных сумм
// (никакая модификация не потеряна из-за гонки).
TEST(SafeCounterProps, ConcurrentAddsPreserveTotal) {
    std::mt19937 rng(0xABCDEF01u);
    std::uniform_int_distribution<unsigned> nthreads(1, 6);
    std::uniform_int_distribution<int> nops(0, 2000);
    std::uniform_int_distribution<long> delta(-100, 100);
    for (int iter = 0; iter < 40; ++iter) {
        const unsigned T = nthreads(rng);
        // Заранее генерируем планы работ для каждого потока и их ожидаемую сумму.
        std::vector<std::vector<long>> plans(T);
        long oracle = 0;
        for (unsigned t = 0; t < T; ++t) {
            const int k = nops(rng);
            plans[t].reserve(static_cast<std::size_t>(k));
            for (int i = 0; i < k; ++i) {
                const long d = delta(rng);
                plans[t].push_back(d);
                oracle += d;
            }
        }
        SafeCounter c;
        std::vector<std::thread> pool;
        pool.reserve(T);
        for (unsigned t = 0; t < T; ++t)
            pool.emplace_back([&c, &plans, t] {
                for (long d : plans[t]) c.add(d);
            });
        for (auto& th : pool) th.join();
        EXPECT_EQ(c.get(), oracle) << "threads=" << T;
    }
}

// --- Задание 5: BlockingQueue / producer_consumer_sum ---

// Сохранение суммы: что положили все producer'ы, то и забрали все consumer'ы —
// при ЛЮБЫХ числах producer'ов / consumer'ов (в т.ч. потребителей больше работы).
TEST(ProducerConsumerProps, TotalSumIsConserved) {
    std::mt19937 rng(0xFEEDFACEu);
    std::uniform_int_distribution<unsigned> prod(1, 5);
    std::uniform_int_distribution<unsigned> cons(1, 6);
    std::uniform_int_distribution<unsigned> items(0, 300);  // в т.ч. 0 элементов
    for (int iter = 0; iter < 50; ++iter) {
        const unsigned P = prod(rng);
        const unsigned C = cons(rng);
        const unsigned K = items(rng);
        // Каждый producer кладёт 1..K -> сумма одного = K*(K+1)/2.
        const long per_producer = static_cast<long>(K) * (static_cast<long>(K) + 1) / 2;
        const long expected = static_cast<long>(P) * per_producer;
        EXPECT_EQ(producer_consumer_sum(P, C, K), expected)
            << "P=" << P << " C=" << C << " K=" << K;
    }
}

// FIFO-инвариант BlockingQueue на одном потоке: то, что закрытая очередь отдаёт
// через pop(), есть в точности последовательность push'ей в исходном порядке,
// после чего — nullopt (пусто и закрыто).
TEST(BlockingQueueProps, SingleThreadRoundTripPreservesOrder) {
    std::mt19937 rng(0x0DDBA11u);
    std::uniform_int_distribution<int> val(-100000, 100000);
    for (int iter = 0; iter < 200; ++iter) {
        const int n = std::uniform_int_distribution<int>(0, 60)(rng);
        std::vector<int> input(static_cast<std::size_t>(n));
        for (int& x : input) x = val(rng);

        BlockingQueue q;
        for (int x : input) q.push(x);
        q.close();

        std::vector<int> drained;
        drained.reserve(input.size());
        while (auto v = q.pop()) drained.push_back(*v);

        // Round-trip: вытащенное совпадает со вставленным и по порядку (FIFO).
        EXPECT_EQ(drained, input) << "n=" << n;
        // На уже опустошённой закрытой очереди pop() стабильно отдаёт nullopt.
        EXPECT_FALSE(q.pop().has_value());
        EXPECT_FALSE(q.pop().has_value());
    }
}

// Перестановочный инвариант при многих consumer'ах: значения могут достаться
// разным потокам в произвольном порядке, но МУЛЬТИМНОЖЕСТВО забранного совпадает
// с мультимножеством положенного (ничего не потеряно и не продублировано).
TEST(BlockingQueueProps, MultiConsumerDrainIsPermutationOfPushed) {
    std::mt19937 rng(0x13371337u);
    for (int iter = 0; iter < 25; ++iter) {
        const int n = std::uniform_int_distribution<int>(0, 500)(rng);
        std::uniform_int_distribution<int> val(-1000, 1000);
        std::vector<int> input(static_cast<std::size_t>(n));
        for (int& x : input) x = val(rng);

        BlockingQueue q;
        for (int x : input) q.push(x);
        q.close();  // все данные уже в очереди, можно закрывать сразу

        const unsigned C = std::uniform_int_distribution<unsigned>(1, 6)(rng);
        std::vector<std::vector<int>> got(C);
        std::vector<std::thread> consumers;
        consumers.reserve(C);
        for (unsigned c = 0; c < C; ++c)
            consumers.emplace_back([&q, &got, c] {
                while (auto v = q.pop()) got[c].push_back(*v);
            });
        for (auto& th : consumers) th.join();

        std::vector<int> all;
        all.reserve(input.size());
        for (auto& part : got)
            all.insert(all.end(), part.begin(), part.end());

        // Сравнение мультимножеств через сортировку обеих сторон.
        std::vector<int> a = input, b = all;
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        EXPECT_EQ(b, a) << "n=" << n << " consumers=" << C;
        // После полного дренажа закрытая очередь отдаёт nullopt.
        EXPECT_FALSE(q.pop().has_value());
    }
}
