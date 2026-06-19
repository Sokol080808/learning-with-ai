// Эти тесты трогать не нужно — это эталон поведения.
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>

extern "C" {
#include "integers.h"
}

// --- add_overflows ----------------------------------------------------------

TEST(AddOverflows, SimpleNoOverflow) {
    EXPECT_EQ(add_overflows(1, 2), 0);
    EXPECT_EQ(add_overflows(-5, 3), 0);
    EXPECT_EQ(add_overflows(0, 0), 0);
}

TEST(AddOverflows, PositivePlusPositiveOverflows) {
    // INT32_MAX + 1 не помещается в int32_t.
    EXPECT_EQ(add_overflows(INT32_MAX, 1), 1);
    EXPECT_EQ(add_overflows(INT32_MAX, INT32_MAX), 1);
}

TEST(AddOverflows, NegativePlusNegativeOverflows) {
    // INT32_MIN + (-1) уходит ниже минимума.
    EXPECT_EQ(add_overflows(INT32_MIN, -1), 1);
    EXPECT_EQ(add_overflows(INT32_MIN, INT32_MIN), 1);
}

TEST(AddOverflows, EdgesThatJustFit) {
    // Ровно на границе — переполнения ещё нет.
    EXPECT_EQ(add_overflows(INT32_MAX, 0), 0);
    EXPECT_EQ(add_overflows(INT32_MIN, 0), 0);
    EXPECT_EQ(add_overflows(INT32_MAX, INT32_MIN), 0);  // == -1
    EXPECT_EQ(add_overflows(INT32_MAX - 1, 1), 0);      // == INT32_MAX
}

// --- swap16 -----------------------------------------------------------------

TEST(Swap16, Basic) {
    EXPECT_EQ(swap16(0xAABB), 0xBBAA);
    EXPECT_EQ(swap16(0x0102), 0x0201);
}

TEST(Swap16, Edges) {
    EXPECT_EQ(swap16(0x0000), 0x0000);
    EXPECT_EQ(swap16(0xFFFF), 0xFFFF);
    EXPECT_EQ(swap16(0x00FF), 0xFF00);
    EXPECT_EQ(swap16(0xFF00), 0x00FF);
}

TEST(Swap16, TwiceIsIdentity) {
    EXPECT_EQ(swap16(swap16(0x1234)), 0x1234);
}

// --- swap32 -----------------------------------------------------------------

TEST(Swap32, Basic) {
    EXPECT_EQ(swap32(0xAABBCCDDu), 0xDDCCBBAAu);
    EXPECT_EQ(swap32(0x01020304u), 0x04030201u);
}

TEST(Swap32, Edges) {
    EXPECT_EQ(swap32(0x00000000u), 0x00000000u);
    EXPECT_EQ(swap32(0xFFFFFFFFu), 0xFFFFFFFFu);
    EXPECT_EQ(swap32(0x000000FFu), 0xFF000000u);
    EXPECT_EQ(swap32(0x12345678u), 0x78563412u);
}

TEST(Swap32, TwiceIsIdentity) {
    EXPECT_EQ(swap32(swap32(0xDEADBEEFu)), 0xDEADBEEFu);
}

// --- sign_extend ------------------------------------------------------------

TEST(SignExtend, EightBits) {
    // 0x7F (127) — старший бит 8-битного числа равен 0, остаётся положительным.
    EXPECT_EQ(sign_extend(0x7F, 8), 127);
    // 0x80 (128) — старший бит 8-битного числа равен 1, значит это -128.
    EXPECT_EQ(sign_extend(0x80, 8), -128);
    // 0xFF — все восемь бит единицы, 8-битное значение -1.
    EXPECT_EQ(sign_extend(0xFF, 8), -1);
}

TEST(SignExtend, IgnoresHigherBits) {
    // Биты выше bits не участвуют: 0xFF80 при bits=8 — это всё те же -128.
    EXPECT_EQ(sign_extend(0xFF80u, 8), -128);
    EXPECT_EQ(sign_extend(0xABCDu, 8), sign_extend(0xCDu, 8));
}

TEST(SignExtend, FourBits) {
    EXPECT_EQ(sign_extend(0x7, 4), 7);    // 0111 -> +7
    EXPECT_EQ(sign_extend(0x8, 4), -8);   // 1000 -> -8
    EXPECT_EQ(sign_extend(0xF, 4), -1);   // 1111 -> -1
}

TEST(SignExtend, TwelveBits) {
    EXPECT_EQ(sign_extend(0x7FFu, 12), 2047);   // максимум положительного
    EXPECT_EQ(sign_extend(0x800u, 12), -2048);  // минимум отрицательного
    EXPECT_EQ(sign_extend(0xFFFu, 12), -1);
}

TEST(SignExtend, OneBit) {
    EXPECT_EQ(sign_extend(0x0u, 1), 0);
    EXPECT_EQ(sign_extend(0x1u, 1), -1);  // единственный бит — знаковый
}

TEST(SignExtend, FullWidth) {
    // bits == 32: ничего не меняем, просто трактуем как int32_t.
    EXPECT_EQ(sign_extend(0x00000001u, 32), 1);
    EXPECT_EQ(sign_extend(0xFFFFFFFFu, 32), -1);
    EXPECT_EQ(sign_extend(0x80000000u, 32), INT32_MIN);
}

// --- is_little_endian -------------------------------------------------------

TEST(IsLittleEndian, MatchesMemoryLayout) {
    // Эталон считаем независимо: кладём 32-битное число и смотрим первый байт.
    uint32_t probe = 0x01020304u;
    unsigned char first_byte;
    std::memcpy(&first_byte, &probe, 1);
    int expected = (first_byte == 0x04) ? 1 : 0;
    EXPECT_EQ(is_little_endian(), expected);
}

TEST(IsLittleEndian, ReturnsBoolean) {
    int r = is_little_endian();
    EXPECT_TRUE(r == 0 || r == 1);
}
