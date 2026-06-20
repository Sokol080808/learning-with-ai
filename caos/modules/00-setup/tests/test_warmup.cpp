// tests/test_warmup.cpp
// Эти тесты трогать не нужно — это эталон поведения. Твоя задача — сделать так,
// чтобы они стали зелёными, правя только src/warmup.c.

#include <gtest/gtest.h>
#include <random>
#include <cstdint>

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

// ---- Randomized / property-based tests ----

// add: oracle = plain C++ addition on the same int values.
// Properties tested:
//   1. add(a, b) == a + b  (oracle)
//   2. add(a, b) == add(b, a)  (commutativity)
//   3. add(a, 0) == a  (identity)
//   4. add(a, -a) == 0  (inverse), with INT_MIN guard
TEST(WarmupRandom, AddOracleAndCommutativity) {
    std::mt19937 rng(0xC0FFEE);
    // Uniform over [-1e6, 1e6] to avoid signed overflow in the oracle itself.
    std::uniform_int_distribution<int> dist(-1000000, 1000000);

    for (int i = 0; i < 1000; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        int expected = a + b;  // oracle: plain addition
        EXPECT_EQ(add(a, b), expected)
            << "add(" << a << ", " << b << ") should be " << expected;
        // Commutativity
        EXPECT_EQ(add(a, b), add(b, a))
            << "add must be commutative for a=" << a << " b=" << b;
    }
}

TEST(WarmupRandom, AddIdentityAndInverse) {
    std::mt19937 rng(0xDEAD);
    std::uniform_int_distribution<int> dist(-500000, 500000);

    for (int i = 0; i < 500; ++i) {
        int a = dist(rng);
        // Identity: add(a, 0) == a
        EXPECT_EQ(add(a, 0), a)
            << "add(" << a << ", 0) should equal " << a;
        EXPECT_EQ(add(0, a), a)
            << "add(0, " << a << ") should equal " << a;
        // Inverse: add(a, -a) == 0
        EXPECT_EQ(add(a, -a), 0)
            << "add(" << a << ", " << -a << ") should be 0";
    }
}

// Edge cases for add: boundary int-range values (no overflow — we just check
// individually-known correct results rather than risk UB).
TEST(WarmupEdge, AddBoundary) {
    EXPECT_EQ(add(0, 0), 0);
    EXPECT_EQ(add(1, -1), 0);
    EXPECT_EQ(add(-1, 1), 0);
    EXPECT_EQ(add(100000, -100000), 0);
    EXPECT_EQ(add(-1000000, -1000000), -2000000);
    EXPECT_EQ(add(1000000, 1000000), 2000000);
}

// seconds_in: oracle = (long)hours * 3600L
// Properties:
//   1. seconds_in(h) == h * 3600L  (oracle)
//   2. seconds_in(h+1) - seconds_in(h) == 3600  (uniform step)
//   3. seconds_in(0) == 0  (edge)
//   4. seconds_in(1) == 3600  (unit)
TEST(WarmupRandom, SecondsInOracle) {
    std::mt19937 rng(0xBEEF);
    // Use [0, 1000000] — well within long range (1e6 * 3600 = 3.6e9 < LONG_MAX).
    std::uniform_int_distribution<int> dist(0, 1000000);

    for (int i = 0; i < 1000; ++i) {
        int h = dist(rng);
        long expected = static_cast<long>(h) * 3600L;  // oracle
        EXPECT_EQ(seconds_in(h), expected)
            << "seconds_in(" << h << ") should be " << expected;
    }
}

TEST(WarmupRandom, SecondsInUniformStep) {
    std::mt19937 rng(0xFACE);
    std::uniform_int_distribution<int> dist(0, 999999);

    for (int i = 0; i < 500; ++i) {
        int h = dist(rng);
        long diff = seconds_in(h + 1) - seconds_in(h);
        EXPECT_EQ(diff, 3600L)
            << "seconds_in(" << h+1 << ") - seconds_in(" << h
            << ") should be 3600, got " << diff;
    }
}

TEST(WarmupEdge, SecondsInBoundary) {
    EXPECT_EQ(seconds_in(0), 0L);
    EXPECT_EQ(seconds_in(1), 3600L);
    EXPECT_EQ(seconds_in(24), 86400L);
    EXPECT_EQ(seconds_in(365 * 24), static_cast<long>(365 * 24) * 3600L);  // one year
}
