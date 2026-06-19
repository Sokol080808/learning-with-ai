#include <gtest/gtest.h>
#include "buggy.hpp"

#include <climits>     // INT_MAX
#include <stdexcept>   // std::out_of_range

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

// ===== Новые задания (посложнее) =====

// Задание 6. checked_at: корректный доступ в пределах границ.
TEST(CheckedAt, ReturnsElementInRange) {
    std::vector<int> v{10, 20, 30};
    EXPECT_EQ(checked_at(v, 0), 10);
    EXPECT_EQ(checked_at(v, 1), 20);
    EXPECT_EQ(checked_at(v, 2), 30);
}

// Задание 6. checked_at: выход за границу — именно std::out_of_range (НЕ logic_error).
TEST(CheckedAt, ThrowsOutOfRange) {
    std::vector<int> v{1, 2, 3};
    EXPECT_THROW(checked_at(v, 3), std::out_of_range);   // i == size
    EXPECT_THROW(checked_at(v, 100), std::out_of_range); // далеко за границей
}

// Задание 6. checked_at: на пустом векторе любой индекс вне границ.
TEST(CheckedAt, EmptyVectorAlwaysThrows) {
    std::vector<int> empty;
    EXPECT_THROW(checked_at(empty, 0), std::out_of_range);
}

// Задание 7. int_sqrt: точные квадраты.
TEST(IntSqrt, PerfectSquares) {
    EXPECT_EQ(int_sqrt(0), 0);
    EXPECT_EQ(int_sqrt(1), 1);
    EXPECT_EQ(int_sqrt(4), 2);
    EXPECT_EQ(int_sqrt(9), 3);
    EXPECT_EQ(int_sqrt(16), 4);
    EXPECT_EQ(int_sqrt(144), 12);
}

// Задание 7. int_sqrt: округление вниз для неполных квадратов.
TEST(IntSqrt, FloorsNonSquares) {
    EXPECT_EQ(int_sqrt(2), 1);
    EXPECT_EQ(int_sqrt(3), 1);
    EXPECT_EQ(int_sqrt(8), 2);
    EXPECT_EQ(int_sqrt(15), 3);
    EXPECT_EQ(int_sqrt(99), 9);
    EXPECT_EQ(int_sqrt(120), 10);  // 10*10=100 <= 120 < 121=11*11
}

// Задание 7. int_sqrt: большие значения у границы int (проверяет отсутствие переполнения r*r).
TEST(IntSqrt, LargeNoOverflow) {
    EXPECT_EQ(int_sqrt(2147395600), 46340);          // 46340^2 = 2147395600
    EXPECT_EQ(int_sqrt(INT_MAX), 46340);             // 46341^2 переполнил бы int
    EXPECT_EQ(int_sqrt(1000000), 1000);
}

// Задание 8. midpoint_int: обычные случаи и округление вниз.
TEST(MidpointInt, BasicCases) {
    EXPECT_EQ(midpoint_int(2, 4), 3);
    EXPECT_EQ(midpoint_int(1, 4), 2);   // 2.5 -> вниз к 2
    EXPECT_EQ(midpoint_int(0, 0), 0);
    EXPECT_EQ(midpoint_int(3, 3), 3);
    EXPECT_EQ(midpoint_int(0, 10), 5);
}

// Задание 8. midpoint_int: отрицательные и смешанные знаки.
TEST(MidpointInt, NegativeAndMixed) {
    EXPECT_EQ(midpoint_int(-4, -2), -3);
    EXPECT_EQ(midpoint_int(-2, 2), 0);
    EXPECT_EQ(midpoint_int(-3, 0), -2);  // -1.5 -> вниз к меньшему концу: -2
}

// Задание 8. midpoint_int: у границы INT_MAX, где (a+b)/2 переполняется (UB).
TEST(MidpointInt, NoOverflowNearMax) {
    EXPECT_EQ(midpoint_int(INT_MAX - 1, INT_MAX), INT_MAX - 1);
    EXPECT_EQ(midpoint_int(INT_MAX, INT_MAX), INT_MAX);
    EXPECT_EQ(midpoint_int(2000000000, 2000000000), 2000000000);
    EXPECT_EQ(midpoint_int(1000000000, 2000000000), 1500000000);
}
