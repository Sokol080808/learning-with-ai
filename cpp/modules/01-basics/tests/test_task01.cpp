#include <gtest/gtest.h>
#include <limits>
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

// --- Задание 6: safe_add ---

TEST(Basics, SafeAddOrdinary) {
    auto r = safe_add(2, 3);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 5);

    auto neg = safe_add(-10, 4);
    ASSERT_TRUE(neg.has_value());
    EXPECT_EQ(*neg, -6);

    auto zero = safe_add(0, 0);
    ASSERT_TRUE(zero.has_value());
    EXPECT_EQ(*zero, 0);
}

TEST(Basics, SafeAddBoundariesOk) {
    const int kMax = std::numeric_limits<int>::max();
    const int kMin = std::numeric_limits<int>::min();

    // Ровно по краю — переполнения ещё нет.
    auto hi = safe_add(kMax - 1, 1);
    ASSERT_TRUE(hi.has_value());
    EXPECT_EQ(*hi, kMax);

    auto lo = safe_add(kMin + 1, -1);
    ASSERT_TRUE(lo.has_value());
    EXPECT_EQ(*lo, kMin);

    auto mix = safe_add(kMax, kMin);  // = -1, переполнения нет
    ASSERT_TRUE(mix.has_value());
    EXPECT_EQ(*mix, -1);
}

TEST(Basics, SafeAddOverflowReturnsNullopt) {
    const int kMax = std::numeric_limits<int>::max();
    const int kMin = std::numeric_limits<int>::min();

    EXPECT_FALSE(safe_add(kMax, 1).has_value());       // переполнение вверх
    EXPECT_FALSE(safe_add(kMax, kMax).has_value());
    EXPECT_FALSE(safe_add(kMin, -1).has_value());      // переполнение вниз
    EXPECT_FALSE(safe_add(kMin, kMin).has_value());
}

// --- Задание 7: битовые операции ---

TEST(Basics, SetBit) {
    EXPECT_EQ(set_bit(0u, 0), 1u);
    EXPECT_EQ(set_bit(0u, 3), 8u);
    EXPECT_EQ(set_bit(0u, 31), 0x80000000u);
    // Установка уже установленного бита ничего не меняет.
    EXPECT_EQ(set_bit(0b1010u, 1), 0b1010u);
    // Остальные биты не трогаем.
    EXPECT_EQ(set_bit(0b0001u, 2), 0b0101u);
}

TEST(Basics, ClearBit) {
    EXPECT_EQ(clear_bit(0b1111u, 0), 0b1110u);
    EXPECT_EQ(clear_bit(0b1111u, 3), 0b0111u);
    EXPECT_EQ(clear_bit(0xFFFFFFFFu, 31), 0x7FFFFFFFu);
    // Сброс уже сброшенного бита ничего не меняет.
    EXPECT_EQ(clear_bit(0b0010u, 0), 0b0010u);
}

TEST(Basics, ToggleBit) {
    EXPECT_EQ(toggle_bit(0u, 0), 1u);
    EXPECT_EQ(toggle_bit(1u, 0), 0u);
    EXPECT_EQ(toggle_bit(0b1010u, 0), 0b1011u);
    EXPECT_EQ(toggle_bit(0b1010u, 1), 0b1000u);
    // Два переключения подряд возвращают исходное.
    EXPECT_EQ(toggle_bit(toggle_bit(0xABCDu, 5), 5), 0xABCDu);
}

TEST(Basics, GetBit) {
    EXPECT_TRUE(get_bit(0b1000u, 3));
    EXPECT_FALSE(get_bit(0b1000u, 2));
    EXPECT_TRUE(get_bit(1u, 0));
    EXPECT_FALSE(get_bit(0u, 0));
    EXPECT_TRUE(get_bit(0x80000000u, 31));
    EXPECT_FALSE(get_bit(0x7FFFFFFFu, 31));
}

// --- Задание 8: swap_ints ---

TEST(Basics, SwapInts) {
    int a = 1, b = 2;
    swap_ints(a, b);
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 1);

    int x = -5, y = 100;
    swap_ints(x, y);
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, -5);
}

TEST(Basics, SwapIntsSameValue) {
    int a = 42, b = 42;
    swap_ints(a, b);
    EXPECT_EQ(a, 42);
    EXPECT_EQ(b, 42);
}

TEST(Basics, SwapIntsSameVariableStaysIntact) {
    // Самоприсваивание-через-swap не должно обнулять значение
    // (ловит наивную реализацию через XOR при a и b — одна переменная).
    int v = 7;
    swap_ints(v, v);
    EXPECT_EQ(v, 7);
}
