// Эти тесты трогать не нужно — это эталон поведения.
//
// Тесты проверяют, что ты правильно «разбираешь» float по стандарту IEEE 754
// (одинарная точность, 32 бита): знак, поле экспоненты, распознавание NaN.

#include <gtest/gtest.h>
#include <cstring>   // memcpy — для построения NaN из битов в самом тесте
#include <cstdint>
#include <random>
#include <cmath>
#include <climits>
#include <cfloat>

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

// ============================================================
// Randomized / property tests
// ============================================================

// Helper: read raw bits from a float without UB
static uint32_t raw_bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}

// Helper: build a float from raw uint32_t bits
static float from_bits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

// ---- float_bits round-trip: from known bits -> float -> float_bits ------

TEST(FloatBitsRandom, RoundTripAllPatterns) {
    // float_bits(from_bits(u)) must equal u for any bit pattern
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t u = dist(rng);
        float f = from_bits(u);
        EXPECT_EQ(float_bits(f), u)
            << "bits = 0x" << std::hex << u;
    }
}

TEST(FloatBitsRandom, KnownNormals) {
    // For ordinary finite floats constructed from known values,
    // float_bits must match the memcpy oracle.
    std::mt19937 rng(0xBEEF1234u);
    std::uniform_real_distribution<float> dist(-1e30f, 1e30f);

    for (int i = 0; i < 1000; ++i) {
        float f = dist(rng);
        EXPECT_EQ(float_bits(f), raw_bits(f));
    }
}

// Edge: negative zero has different bits from positive zero
TEST(FloatBitsRandom, NegativeZeroBits) {
    uint32_t neg_zero_bits = 0x80000000u;
    float neg_zero = from_bits(neg_zero_bits);
    EXPECT_EQ(float_bits(neg_zero), 0x80000000u);
}

TEST(FloatBitsRandom, FltMaxAndMin) {
    EXPECT_EQ(float_bits(FLT_MAX),  raw_bits(FLT_MAX));
    EXPECT_EQ(float_bits(-FLT_MAX), raw_bits(-FLT_MAX));
    EXPECT_EQ(float_bits(FLT_MIN),  raw_bits(FLT_MIN));
}

// ---- float_sign: oracle = bit 31 of raw representation ----

TEST(FloatSignRandom, AgainstBitOracle) {
    std::mt19937 rng(0xDEADC0DEu);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t u = dist(rng);
        float f = from_bits(u);
        int expected = static_cast<int>((u >> 31) & 1u);
        EXPECT_EQ(float_sign(f), expected)
            << "bits = 0x" << std::hex << u;
    }
}

TEST(FloatSignRandom, PositiveNormals) {
    std::mt19937 rng(0xABCD1234u);
    std::uniform_real_distribution<float> dist(1e-30f, 1e30f);

    for (int i = 0; i < 500; ++i) {
        float f = dist(rng);
        EXPECT_EQ(float_sign(f), 0) << "f = " << f;
    }
}

TEST(FloatSignRandom, NegativeNormals) {
    std::mt19937 rng(0x12345678u);
    std::uniform_real_distribution<float> dist(1e-30f, 1e30f);

    for (int i = 0; i < 500; ++i) {
        float f = -dist(rng);  // force negative
        EXPECT_EQ(float_sign(f), 1) << "f = " << f;
    }
}

TEST(FloatSignRandom, NegativeZeroSignIsOne) {
    // -0.0f has sign bit = 1
    float neg_zero = from_bits(0x80000000u);
    EXPECT_EQ(float_sign(neg_zero), 1);
}

TEST(FloatSignRandom, PlusZeroSignIsZero) {
    EXPECT_EQ(float_sign(0.0f), 0);
}

TEST(FloatSignRandom, BothInfinities) {
    float pos_inf = from_bits(0x7F800000u);
    float neg_inf = from_bits(0xFF800000u);
    EXPECT_EQ(float_sign(pos_inf), 0);
    EXPECT_EQ(float_sign(neg_inf), 1);
}

// ---- float_raw_exponent: oracle = (raw_bits >> 23) & 0xFF ----

TEST(RawExponentRandom, AgainstBitOracle) {
    std::mt19937 rng(0xFACEFEEDu);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t u = dist(rng);
        float f = from_bits(u);
        int expected = static_cast<int>((u >> 23) & 0xFFu);
        EXPECT_EQ(float_raw_exponent(f), expected)
            << "bits = 0x" << std::hex << u;
    }
}

TEST(RawExponentRandom, SignBitDoesNotAffectExponent) {
    // Flipping sign bit must not change exponent field
    std::mt19937 rng(0x55AA55AAu);
    std::uniform_int_distribution<uint32_t> dist(0, 0x7FFFFFFFu);  // positive half

    for (int i = 0; i < 500; ++i) {
        uint32_t u_pos = dist(rng);
        uint32_t u_neg = u_pos | 0x80000000u;
        float f_pos = from_bits(u_pos);
        float f_neg = from_bits(u_neg);
        EXPECT_EQ(float_raw_exponent(f_pos), float_raw_exponent(f_neg))
            << "bits = 0x" << std::hex << u_pos;
    }
}

TEST(RawExponentRandom, SubnormalsHaveZeroExponent) {
    // Subnormals: exponent bits = 0, mantissa != 0, so raw exponent = 0
    std::mt19937 rng(0x0A0B0C0Du);
    std::uniform_int_distribution<uint32_t> mant_dist(1, 0x7FFFFFu);

    for (int i = 0; i < 200; ++i) {
        uint32_t mant = mant_dist(rng);
        // subnormal: sign=0, exponent=0, mantissa!=0
        uint32_t u = mant & 0x7FFFFFu;
        float f = from_bits(u);
        EXPECT_EQ(float_raw_exponent(f), 0)
            << "subnormal bits = 0x" << std::hex << u;
    }
}

TEST(RawExponentRandom, NaNAndInfHaveExponent255) {
    // Any bit pattern with exponent field = 0xFF must return 255
    std::mt19937 rng(0xCAFEBABEu);
    std::uniform_int_distribution<uint32_t> mant_dist(0, 0x7FFFFFu);

    for (int i = 0; i < 200; ++i) {
        uint32_t mant = mant_dist(rng);
        // exponent = 0xFF, random mantissa (both NaN and inf)
        uint32_t u = 0x7F800000u | mant;
        float f = from_bits(u);
        EXPECT_EQ(float_raw_exponent(f), 255)
            << "bits = 0x" << std::hex << u;
    }
}

TEST(RawExponentRandom, ExponentRange) {
    // For every possible biased exponent e in [0, 255], check one sample
    for (int e = 0; e <= 255; ++e) {
        uint32_t u = (static_cast<uint32_t>(e) << 23);  // sign=0, mantissa=0
        float f = from_bits(u);
        EXPECT_EQ(float_raw_exponent(f), e) << "e = " << e;
    }
}

// ---- my_isnan: oracle = exponent==255 && mantissa!=0 ----

// Reference NaN checker that does NOT call my_isnan
static int ref_isnan(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    uint32_t exp  = (u >> 23) & 0xFFu;
    uint32_t mant =  u & 0x7FFFFFu;
    return (exp == 255u && mant != 0u) ? 1 : 0;
}

TEST(IsNanRandom, AgainstBitOracle) {
    std::mt19937 rng(0xC0FFEE42u);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t u = dist(rng);
        float f = from_bits(u);
        EXPECT_EQ(my_isnan(f), ref_isnan(f))
            << "bits = 0x" << std::hex << u;
    }
}

TEST(IsNanRandom, AllNaNMantissas) {
    // Every nonzero mantissa with exponent=255 is a NaN
    // Test 500 random mantissas for both sign bits
    std::mt19937 rng(0x7EA7FABEu);
    std::uniform_int_distribution<uint32_t> mant_dist(1, 0x7FFFFFu);

    for (int i = 0; i < 500; ++i) {
        uint32_t mant = mant_dist(rng);
        uint32_t u_pos = 0x7F800000u | mant;   // positive NaN
        uint32_t u_neg = 0xFF800000u | mant;   // negative NaN
        EXPECT_EQ(my_isnan(from_bits(u_pos)), 1)
            << "positive NaN bits = 0x" << std::hex << u_pos;
        EXPECT_EQ(my_isnan(from_bits(u_neg)), 1)
            << "negative NaN bits = 0x" << std::hex << u_neg;
    }
}

TEST(IsNanRandom, NaNSignBitDoesNotMatter) {
    // Both 0x7FC00000 (positive quiet NaN) and 0xFFC00000 (negative quiet NaN)
    // must return 1
    EXPECT_EQ(my_isnan(from_bits(0x7FC00000u)), 1);
    EXPECT_EQ(my_isnan(from_bits(0xFFC00000u)), 1);
    EXPECT_EQ(my_isnan(from_bits(0x7F800001u)), 1);  // signaling NaN
    EXPECT_EQ(my_isnan(from_bits(0xFF800001u)), 1);  // negative signaling NaN
}

TEST(IsNanRandom, FiniteNumbersAreNotNaN) {
    // Random finite floats (exponent 0..254) must return 0
    std::mt19937 rng(0xF00DBEEF);
    std::uniform_real_distribution<float> dist(-1e38f, 1e38f);

    for (int i = 0; i < 500; ++i) {
        float f = dist(rng);
        // dist may produce inf for extreme values; skip those
        if (std::isinf(f)) continue;
        EXPECT_EQ(my_isnan(f), 0) << "f = " << f;
    }
}

TEST(IsNanRandom, PositiveInfinityIsNotNaN) {
    EXPECT_EQ(my_isnan(from_bits(0x7F800000u)), 0);
}

TEST(IsNanRandom, NegativeInfinityIsNotNaN) {
    EXPECT_EQ(my_isnan(from_bits(0xFF800000u)), 0);
}

TEST(IsNanRandom, SubnormalsAreNotNaN) {
    // Subnormal: exponent=0, mantissa!=0 — finite, not NaN
    std::mt19937 rng(0x1EE7C0DEu);
    std::uniform_int_distribution<uint32_t> mant_dist(1, 0x7FFFFFu);

    for (int i = 0; i < 200; ++i) {
        uint32_t u = mant_dist(rng) & 0x7FFFFFu;  // sign=0, exp=0
        EXPECT_EQ(my_isnan(from_bits(u)), 0)
            << "subnormal bits = 0x" << std::hex << u;
    }
}
