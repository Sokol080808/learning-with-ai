// Эти тесты трогать не нужно — это эталон поведения.
//
// Тесты проверяют, что ты правильно «разбираешь» float по стандарту IEEE 754
// (одинарная точность, 32 бита): знак, поле экспоненты, распознавание NaN.

#include <gtest/gtest.h>
#include <cstring>   // memcpy — для построения NaN из битов в самом тесте
#include <cstdint>

extern "C" {
#include "floats.h"
}

// --- float_bits: сырые биты числа ---

TEST(FloatBits, One) {
    // 1.0f в IEEE 754 = 0x3F800000
    EXPECT_EQ(float_bits(1.0f), 0x3F800000u);
}

TEST(FloatBits, MinusOne) {
    // -1.0f отличается от 1.0f только знаковым (старшим) битом
    EXPECT_EQ(float_bits(-1.0f), 0xBF800000u);
}

TEST(FloatBits, Two) {
    // 2.0f = 0x40000000
    EXPECT_EQ(float_bits(2.0f), 0x40000000u);
}

TEST(FloatBits, Zero) {
    // +0.0f — все биты нулевые
    EXPECT_EQ(float_bits(0.0f), 0x00000000u);
}

// --- float_sign: знаковый бит ---

TEST(FloatSign, PositiveIsZero) {
    EXPECT_EQ(float_sign(1.0f), 0);
    EXPECT_EQ(float_sign(2.0f), 0);
    EXPECT_EQ(float_sign(0.0f), 0);
}

TEST(FloatSign, NegativeIsOne) {
    EXPECT_EQ(float_sign(-1.0f), 1);
    EXPECT_EQ(float_sign(-42.5f), 1);
}

// --- float_raw_exponent: поле экспоненты 0..255 ---

TEST(RawExponent, OneIs127) {
    // У 1.0f смещённая экспонента = 127 (0x7F)
    EXPECT_EQ(float_raw_exponent(1.0f), 127);
}

TEST(RawExponent, TwoIs128) {
    // У 2.0f экспонента на единицу больше: 128
    EXPECT_EQ(float_raw_exponent(2.0f), 128);
}

TEST(RawExponent, HalfIs126) {
    // У 0.5f экспонента на единицу меньше, чем у 1.0f: 126
    EXPECT_EQ(float_raw_exponent(0.5f), 126);
}

TEST(RawExponent, ZeroIsZero) {
    // У нуля поле экспоненты целиком нулевое
    EXPECT_EQ(float_raw_exponent(0.0f), 0);
}

TEST(RawExponent, SignDoesNotMatter) {
    // Знак не влияет на поле экспоненты
    EXPECT_EQ(float_raw_exponent(-1.0f), 127);
}

// --- my_isnan: распознавание NaN ---

// Собираем NaN из конкретного набора битов, не полагаясь на вычисления:
// экспонента = все единицы (255), мантисса != 0 => это NaN.
static float make_nan_from_bits() {
    uint32_t bits = 0x7FC00000u;  // классический "quiet NaN"
    float f;
    std::memcpy(&f, &bits, sizeof(f));
    return f;
}

TEST(IsNan, RealNanIsNan) {
    EXPECT_EQ(my_isnan(make_nan_from_bits()), 1);
}

TEST(IsNan, OrdinaryNumbersAreNotNan) {
    EXPECT_EQ(my_isnan(0.0f), 0);
    EXPECT_EQ(my_isnan(1.0f), 0);
    EXPECT_EQ(my_isnan(-1.0f), 0);
    EXPECT_EQ(my_isnan(42.5f), 0);
}

TEST(IsNan, InfinityIsNotNan) {
    // +бесконечность: экспонента = 255, но мантисса == 0 => это НЕ NaN
    uint32_t inf_bits = 0x7F800000u;
    float inf;
    std::memcpy(&inf, &inf_bits, sizeof(inf));
    EXPECT_EQ(my_isnan(inf), 0);
}
