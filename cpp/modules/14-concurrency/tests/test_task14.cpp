#include <gtest/gtest.h>
#include <numeric>
#include <vector>
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
