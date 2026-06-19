#include <gtest/gtest.h>
#include "buggy.hpp"

TEST(Buggy, SumFirstN) {
    EXPECT_EQ(sum_first_n({1, 2, 3, 4, 5}, 3), 6);   // 1+2+3
    EXPECT_EQ(sum_first_n({10, 20}, 0), 0);
    EXPECT_EQ(sum_first_n({5}, 1), 5);
}

TEST(Buggy, Repeat) {
    EXPECT_EQ(repeat("ab", 3), "ababab");
    EXPECT_EQ(repeat("x", 1), "x");
    EXPECT_EQ(repeat("y", 0), "");
}

TEST(Buggy, MaxIn) {
    EXPECT_EQ(max_in({3, 7, 1}), 7);
    EXPECT_EQ(max_in({-5, -2, -9}), -2);  // все отрицательные
}

TEST(Buggy, HasDuplicate) {
    EXPECT_FALSE(has_duplicate({1, 2, 3}));
    EXPECT_TRUE(has_duplicate({1, 2, 2}));
    EXPECT_FALSE(has_duplicate({}));
}

TEST(Buggy, Average) {
    EXPECT_DOUBLE_EQ(average({1, 2, 3, 4}), 2.5);
    EXPECT_DOUBLE_EQ(average({2, 4}), 3.0);
    EXPECT_DOUBLE_EQ(average({5}), 5.0);
}
