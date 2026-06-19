// tests/test_warmup.cpp
// Эти тесты трогать не нужно — это эталон поведения. Твоя задача — сделать так,
// чтобы они стали зелёными, правя только src/warmup.c.

#include <gtest/gtest.h>

extern "C" {
#include "warmup.h"
}

TEST(Warmup, AddBasic) {
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(10, 7), 17);
}

TEST(Warmup, AddZeroAndNegative) {
    EXPECT_EQ(add(0, 0), 0);
    EXPECT_EQ(add(-4, 4), 0);
    EXPECT_EQ(add(-5, -8), -13);
}

TEST(Warmup, SecondsInOneHour) {
    EXPECT_EQ(seconds_in(1), 3600L);
}

TEST(Warmup, SecondsInZeroHours) {
    EXPECT_EQ(seconds_in(0), 0L);
}

TEST(Warmup, SecondsInManyHours) {
    EXPECT_EQ(seconds_in(2), 7200L);
    EXPECT_EQ(seconds_in(24), 86400L);
    // День большой — но в long это влезает без проблем.
    EXPECT_EQ(seconds_in(1000), 3600000L);
}
