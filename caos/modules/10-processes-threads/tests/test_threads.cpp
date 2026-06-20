// Эти тесты трогать не нужно — это эталон поведения.
#include <gtest/gtest.h>

#include <vector>

extern "C" {
#include "threads.h"
}

// --- parallel_sum ----------------------------------------------------------

// Сумма не зависит от числа потоков — она обязана совпасть с обычной суммой.
TEST(ParallelSum, OneThreadSimple) {
    std::vector<long> a = {1, 2, 3, 4, 5};
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 1), 15);
}

TEST(ParallelSum, ManyThreadsSameResult) {
    std::vector<long> a = {1, 2, 3, 4, 5, 6, 7, 8};
    // 8 элементов, сумма 36 — при любом разумном числе потоков.
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 1), 36);
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 2), 36);
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 3), 36);
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 4), 36);
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 8), 36);
}

// num_threads не делит n нацело — куски разной длины, сумма та же.
TEST(ParallelSum, UnevenSplit) {
    std::vector<long> a = {10, 20, 30, 40, 50, 60, 70};  // сумма 280
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 3), 280);
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 4), 280);
}

// num_threads == n — каждому потоку по одному элементу.
TEST(ParallelSum, ThreadPerElement) {
    std::vector<long> a = {2, 4, 6, 8};
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 4), 20);
}

TEST(ParallelSum, NegativeNumbers) {
    std::vector<long> a = {-5, 5, -10, 10, -3};
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 2), -3);
}

// Пустой массив: суммировать нечего.
TEST(ParallelSum, EmptyArray) {
    EXPECT_EQ(parallel_sum(nullptr, 0, 1), 0);
}

// Большой массив 1..1000 => сумма 500500. Проверяем, что ничего не теряется
// и не задваивается при разбиении на потоки.
TEST(ParallelSum, LargeArray) {
    std::vector<long> a;
    long expected = 0;
    for (long i = 1; i <= 1000; ++i) {
        a.push_back(i);
        expected += i;
    }
    EXPECT_EQ(parallel_sum(a.data(), a.size(), 7), expected);  // 500500
}

// --- mutex_counter ---------------------------------------------------------

// Итог обязан быть ровно num_threads * per_thread — без потерь от гонки.
TEST(MutexCounter, SmallCase) {
    EXPECT_EQ(mutex_counter(1, 1000), 1000);
}

TEST(MutexCounter, FourThreads) {
    EXPECT_EQ(mutex_counter(4, 1000), 4000);
}

// Много потоков и много инкрементов — здесь и ловится потеря из-за гонки,
// если забыть мьютекс.
TEST(MutexCounter, ContendedNoLostUpdates) {
    EXPECT_EQ(mutex_counter(8, 100000), 800000);
}

TEST(MutexCounter, ZeroPerThread) {
    EXPECT_EQ(mutex_counter(4, 0), 0);
}

// ============================================================
// Randomised tests (seed 0xC0FFEE, deterministic)
// ============================================================
#include <random>
#include <numeric>

// --- parallel_sum randomised -------------------------------------------

// Oracle: compare parallel_sum against std::accumulate for 500 random
// (array, num_threads) pairs. Covers uneven splits, negative values,
// large n, and thread counts from 1 up to n.
TEST(ParallelSum, RandomOracleVsSequential) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<size_t> size_dist(1, 200);
    std::uniform_int_distribution<long>   val_dist(-100000L, 100000L);

    for (int iter = 0; iter < 500; ++iter) {
        size_t n = size_dist(rng);
        std::vector<long> arr(n);
        for (auto &x : arr) x = val_dist(rng);

        long expected = std::accumulate(arr.begin(), arr.end(), 0L);

        // Try several thread counts for the same array: 1, mid, and n.
        std::uniform_int_distribution<int> t_dist(1, static_cast<int>(n));
        int t = t_dist(rng);

        EXPECT_EQ(parallel_sum(arr.data(), n, 1),        expected)
            << "iter=" << iter << " n=" << n << " threads=1";
        EXPECT_EQ(parallel_sum(arr.data(), n, t),        expected)
            << "iter=" << iter << " n=" << n << " threads=" << t;
        EXPECT_EQ(parallel_sum(arr.data(), n, (int)n),   expected)
            << "iter=" << iter << " n=" << n << " threads=n";
    }
}

// All-same value: sum == n * value regardless of threading.
TEST(ParallelSum, AllSameValueRandomized) {
    std::mt19937 rng(0xC0FFEE ^ 1);
    std::uniform_int_distribution<size_t> size_dist(1, 300);
    std::uniform_int_distribution<long>   val_dist(-500L, 500L);

    for (int iter = 0; iter < 500; ++iter) {
        size_t n = size_dist(rng);
        long v = val_dist(rng);
        std::vector<long> arr(n, v);
        long expected = v * static_cast<long>(n);

        std::uniform_int_distribution<int> t_dist(1, (int)n);
        int t = t_dist(rng);

        EXPECT_EQ(parallel_sum(arr.data(), n, t), expected)
            << "iter=" << iter << " n=" << n << " v=" << v << " t=" << t;
    }
}

// Edge: n == 1 with 1 thread; the single-element case.
TEST(ParallelSum, SingleElementEdge) {
    std::mt19937 rng(0xC0FFEE ^ 2);
    std::uniform_int_distribution<long> val_dist(-1000000L, 1000000L);
    for (int iter = 0; iter < 500; ++iter) {
        long v = val_dist(rng);
        EXPECT_EQ(parallel_sum(&v, 1, 1), v) << "iter=" << iter;
    }
}

// Prefix-sum invariant: sum of first k elements via parallel_sum must equal
// std::accumulate for the same prefix. Stresses arbitrary sub-lengths.
TEST(ParallelSum, PrefixLengthsMatchSequential) {
    std::mt19937 rng(0xC0FFEE ^ 3);
    std::uniform_int_distribution<long> val_dist(-9999L, 9999L);

    // Build one fixed-size array and check all prefix lengths.
    constexpr size_t N = 64;
    std::vector<long> arr(N);
    for (auto &x : arr) x = val_dist(rng);

    for (size_t k = 1; k <= N; ++k) {
        long expected = std::accumulate(arr.begin(), arr.begin() + (int)k, 0L);
        // use min(k, 8) threads to avoid requiring num_threads <= n constraint
        int t = (int)std::min(k, (size_t)8);
        EXPECT_EQ(parallel_sum(arr.data(), k, t), expected) << "k=" << k;
    }
}

// --- mutex_counter randomised ------------------------------------------

// Oracle: result must equal exactly num_threads * per_thread for all
// random (num_threads, per_thread) pairs.
TEST(MutexCounter, RandomExactProduct) {
    std::mt19937 rng(0xC0FFEE ^ 4);
    std::uniform_int_distribution<int> t_dist(1, 16);
    std::uniform_int_distribution<int> p_dist(0, 5000);

    for (int iter = 0; iter < 500; ++iter) {
        int t = t_dist(rng);
        int p = p_dist(rng);
        long expected = (long)t * p;
        EXPECT_EQ(mutex_counter(t, p), expected)
            << "iter=" << iter << " threads=" << t << " per_thread=" << p;
    }
}

// Boundary: 1 thread, many increments — no contention, result must be exact.
TEST(MutexCounter, SingleThreadManyIncrements) {
    std::mt19937 rng(0xC0FFEE ^ 5);
    std::uniform_int_distribution<int> p_dist(1, 100000);
    for (int iter = 0; iter < 200; ++iter) {
        int p = p_dist(rng);
        EXPECT_EQ(mutex_counter(1, p), (long)p) << "iter=" << iter;
    }
}

// Boundary: many threads, 1 increment each — maximises scheduling races.
TEST(MutexCounter, ManyThreadsOneIncrement) {
    std::mt19937 rng(0xC0FFEE ^ 6);
    std::uniform_int_distribution<int> t_dist(2, 32);
    for (int iter = 0; iter < 300; ++iter) {
        int t = t_dist(rng);
        EXPECT_EQ(mutex_counter(t, 1), (long)t)
            << "iter=" << iter << " threads=" << t;
    }
}

// High-contention stress: 8 threads x 50000 increments, repeated 20 times.
// A data race would cause a different result on nearly every run.
TEST(MutexCounter, HighContentionStress) {
    for (int iter = 0; iter < 20; ++iter) {
        EXPECT_EQ(mutex_counter(8, 50000), 400000LL)
            << "iter=" << iter;
    }
}
