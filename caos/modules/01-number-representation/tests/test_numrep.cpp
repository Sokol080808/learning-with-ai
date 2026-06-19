// Эти тесты трогать не нужно — это эталон поведения.
// Они включают C-заголовок через extern "C", чтобы C++ увидел C-функции.

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "numrep.h"
}

// ---------- parse_binary ----------

TEST(ParseBinary, Basic) {
    EXPECT_EQ(parse_binary("1011"), 11u);
    EXPECT_EQ(parse_binary("0"), 0u);
    EXPECT_EQ(parse_binary("1"), 1u);
    EXPECT_EQ(parse_binary("10"), 2u);
    EXPECT_EQ(parse_binary("100"), 4u);
}

TEST(ParseBinary, LeadingZeros) {
    EXPECT_EQ(parse_binary("00001011"), 11u);
    EXPECT_EQ(parse_binary("0000"), 0u);
}

TEST(ParseBinary, Empty) {
    EXPECT_EQ(parse_binary(""), 0u);
}

TEST(ParseBinary, Wide) {
    // 32 единицы подряд -> максимальное 32-битное беззнаковое.
    EXPECT_EQ(parse_binary("11111111111111111111111111111111"), 0xFFFFFFFFu);
    // Старший бит выставлен (значение не помещается в int32 — но uint32 ок).
    EXPECT_EQ(parse_binary("10000000000000000000000000000000"), 0x80000000u);
}

// ---------- format_binary ----------

TEST(FormatBinary, Basic) {
    char buf[33];
    format_binary(5, 8, buf);
    EXPECT_STREQ(buf, "00000101");
    format_binary(11, 4, buf);
    EXPECT_STREQ(buf, "1011");
    format_binary(0, 4, buf);
    EXPECT_STREQ(buf, "0000");
}

TEST(FormatBinary, OnlyLowBits) {
    char buf[33];
    // bits меньше реальной ширины -> берём только младшие bits разрядов.
    format_binary(0xFF, 4, buf); // 11111111 -> младшие 4 -> 1111
    EXPECT_STREQ(buf, "1111");
    format_binary(0b1011, 2, buf); // младшие 2 бита числа 1011 -> 11
    EXPECT_STREQ(buf, "11");
}

TEST(FormatBinary, FullWidth) {
    char buf[33];
    format_binary(0x80000000u, 32, buf);
    EXPECT_STREQ(buf, "10000000000000000000000000000000");
    EXPECT_EQ(std::strlen(buf), 32u);
}

TEST(FormatBinary, NullTerminated) {
    char buf[10];
    std::memset(buf, 'X', sizeof(buf));
    format_binary(2, 4, buf);
    EXPECT_STREQ(buf, "0010");
    EXPECT_EQ(buf[4], '\0'); // ровно после bits символов стоит терминатор
}

// ---------- count_set_bits ----------

TEST(CountSetBits, Basic) {
    EXPECT_EQ(count_set_bits(0u), 0);
    EXPECT_EQ(count_set_bits(1u), 1);
    EXPECT_EQ(count_set_bits(11u), 3); // 1011
    EXPECT_EQ(count_set_bits(255u), 8);
}

TEST(CountSetBits, AllBits) {
    EXPECT_EQ(count_set_bits(0xFFFFFFFFu), 32);
    EXPECT_EQ(count_set_bits(0x80000000u), 1); // только старший
    EXPECT_EQ(count_set_bits(0xAAAAAAAAu), 16); // через один
}

// ---------- parse_hex ----------

TEST(ParseHex, Basic) {
    EXPECT_EQ(parse_hex("1A"), 26u);
    EXPECT_EQ(parse_hex("0"), 0u);
    EXPECT_EQ(parse_hex("F"), 15u);
    EXPECT_EQ(parse_hex("10"), 16u);
    EXPECT_EQ(parse_hex("FF"), 255u);
}

TEST(ParseHex, CaseInsensitive) {
    EXPECT_EQ(parse_hex("1a"), 26u);
    EXPECT_EQ(parse_hex("ff"), 255u);
    EXPECT_EQ(parse_hex("aBcDeF"), 0xABCDEFu);
}

TEST(ParseHex, Empty) {
    EXPECT_EQ(parse_hex(""), 0u);
}

TEST(ParseHex, Wide) {
    EXPECT_EQ(parse_hex("DEADBEEF"), 0xDEADBEEFu);
    EXPECT_EQ(parse_hex("FFFFFFFF"), 0xFFFFFFFFu);
    EXPECT_EQ(parse_hex("80000000"), 0x80000000u);
}
