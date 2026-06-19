#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <numeric>
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
