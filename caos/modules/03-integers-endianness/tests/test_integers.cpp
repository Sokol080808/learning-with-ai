// Эти тесты трогать не нужно — это эталон поведения.
#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <random>

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

// ============================================================
// Randomized / property-based tests (seeded, deterministic)
// ============================================================

// --- add_overflows (randomized) --------------------------------------------

// Oracle: compute in int64_t, check if result fits in int32_t.
static int oracle_add_overflows(int32_t a, int32_t b) {
    int64_t result = (int64_t)a + (int64_t)b;
    return (result > INT32_MAX || result < INT32_MIN) ? 1 : 0;
}

TEST(AddOverflows, RandomNoOverflowPairs) {
    // Both values in [-1000, 1000] — sum always fits.
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int32_t> small(-1000, 1000);
    for (int i = 0; i < 1000; ++i) {
        int32_t a = small(rng);
        int32_t b = small(rng);
        EXPECT_EQ(add_overflows(a, b), 0)
            << "a=" << a << " b=" << b;
    }
}

TEST(AddOverflows, RandomOracleCheck) {
    // Full int32_t range — compare against int64_t oracle.
    std::mt19937 rng(0xDEAD1234);
    std::uniform_int_distribution<int32_t> full(INT32_MIN, INT32_MAX);
    for (int i = 0; i < 1000; ++i) {
        int32_t a = full(rng);
        int32_t b = full(rng);
        EXPECT_EQ(add_overflows(a, b), oracle_add_overflows(a, b))
            << "a=" << a << " b=" << b;
    }
}

TEST(AddOverflows, BoundaryOverflowCases) {
    // INT32_MAX - k + (k+1) overflows; INT32_MAX - k + k does not.
    std::mt19937 rng(0xBEEF0001);
    std::uniform_int_distribution<int32_t> small(0, 1000);
    for (int i = 0; i < 500; ++i) {
        int32_t k = small(rng);
        // INT32_MAX - k + (k+1) overflows
        EXPECT_EQ(add_overflows(INT32_MAX - k, k + 1), 1)
            << "k=" << k;
        // INT32_MAX - k + k == INT32_MAX, no overflow
        EXPECT_EQ(add_overflows(INT32_MAX - k, k), 0)
            << "k=" << k;
        // INT32_MIN + k + (-(k+1)) overflows downward
        EXPECT_EQ(add_overflows(INT32_MIN + k, -(k + 1)), 1)
            << "k=" << k;
        // INT32_MIN + k + (-k) == INT32_MIN, no overflow
        EXPECT_EQ(add_overflows(INT32_MIN + k, -k), 0)
            << "k=" << k;
    }
}

TEST(AddOverflows, MixedSignNeverOverflows) {
    // a > 0, b < 0 (or vice versa) can never overflow int32_t.
    std::mt19937 rng(0xABCD0001);
    std::uniform_int_distribution<int32_t> pos(1, INT32_MAX);
    std::uniform_int_distribution<int32_t> neg(INT32_MIN, -1);
    for (int i = 0; i < 1000; ++i) {
        int32_t a = pos(rng);
        int32_t b = neg(rng);
        EXPECT_EQ(add_overflows(a, b), 0) << "a=" << a << " b=" << b;
        EXPECT_EQ(add_overflows(b, a), 0) << "b=" << b << " a=" << a;
    }
}

// --- swap16 (randomized) ---------------------------------------------------

TEST(Swap16, RandomRoundTrip) {
    // swap16(swap16(x)) == x for all x.
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFF);
    for (int i = 0; i < 1000; ++i) {
        uint16_t x = static_cast<uint16_t>(dist(rng));
        EXPECT_EQ(swap16(swap16(x)), x) << "x=" << x;
    }
}

TEST(Swap16, RandomOracleByteSwap) {
    // Oracle: manually swap bytes via memcpy.
    std::mt19937 rng(0xFACE0001);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFF);
    for (int i = 0; i < 1000; ++i) {
        uint16_t x = static_cast<uint16_t>(dist(rng));
        // Oracle: high byte of result = low byte of x, low byte of result = high byte of x.
        uint16_t oracle = static_cast<uint16_t>(((x & 0x00FFu) << 8) | ((x & 0xFF00u) >> 8));
        EXPECT_EQ(swap16(x), oracle) << "x=0x" << std::hex << x;
    }
}

TEST(Swap16, HighByteBecomesLow) {
    // For any x, high byte of result == low byte of x.
    std::mt19937 rng(0x11223344);
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFF);
    for (int i = 0; i < 1000; ++i) {
        uint16_t x = static_cast<uint16_t>(dist(rng));
        uint16_t r = swap16(x);
        EXPECT_EQ(r >> 8, x & 0xFF) << "x=0x" << std::hex << x;
        EXPECT_EQ(r & 0xFF, x >> 8)  << "x=0x" << std::hex << x;
    }
}

// --- swap32 (randomized) ---------------------------------------------------

TEST(Swap32, RandomRoundTrip) {
    // swap32(swap32(x)) == x for all x.
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<uint32_t> dist;
    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        EXPECT_EQ(swap32(swap32(x)), x) << "x=0x" << std::hex << x;
    }
}

TEST(Swap32, RandomOracleByteReverse) {
    // Oracle: reverse 4 bytes using memcpy into byte array.
    std::mt19937 rng(0xBEEFCAFE);
    std::uniform_int_distribution<uint32_t> dist;
    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        uint8_t bytes[4];
        std::memcpy(bytes, &x, 4);
        uint32_t oracle = ((uint32_t)bytes[0] << 24) |
                          ((uint32_t)bytes[1] << 16) |
                          ((uint32_t)bytes[2] <<  8) |
                          ((uint32_t)bytes[3]);
        EXPECT_EQ(swap32(x), oracle) << "x=0x" << std::hex << x;
    }
}

TEST(Swap32, BytePositionsCorrect) {
    // Byte 0 of input must appear as byte 3 of output, etc.
    std::mt19937 rng(0x55AA55AA);
    std::uniform_int_distribution<uint32_t> dist;
    for (int i = 0; i < 1000; ++i) {
        uint32_t x = dist(rng);
        uint32_t r = swap32(x);
        // byte k of r == byte (3-k) of x
        for (int k = 0; k < 4; ++k) {
            uint8_t r_byte = (r >> (k * 8)) & 0xFF;
            uint8_t x_byte = (x >> ((3 - k) * 8)) & 0xFF;
            EXPECT_EQ(r_byte, x_byte)
                << "byte " << k << " mismatch for x=0x" << std::hex << x;
        }
    }
}

TEST(Swap32, Swap16Consistency) {
    // swap32 on a value with equal upper/lower 16-bit halves should equal
    // swap16 applied to each half (placed in swapped positions).
    // i.e. swap32(AABB | AABB<<16) == swap16(AABB) | swap16(AABB)<<16
    std::mt19937 rng(0x9988AABB);
    std::uniform_int_distribution<uint32_t> half(0, 0xFFFF);
    for (int i = 0; i < 500; ++i) {
        uint32_t lo = half(rng);
        uint32_t x = (lo << 16) | lo;
        uint32_t r = swap32(x);
        uint16_t sw = swap16(static_cast<uint16_t>(lo));
        uint32_t expected = ((uint32_t)sw << 16) | (uint32_t)sw;
        EXPECT_EQ(r, expected) << "lo=0x" << std::hex << lo;
    }
}

// --- sign_extend (randomized) ----------------------------------------------

// Oracle: standard C sign extension via casting.
static int32_t oracle_sign_extend(uint32_t x, int bits) {
    if (bits == 32) return (int32_t)x;
    // Mask to bits width, then use struct bitfield trick via shift.
    uint32_t mask = (1u << bits) - 1u;
    uint32_t val = x & mask;
    uint32_t sign_bit = 1u << (bits - 1);
    if (val & sign_bit) {
        // Set all upper bits to 1
        return (int32_t)(val | (~mask));
    } else {
        return (int32_t)val;
    }
}

TEST(SignExtend, RandomAllWidths) {
    // For bits 1..32, compare against oracle for random inputs.
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<uint32_t> vdist;
    std::uniform_int_distribution<int> bdist(1, 32);
    for (int i = 0; i < 1000; ++i) {
        uint32_t x = vdist(rng);
        int bits = bdist(rng);
        int32_t got = sign_extend(x, bits);
        int32_t expected = oracle_sign_extend(x, bits);
        EXPECT_EQ(got, expected)
            << "x=0x" << std::hex << x << " bits=" << std::dec << bits;
    }
}

TEST(SignExtend, HigherBitsIgnored) {
    // Bits above 'bits' in x must not affect the result.
    std::mt19937 rng(0xDECAF000);
    std::uniform_int_distribution<uint32_t> vdist;
    std::uniform_int_distribution<int> bdist(1, 31);
    for (int i = 0; i < 1000; ++i) {
        int bits = bdist(rng);
        uint32_t mask = (1u << bits) - 1u;
        uint32_t lo = vdist(rng) & mask;          // only low bits
        uint32_t hi = vdist(rng) & ~mask;         // only upper bits (garbage)
        EXPECT_EQ(sign_extend(lo | hi, bits), sign_extend(lo, bits))
            << "bits=" << bits << " lo=0x" << std::hex << lo;
    }
}

TEST(SignExtend, SignBitZeroMeansPositive) {
    // If the sign bit (bit bits-1) is 0, result must be >= 0.
    std::mt19937 rng(0xBEEFBEEF);
    std::uniform_int_distribution<int> bdist(2, 31);
    std::uniform_int_distribution<uint32_t> vdist;
    for (int i = 0; i < 1000; ++i) {
        int bits = bdist(rng);
        uint32_t mask = (1u << bits) - 1u;
        uint32_t sign_bit = 1u << (bits - 1);
        // Force sign bit off
        uint32_t x = vdist(rng) & mask & ~sign_bit;
        int32_t result = sign_extend(x, bits);
        EXPECT_GE(result, 0)
            << "x=0x" << std::hex << x << " bits=" << std::dec << bits;
    }
}

TEST(SignExtend, SignBitOneMeansNegative) {
    // If the sign bit (bit bits-1) is 1, result must be < 0.
    std::mt19937 rng(0xF00DCAFE);
    std::uniform_int_distribution<int> bdist(2, 31);
    std::uniform_int_distribution<uint32_t> vdist;
    for (int i = 0; i < 1000; ++i) {
        int bits = bdist(rng);
        uint32_t mask = (1u << bits) - 1u;
        uint32_t sign_bit = 1u << (bits - 1);
        // Force sign bit on
        uint32_t x = (vdist(rng) & mask) | sign_bit;
        int32_t result = sign_extend(x, bits);
        EXPECT_LT(result, 0)
            << "x=0x" << std::hex << x << " bits=" << std::dec << bits;
    }
}

TEST(SignExtend, MinMaxForEachWidth) {
    // For each width 1..31: max positive = 2^(bits-1)-1, min negative = -2^(bits-1).
    for (int bits = 1; bits <= 31; ++bits) {
        int32_t max_pos = (int32_t)((1u << (bits - 1)) - 1u);
        int32_t min_neg = -(int32_t)(1u << (bits - 1));
        uint32_t max_x = (1u << (bits - 1)) - 1u;  // all bits set except sign
        uint32_t min_x = (1u << (bits - 1));        // only sign bit set
        EXPECT_EQ(sign_extend(max_x, bits), max_pos) << "bits=" << bits;
        EXPECT_EQ(sign_extend(min_x, bits), min_neg) << "bits=" << bits;
    }
}

TEST(SignExtend, AllOnesAnyWidth) {
    // All lower 'bits' bits set = -1 for any width 1..32.
    for (int bits = 1; bits <= 32; ++bits) {
        uint32_t x = (bits == 32) ? 0xFFFFFFFFu : (1u << bits) - 1u;
        EXPECT_EQ(sign_extend(x, bits), -1) << "bits=" << bits;
    }
}

// --- Cross-function / combined properties ----------------------------------

TEST(Swap32Swap16, Composed) {
    // swap32(x) applied twice is identity; independently check each half
    // matches two swap16 calls.
    std::mt19937 rng(0x12345678);
    std::uniform_int_distribution<uint32_t> dist;
    for (int i = 0; i < 500; ++i) {
        uint32_t x = dist(rng);
        // Round-trip
        EXPECT_EQ(swap32(swap32(x)), x);
        EXPECT_EQ(swap16(swap16(static_cast<uint16_t>(x & 0xFFFF))),
                  static_cast<uint16_t>(x & 0xFFFF));
    }
}

TEST(AddOverflows, ZeroIdentity) {
    // a + 0 never overflows, for all a.
    std::mt19937 rng(0xABCDEF01);
    std::uniform_int_distribution<int32_t> dist(INT32_MIN, INT32_MAX);
    for (int i = 0; i < 1000; ++i) {
        int32_t a = dist(rng);
        EXPECT_EQ(add_overflows(a, 0), 0) << "a=" << a;
        EXPECT_EQ(add_overflows(0, a), 0) << "a=" << a;
    }
}

TEST(AddOverflows, Commutativity) {
    // add_overflows(a, b) == add_overflows(b, a).
    std::mt19937 rng(0x76543210);
    std::uniform_int_distribution<int32_t> dist(INT32_MIN, INT32_MAX);
    for (int i = 0; i < 1000; ++i) {
        int32_t a = dist(rng);
        int32_t b = dist(rng);
        EXPECT_EQ(add_overflows(a, b), add_overflows(b, a))
            << "a=" << a << " b=" << b;
    }
}
