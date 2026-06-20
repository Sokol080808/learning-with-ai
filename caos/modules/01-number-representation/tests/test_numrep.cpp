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

// ============================================================
// Randomised / property-based tests (seeded, deterministic)
// ============================================================
#include <random>
#include <cstring>
#include <climits>
#include <string>
#include <algorithm>
#include <bitset>

// ---- helper: build a binary string from a uint32_t value ----
static std::string make_binary_str(uint32_t x, int bits) {
    std::string s(bits, '0');
    for (int i = bits - 1; i >= 0; --i) {
        s[bits - 1 - i] = ((x >> i) & 1u) ? '1' : '0';
    }
    return s;
}

// ---- helper: build a hex string from a uint32_t value ----
static std::string make_hex_str(uint32_t x, bool upper = true) {
    if (x == 0) return "0";
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    std::string s;
    while (x) {
        s += digits[x & 0xFu];
        x >>= 4;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

// ---- helper: reference popcount ----
static int ref_popcount(uint32_t x) {
    int cnt = 0;
    while (x) { cnt += (x & 1u); x >>= 1; }
    return cnt;
}

// ============================================================
// parse_binary — round-trip and oracle, 1000 random values
// ============================================================
TEST(ParseBinary, RandomRoundTrip) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    std::uniform_int_distribution<int> bits_dist(1, 32);

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        int bits = bits_dist(rng);

        // Build the binary string using the reference helper (mask to `bits`)
        uint32_t masked = x & ((bits == 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u));
        std::string s = make_binary_str(masked, bits);

        // parse_binary must recover exactly `masked`
        EXPECT_EQ(parse_binary(s.c_str()), masked)
            << "input=\"" << s << "\" expected=" << masked;
    }
}

TEST(ParseBinary, RandomOracleAgainstShift) {
    // Independent oracle: accumulate left-shifts character by character
    std::mt19937 rng(0xBEEF01);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        std::string s = make_binary_str(x, 32);

        uint32_t oracle = 0;
        for (char c : s) oracle = (oracle << 1u) | (uint32_t)(c - '0');

        EXPECT_EQ(parse_binary(s.c_str()), oracle)
            << "input=\"" << s << "\"";
    }
}

// Edge cases: single bit strings
TEST(ParseBinary, SingleBitEdge) {
    EXPECT_EQ(parse_binary("0"), 0u);
    EXPECT_EQ(parse_binary("1"), 1u);
}

// Edge case: all-zeros of various lengths
TEST(ParseBinary, AllZeros) {
    for (int len = 1; len <= 32; ++len) {
        std::string s(len, '0');
        EXPECT_EQ(parse_binary(s.c_str()), 0u) << "len=" << len;
    }
}

// Edge case: all-ones of various lengths
TEST(ParseBinary, AllOnes) {
    for (int len = 1; len <= 32; ++len) {
        std::string s(len, '1');
        uint32_t expected = (len == 32) ? 0xFFFFFFFFu : ((1u << len) - 1u);
        EXPECT_EQ(parse_binary(s.c_str()), expected) << "len=" << len;
    }
}

// ============================================================
// format_binary — round-trip parse_binary∘format_binary
// ============================================================
TEST(FormatBinary, RandomRoundTrip) {
    std::mt19937 rng(0xC0FFEE2);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    std::uniform_int_distribution<int> bits_dist(1, 32);
    char buf[33];

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        int bits = bits_dist(rng);

        format_binary(x, bits, buf);

        // Length must equal bits
        EXPECT_EQ((int)std::strlen(buf), bits)
            << "x=" << x << " bits=" << bits;

        // Only '0' and '1' characters
        for (int k = 0; k < bits; ++k) {
            EXPECT_TRUE(buf[k] == '0' || buf[k] == '1')
                << "bad char at pos " << k << " for x=" << x << " bits=" << bits;
        }

        // round-trip: parse the produced string back
        uint32_t masked = x & ((bits == 32) ? 0xFFFFFFFFu : ((1u << bits) - 1u));
        EXPECT_EQ(parse_binary(buf), masked)
            << "x=" << x << " bits=" << bits << " buf=\"" << buf << "\"";
    }
}

TEST(FormatBinary, NullTerminatorAlwaysPresent) {
    std::mt19937 rng(0xCAFE);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    std::uniform_int_distribution<int> bits_dist(1, 32);
    char buf[34];

    for (int i = 0; i < 500; ++i) {
        uint32_t x = dist(rng);
        int bits = bits_dist(rng);
        // Poison the buffer
        std::memset(buf, 0xAA, sizeof(buf));
        format_binary(x, bits, buf);
        EXPECT_EQ(buf[bits], '\0')
            << "x=" << x << " bits=" << bits;
    }
}

TEST(FormatBinary, MSBCorrect) {
    // The most-significant produced character must equal bit (bits-1) of x
    std::mt19937 rng(0xF00D);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    std::uniform_int_distribution<int> bits_dist(1, 32);
    char buf[33];

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        int bits = bits_dist(rng);
        format_binary(x, bits, buf);

        int msb = (int)((x >> (bits - 1)) & 1u);
        char expected_char = (msb == 1) ? '1' : '0';
        EXPECT_EQ(buf[0], expected_char)
            << "x=" << x << " bits=" << bits;
    }
}

// ============================================================
// count_set_bits — oracle: std::bitset / naive loop
// ============================================================
TEST(CountSetBits, RandomOracle) {
    std::mt19937 rng(0xDEAD);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        int expected = ref_popcount(x);
        EXPECT_EQ(count_set_bits(x), expected) << "x=" << x;
    }
}

TEST(CountSetBits, PowersOfTwo) {
    // Powers of two always have exactly 1 set bit
    for (int k = 0; k < 32; ++k) {
        uint32_t x = 1u << k;
        EXPECT_EQ(count_set_bits(x), 1) << "x=1<<" << k;
    }
}

TEST(CountSetBits, OneLessThanPowerOfTwo) {
    // (2^k - 1) has exactly k set bits
    for (int k = 1; k <= 32; ++k) {
        uint32_t x = (k == 32) ? 0xFFFFFFFFu : ((1u << k) - 1u);
        EXPECT_EQ(count_set_bits(x), k) << "k=" << k;
    }
}

TEST(CountSetBits, AlternatingBits) {
    // 0xAAAAAAAA = ...10101010 => 16 set bits
    EXPECT_EQ(count_set_bits(0xAAAAAAAAu), 16);
    // 0x55555555 = ...01010101 => 16 set bits
    EXPECT_EQ(count_set_bits(0x55555555u), 16);
}

TEST(CountSetBits, ComplementSumsTo32) {
    // popcount(x) + popcount(~x) == 32 for any x
    std::mt19937 rng(0xC0DE);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        EXPECT_EQ(count_set_bits(x) + count_set_bits(~x), 32) << "x=" << x;
    }
}

// ============================================================
// parse_hex — oracle and round-trip
// ============================================================
TEST(ParseHex, RandomOracleUppercase) {
    std::mt19937 rng(0xABCD);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        std::string s = make_hex_str(x, /*upper=*/true);
        EXPECT_EQ(parse_hex(s.c_str()), x)
            << "s=\"" << s << "\" expected=" << x;
    }
}

TEST(ParseHex, RandomOracleLowercase) {
    std::mt19937 rng(0x1234);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        std::string s = make_hex_str(x, /*upper=*/false);
        EXPECT_EQ(parse_hex(s.c_str()), x)
            << "s=\"" << s << "\" expected=" << x;
    }
}

TEST(ParseHex, RandomMixedCase) {
    // Randomly flip case of each character
    std::mt19937 rng(0x5678);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    std::uniform_int_distribution<int> flip(0, 1);

    for (int i = 0; i < 500; ++i) {
        uint32_t x = dist(rng);
        std::string s = make_hex_str(x, true);
        for (char& c : s) {
            if (c >= 'A' && c <= 'F' && flip(rng)) c = (char)(c - 'A' + 'a');
        }
        EXPECT_EQ(parse_hex(s.c_str()), x)
            << "s=\"" << s << "\" expected=" << x;
    }
}

TEST(ParseHex, SingleDigits) {
    // All 16 single-digit hex values
    const char* digits[] = {
        "0","1","2","3","4","5","6","7",
        "8","9","A","B","C","D","E","F"
    };
    for (int v = 0; v < 16; ++v) {
        EXPECT_EQ(parse_hex(digits[v]), (uint32_t)v) << "digit=" << digits[v];
    }
    // Lowercase
    const char* ldigits[] = {"a","b","c","d","e","f"};
    for (int v = 0; v < 6; ++v) {
        EXPECT_EQ(parse_hex(ldigits[v]), (uint32_t)(10 + v)) << "digit=" << ldigits[v];
    }
}

TEST(ParseHex, LeadingZerosParsedCorrectly) {
    // "00001A" == 26
    EXPECT_EQ(parse_hex("00001A"), 26u);
    EXPECT_EQ(parse_hex("0000"), 0u);
    EXPECT_EQ(parse_hex("00000001"), 1u);
}

// ============================================================
// Cross-function invariants
// ============================================================
TEST(CrossFunction, FormatThenCountBits) {
    // count_set_bits(x) must equal number of '1' characters in format_binary(x,32,buf)
    std::mt19937 rng(0x9999);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    char buf[33];

    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        format_binary(x, 32, buf);

        int ones_in_str = 0;
        for (int k = 0; k < 32; ++k) ones_in_str += (buf[k] == '1');

        EXPECT_EQ(count_set_bits(x), ones_in_str) << "x=" << x;
    }
}

TEST(CrossFunction, ParseBinaryMatchesParseHex) {
    // For values 0..255: parse_binary(binary_str) == parse_hex(hex_str)
    std::mt19937 rng(0x7777);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFu);  // 16-bit values safe for both

    for (int i = 0; i < 500; ++i) {
        uint32_t x = dist(rng);
        std::string bin_s = make_binary_str(x, 32);
        std::string hex_s = make_hex_str(x, true);

        EXPECT_EQ(parse_binary(bin_s.c_str()), parse_hex(hex_s.c_str()))
            << "x=" << x;
    }
}
