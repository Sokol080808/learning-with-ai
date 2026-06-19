#include <gtest/gtest.h>
#include "warmup.hpp"

// Эти тесты трогать не нужно — это эталон поведения, к которому ты приводишь код.

TEST(Warmup, AddSumsNumbers) {
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(-4, 4), 0);
    EXPECT_EQ(add(10, 25), 35);
}

TEST(Warmup, SecondsInHours) {
    EXPECT_EQ(seconds_in(1), 3600);
    EXPECT_EQ(seconds_in(2), 7200);
    EXPECT_EQ(seconds_in(0), 0);
}

// Задание 2: abs_diff объявлена в .hpp, определена в .cpp.
// Если определение не написать — будет ошибка ЛИНКОВКИ, и тест не соберётся.
TEST(Warmup, AbsDiffIsAlwaysNonNegative) {
    EXPECT_EQ(abs_diff(7, 3), 4);
    EXPECT_EQ(abs_diff(3, 7), 4);   // порядок не важен — это модуль
    EXPECT_EQ(abs_diff(5, 5), 0);
    EXPECT_EQ(abs_diff(-2, 3), 5);
    EXPECT_EQ(abs_diff(-10, -4), 6);
}

// Задание 3: is_even — inline-функция целиком в заголовке.
TEST(Warmup, IsEvenSplitsParity) {
    EXPECT_TRUE(is_even(0));
    EXPECT_TRUE(is_even(2));
    EXPECT_TRUE(is_even(-4));
    EXPECT_TRUE(is_even(100));
    EXPECT_FALSE(is_even(1));
    EXPECT_FALSE(is_even(3));
    EXPECT_FALSE(is_even(-7));
}
