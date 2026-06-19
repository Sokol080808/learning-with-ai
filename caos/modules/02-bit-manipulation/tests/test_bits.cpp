// Эти тесты трогать не нужно — это эталон поведения.
// Они подключают C-заголовок через extern "C", чтобы C++ увидел C-функции.
#include <gtest/gtest.h>

extern "C" {
#include "bits.h"
}

// ---------- get_bit ----------
TEST(GetBit, ReadsIndividualBits) {
    // 0b1010 = 10: бит 0 = 0, бит 1 = 1, бит 2 = 0, бит 3 = 1
    EXPECT_EQ(get_bit(0b1010u, 0), 0);
    EXPECT_EQ(get_bit(0b1010u, 1), 1);
    EXPECT_EQ(get_bit(0b1010u, 2), 0);
    EXPECT_EQ(get_bit(0b1010u, 3), 1);
}

TEST(GetBit, AllZeroAndAllOne) {
    EXPECT_EQ(get_bit(0u, 0), 0);
    EXPECT_EQ(get_bit(0u, 31), 0);
    EXPECT_EQ(get_bit(0xFFFFFFFFu, 0), 1);
    EXPECT_EQ(get_bit(0xFFFFFFFFu, 31), 1);
}

TEST(GetBit, HighBit) {
    // Установлен только старший (31-й) бит.
    EXPECT_EQ(get_bit(0x80000000u, 31), 1);
    EXPECT_EQ(get_bit(0x80000000u, 30), 0);
    EXPECT_EQ(get_bit(0x80000000u, 0), 0);
}

// ---------- set_bit ----------
TEST(SetBit, SetsTargetBit) {
    EXPECT_EQ(set_bit(0u, 0), 1u);
    EXPECT_EQ(set_bit(0u, 3), 0b1000u);
    EXPECT_EQ(set_bit(0u, 31), 0x80000000u);
}

TEST(SetBit, LeavesOtherBitsAlone) {
    // Бит 1 уже установлен; ставим бит 0 — должно стать 0b11.
    EXPECT_EQ(set_bit(0b10u, 0), 0b11u);
    // Если бит уже стоит — число не меняется.
    EXPECT_EQ(set_bit(0b100u, 2), 0b100u);
}

// ---------- clear_bit ----------
TEST(ClearBit, ClearsTargetBit) {
    EXPECT_EQ(clear_bit(0b1111u, 0), 0b1110u);
    EXPECT_EQ(clear_bit(0b1111u, 3), 0b0111u);
    EXPECT_EQ(clear_bit(0xFFFFFFFFu, 31), 0x7FFFFFFFu);
}

TEST(ClearBit, LeavesOtherBitsAlone) {
    // Если бит уже сброшен — число не меняется.
    EXPECT_EQ(clear_bit(0b1011u, 2), 0b1011u);
}

// ---------- toggle_bit ----------
TEST(ToggleBit, FlipsTargetBit) {
    EXPECT_EQ(toggle_bit(0b0000u, 1), 0b0010u);  // 0 -> 1
    EXPECT_EQ(toggle_bit(0b0010u, 1), 0b0000u);  // 1 -> 0
    EXPECT_EQ(toggle_bit(0x00000000u, 31), 0x80000000u);
}

TEST(ToggleBit, TwiceRestoresOriginal) {
    uint32_t x = 0xDEADBEEFu;
    EXPECT_EQ(toggle_bit(toggle_bit(x, 7), 7), x);
}

// ---------- is_power_of_two ----------
TEST(IsPowerOfTwo, TruePowers) {
    EXPECT_EQ(is_power_of_two(1u), 1);          // 2^0
    EXPECT_EQ(is_power_of_two(2u), 1);          // 2^1
    EXPECT_EQ(is_power_of_two(4u), 1);
    EXPECT_EQ(is_power_of_two(1024u), 1);       // 2^10
    EXPECT_EQ(is_power_of_two(0x80000000u), 1); // 2^31
}

TEST(IsPowerOfTwo, NotPowers) {
    EXPECT_EQ(is_power_of_two(0u), 0);   // ноль — не степень двойки
    EXPECT_EQ(is_power_of_two(3u), 0);
    EXPECT_EQ(is_power_of_two(6u), 0);
    EXPECT_EQ(is_power_of_two(0xFFFFFFFFu), 0);
}

// ---------- reverse_bytes ----------
TEST(ReverseBytes, SwapsByteOrder) {
    EXPECT_EQ(reverse_bytes(0x11223344u), 0x44332211u);
    EXPECT_EQ(reverse_bytes(0xAABBCCDDu), 0xDDCCBBAAu);
}

TEST(ReverseBytes, EdgeCases) {
    EXPECT_EQ(reverse_bytes(0x00000000u), 0x00000000u);
    EXPECT_EQ(reverse_bytes(0xFFFFFFFFu), 0xFFFFFFFFu);
    // Только младший байт занят — он уезжает в старший.
    EXPECT_EQ(reverse_bytes(0x000000FFu), 0xFF000000u);
    EXPECT_EQ(reverse_bytes(0x12000000u), 0x00000012u);
}

TEST(ReverseBytes, TwiceRestoresOriginal) {
    uint32_t x = 0xCAFEBABEu;
    EXPECT_EQ(reverse_bytes(reverse_bytes(x)), x);
}
