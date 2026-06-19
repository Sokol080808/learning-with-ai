#include <gtest/gtest.h>
#include "task01.hpp"

TEST(Basics, ToFahrenheit) {
    EXPECT_DOUBLE_EQ(to_fahrenheit(0.0), 32.0);
    EXPECT_DOUBLE_EQ(to_fahrenheit(100.0), 212.0);
    EXPECT_DOUBLE_EQ(to_fahrenheit(37.0), 98.6);
    EXPECT_DOUBLE_EQ(to_fahrenheit(-40.0), -40.0);
}

TEST(Basics, IsEven) {
    EXPECT_TRUE(is_even(0));
    EXPECT_TRUE(is_even(4));
    EXPECT_TRUE(is_even(-2));
    EXPECT_FALSE(is_even(1));
    EXPECT_FALSE(is_even(-3));
}

TEST(Basics, MinOfThree) {
    EXPECT_EQ(min_of_three(1, 2, 3), 1);
    EXPECT_EQ(min_of_three(3, 2, 1), 1);
    EXPECT_EQ(min_of_three(5, 5, 5), 5);
    EXPECT_EQ(min_of_three(-1, 0, 1), -1);
}

TEST(Basics, TripleByReference) {
    int x = 7;
    triple(x);
    EXPECT_EQ(x, 21);
    int y = -3;
    triple(y);
    EXPECT_EQ(y, -9);
}

TEST(Basics, Average3) {
    EXPECT_DOUBLE_EQ(average3(2, 4, 6), 4.0);
    EXPECT_DOUBLE_EQ(average3(1, 2, 2), 5.0 / 3.0);  // НЕ должно быть 1.0
    EXPECT_DOUBLE_EQ(average3(0, 0, 1), 1.0 / 3.0);
}
