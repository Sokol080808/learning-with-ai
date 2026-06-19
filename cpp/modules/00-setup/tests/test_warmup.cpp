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
