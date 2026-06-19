#include <gtest/gtest.h>
#include <numeric>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <optional>
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
