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

// ============================================================
// float_pack: собрать float из полей (обратно к разбору)
// ============================================================

// --- Известные значения: поля -> ожидаемый float ---

TEST(FloatPack, KnownValues) {
    // 1.0f = sign 0, biased exponent 127, mantissa 0
    EXPECT_EQ(float_bits(float_pack(0, 127, 0)), 0x3F800000u);
    // -1.0f = sign 1, exponent 127, mantissa 0
    EXPECT_EQ(float_bits(float_pack(1, 127, 0)), 0xBF800000u);
    // 2.0f = sign 0, exponent 128, mantissa 0
    EXPECT_EQ(float_bits(float_pack(0, 128, 0)), 0x40000000u);
    // 0.5f = sign 0, exponent 126, mantissa 0
    EXPECT_EQ(float_bits(float_pack(0, 126, 0)), 0x3F000000u);
}

TEST(FloatPack, ProducesActualFloatValues) {
    // Не только биты совпадают, но и численное значение читается верно.
    EXPECT_FLOAT_EQ(float_pack(0, 127, 0),  1.0f);
    EXPECT_FLOAT_EQ(float_pack(1, 127, 0), -1.0f);
    EXPECT_FLOAT_EQ(float_pack(0, 128, 0),  2.0f);
    EXPECT_FLOAT_EQ(float_pack(0, 126, 0),  0.5f);
}

// --- Краевые: ±0, ±Inf, NaN, subnormal ---

TEST(FloatPack, PositiveZero) {
    EXPECT_EQ(float_bits(float_pack(0, 0, 0)), 0x00000000u);
}

TEST(FloatPack, NegativeZero) {
    // sign=1, всё остальное 0 — это -0.0f, бит 31 поднят
    EXPECT_EQ(float_bits(float_pack(1, 0, 0)), 0x80000000u);
}

TEST(FloatPack, PositiveInfinity) {
    // exponent=255, mantissa=0 => +Inf
    float f = float_pack(0, 255, 0);
    EXPECT_EQ(float_bits(f), 0x7F800000u);
    EXPECT_TRUE(std::isinf(f));
    EXPECT_GT(f, 0.0f);
}

TEST(FloatPack, NegativeInfinity) {
    float f = float_pack(1, 255, 0);
    EXPECT_EQ(float_bits(f), 0xFF800000u);
    EXPECT_TRUE(std::isinf(f));
    EXPECT_LT(f, 0.0f);
}

TEST(FloatPack, NaN) {
    // exponent=255, mantissa!=0 => NaN
    float f = float_pack(0, 255, 0x400000u);  // quiet NaN
    EXPECT_EQ(my_isnan(f), 1);
    EXPECT_TRUE(std::isnan(f));
}

TEST(FloatPack, Subnormal) {
    // exponent=0, mantissa!=0 => субнормальное число
    float f = float_pack(0, 0, 1u);  // наименьшее положительное субнормальное
    EXPECT_EQ(float_bits(f), 0x00000001u);
    EXPECT_EQ(float_raw_exponent(f), 0);
    EXPECT_GT(f, 0.0f);            // оно положительно и не ноль
    EXPECT_LT(f, FLT_MIN);        // но меньше наименьшего НОРМАЛЬНОГО
}

// --- Маскирование: лишние биты в аргументах не текут в чужие поля ---

TEST(FloatPack, OnlyLowSignBitUsed) {
    // sign=2 (бит 1, не бит 0) => знаковый бит должен остаться 0
    EXPECT_EQ(float_bits(float_pack(2, 127, 0)), 0x3F800000u);
    // sign=3 (биты 0 и 1) => младший бит 1 => знак выставлен
    EXPECT_EQ(float_bits(float_pack(3, 127, 0)), 0xBF800000u);
}

TEST(FloatPack, ExponentMaskedToEightBits) {
    // 0x1FF = 511, но в поле уходят только младшие 8 бит => 0xFF = 255
    float f = float_pack(0, 0x1FF, 0);
    EXPECT_EQ(float_raw_exponent(f), 255);
}

TEST(FloatPack, MantissaMaskedTo23Bits) {
    // Биты мантиссы сверх 23 не должны залезть в поле экспоненты.
    float f = float_pack(0, 100, 0xFFFFFFFFu);
    EXPECT_EQ(float_raw_exponent(f), 100);            // экспонента не испорчена
    EXPECT_EQ(float_bits(f) & 0x7FFFFFu, 0x7FFFFFu);  // мантисса = младшие 23 бита
}

// --- Round-trip: разобрать любой float и собрать обратно ---
//
// Свойство: float_pack(sign, exp, bits) восстанавливает исходные биты
// для ЛЮБОГО шаблона. Берём sign/exp из аккуратных функций, а полные биты
// как мантиссу — маски внутри float_pack обрежут лишнее по полям.

TEST(FloatPackRandom, RoundTripAllPatterns) {
    std::mt19937 rng(0x50FA1234u);  // фиксированный seed => детерминизм
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 2000; ++i) {
        uint32_t u = dist(rng);
        float f = from_bits(u);
        int sign = float_sign(f);
        int exp  = float_raw_exponent(f);
        uint32_t mant = float_bits(f) & 0x7FFFFFu;
        // Собираем заново из «честно разобранных» полей — должны получить те же биты.
        float g = float_pack(sign, exp, mant);
        EXPECT_EQ(float_bits(g), u) << "bits = 0x" << std::hex << u;
    }
}

TEST(FloatPackRandom, RoundTripFiniteValues) {
    // Для конечных значений собранный float побитово равен исходному,
    // а значит и численно равен (NaN исключаем — у него != сам себе).
    std::mt19937 rng(0xA5A5F00Du);
    std::uniform_real_distribution<float> dist(-1e30f, 1e30f);

    for (int i = 0; i < 1000; ++i) {
        float f = dist(rng);
        int sign = float_sign(f);
        int exp  = float_raw_exponent(f);
        uint32_t mant = float_bits(f) & 0x7FFFFFu;
        float g = float_pack(sign, exp, mant);
        EXPECT_EQ(float_bits(g), float_bits(f)) << "f = " << f;
        EXPECT_FLOAT_EQ(g, f) << "f = " << f;
    }
}

// ============================================================
// round_half_to_even: округление к чётному (режим IEEE по умолчанию)
// ============================================================

TEST(RoundHalfToEven, ExactHalvesGoToEven) {
    // Классические половинки: тай уходит к ЧЁТНОМУ соседу
    EXPECT_EQ(round_half_to_even(0.5),  0.0);
    EXPECT_EQ(round_half_to_even(1.5),  2.0);
    EXPECT_EQ(round_half_to_even(2.5),  2.0);
    EXPECT_EQ(round_half_to_even(3.5),  4.0);
    EXPECT_EQ(round_half_to_even(4.5),  4.0);
    EXPECT_EQ(round_half_to_even(-0.5), 0.0);   // значение −0.0 == 0.0
    EXPECT_EQ(round_half_to_even(-1.5), -2.0);
    EXPECT_EQ(round_half_to_even(-2.5), -2.0);
    EXPECT_EQ(round_half_to_even(-3.5), -4.0);
}

TEST(RoundHalfToEven, NonHalvesRoundToNearest) {
    EXPECT_EQ(round_half_to_even(0.4),  0.0);
    EXPECT_EQ(round_half_to_even(0.6),  1.0);
    EXPECT_EQ(round_half_to_even(2.3),  2.0);
    EXPECT_EQ(round_half_to_even(2.7),  3.0);
    EXPECT_EQ(round_half_to_even(-0.4), 0.0);
    EXPECT_EQ(round_half_to_even(-0.6), -1.0);
    EXPECT_EQ(round_half_to_even(-2.7), -3.0);
}

TEST(RoundHalfToEven, AlreadyInteger) {
    EXPECT_EQ(round_half_to_even(0.0),   0.0);
    EXPECT_EQ(round_half_to_even(5.0),   5.0);
    EXPECT_EQ(round_half_to_even(-5.0), -5.0);
    EXPECT_EQ(round_half_to_even(100.0), 100.0);
}

TEST(RoundHalfToEven, NegativeHalfKeepsSignedZero) {
    // -0.5 округляется к 0, но это должен быть ЗНАКОВЫЙ ноль -0.0
    double r = round_half_to_even(-0.5);
    EXPECT_EQ(r, 0.0);                 // численно ноль
    EXPECT_TRUE(std::signbit(r));      // но знаковый бит поднят => -0.0
}

TEST(RoundHalfToEven, SpecialValuesPassThrough) {
    EXPECT_TRUE(std::isnan(round_half_to_even(std::nan(""))));
    EXPECT_TRUE(std::isinf(round_half_to_even(INFINITY)));
    EXPECT_GT(round_half_to_even(INFINITY), 0.0);
    EXPECT_LT(round_half_to_even(-INFINITY), 0.0);
    // ±0 проходят насквозь с сохранением знака
    EXPECT_FALSE(std::signbit(round_half_to_even(0.0)));
    EXPECT_TRUE(std::signbit(round_half_to_even(-0.0)));
}

TEST(RoundHalfToEven, LargeIntegersUnchanged) {
    // Большие числа без дробной части возвращаются как есть
    EXPECT_EQ(round_half_to_even(1e15),  1e15);
    EXPECT_EQ(round_half_to_even(-1e15), -1e15);
}

// Оракул, не вызывающий round_half_to_even: округление к чётному «вручную».
static double ref_round_even(double x) {
    if (std::isnan(x) || std::isinf(x) || x == 0.0) return x;
    double lo = std::floor(x);
    double diff = x - lo;
    double res;
    if (diff < 0.5)      res = lo;
    else if (diff > 0.5) res = lo + 1.0;
    else                 res = (std::fmod(lo, 2.0) == 0.0) ? lo : lo + 1.0;
    return res;
}

TEST(RoundHalfToEvenRandom, AgainstOracle) {
    std::mt19937 rng(0x0DDE7E11u);  // fixed seed => deterministic
    // Диапазон с дробями; значения вида k и k.5 встречаются естественно редко,
    // но обычные дроби покрывают «к ближайшему» обильно.
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);

    for (int i = 0; i < 5000; ++i) {
        double x = dist(rng);
        EXPECT_EQ(round_half_to_even(x), ref_round_even(x))
            << "x = " << x;
    }
}

TEST(RoundHalfToEvenRandom, ExactHalvesAgainstOracle) {
    // Прицельно по половинкам k + 0.5 (они точно представимы в double):
    // оба соседа целые, тай-брейк к чётному должен совпасть с оракулом.
    for (int k = -50; k <= 50; ++k) {
        double x = static_cast<double>(k) + 0.5;
        double got = round_half_to_even(x);
        double want = ref_round_even(x);
        EXPECT_EQ(got, want) << "x = " << x;
        // И прямая проверка чётности результата.
        EXPECT_EQ(std::fmod(got, 2.0), 0.0) << "x = " << x << " got = " << got;
    }
}
