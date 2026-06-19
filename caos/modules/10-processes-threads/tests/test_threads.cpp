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
