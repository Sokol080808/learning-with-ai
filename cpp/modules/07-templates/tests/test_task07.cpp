#include <gtest/gtest.h>
#include <string>
#include "task07.hpp"

TEST(Templates, MyMax) {
    EXPECT_EQ(my_max(3, 5), 5);
    EXPECT_EQ(my_max(5, 3), 5);
    EXPECT_DOUBLE_EQ(my_max(2.5, 1.5), 2.5);
    EXPECT_EQ(my_max(std::string("a"), std::string("b")), "b");
}

TEST(Templates, ClampValue) {
    EXPECT_EQ(clamp_value(5, 0, 10), 5);
    EXPECT_EQ(clamp_value(-3, 0, 10), 0);
    EXPECT_EQ(clamp_value(42, 0, 10), 10);
    EXPECT_DOUBLE_EQ(clamp_value(1.5, 0.0, 1.0), 1.0);
}

TEST(Templates, SumVector) {
    EXPECT_EQ(sum_vector(std::vector<int>{1, 2, 3, 4}), 10);
    EXPECT_DOUBLE_EQ(sum_vector(std::vector<double>{1.5, 2.5}), 4.0);
    EXPECT_EQ(sum_vector(std::vector<int>{}), 0);
}

TEST(Templates, PairSwapped) {
    Pair<int, std::string> p(1, "one");
    auto q = p.swapped();
    EXPECT_EQ(q.first, "one");
    EXPECT_EQ(q.second, 1);
}

TEST(Templates, IsPowerOfTwo) {
    EXPECT_TRUE(is_power_of_two(1));
    EXPECT_TRUE(is_power_of_two(2));
    EXPECT_TRUE(is_power_of_two(1024));
    EXPECT_FALSE(is_power_of_two(0));
    EXPECT_FALSE(is_power_of_two(3));
    EXPECT_FALSE(is_power_of_two(-4));
}
