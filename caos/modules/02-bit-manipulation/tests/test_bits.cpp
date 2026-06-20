// Эти тесты трогать не нужно — это эталон поведения.
// Они подключают C-заголовок через extern "C", чтобы C++ увидел C-функции.
#include <gtest/gtest.h>
#include <random>
#include <cstdint>

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

// ============================================================
//  RANDOMIZED TESTS — deterministic seed, ~500-1000 iterations
// ============================================================

// ---------- GetBit randomized ----------

// Invariant: get_bit returns only 0 or 1.
TEST(GetBitRandom, ReturnsOnlyZeroOrOne) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        int result = get_bit(x, i);
        EXPECT_TRUE(result == 0 || result == 1)
            << "get_bit(" << x << ", " << i << ") = " << result;
    }
}

// Oracle: compare against the reference shift-and-mask formula.
TEST(GetBitRandom, MatchesOracle) {
    std::mt19937 rng(0xC0FFEE + 1);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x    = dist_x(rng);
        int i         = dist_i(rng);
        int expected  = static_cast<int>((x >> static_cast<unsigned>(i)) & 1u);
        EXPECT_EQ(get_bit(x, i), expected)
            << "x=" << x << " i=" << i;
    }
}

// Edge: all-zeros and all-ones for every bit position.
TEST(GetBitRandom, EdgeZeroAndMax) {
    for (int i = 0; i < 32; ++i) {
        EXPECT_EQ(get_bit(0u, i), 0)    << "i=" << i;
        EXPECT_EQ(get_bit(UINT32_MAX, i), 1) << "i=" << i;
    }
}

// ---------- SetBit randomized ----------

// Round-trip: after set_bit, get_bit must return 1.
TEST(SetBitRandom, SetThenGetReturnsOne) {
    std::mt19937 rng(0xC0FFEE + 2);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        EXPECT_EQ(get_bit(set_bit(x, i), i), 1)
            << "x=" << x << " i=" << i;
    }
}

// Other bits must be unchanged after set_bit.
TEST(SetBitRandom, DoesNotTouchOtherBits) {
    std::mt19937 rng(0xC0FFEE + 3);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 500; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        uint32_t result = set_bit(x, i);
        uint32_t mask   = 1u << static_cast<unsigned>(i);
        // All bits except bit i must be the same.
        EXPECT_EQ(result & ~mask, x & ~mask)
            << "x=" << x << " i=" << i;
    }
}

// Idempotent: setting an already-set bit changes nothing.
TEST(SetBitRandom, IdempotentOnSetBit) {
    std::mt19937 rng(0xC0FFEE + 4);
    std::uniform_int_distribution<int> dist_i(0, 31);
    for (int iter = 0; iter < 500; ++iter) {
        int i         = dist_i(rng);
        uint32_t x    = 1u << static_cast<unsigned>(i);  // bit i already set
        EXPECT_EQ(set_bit(x, i), x) << "x=" << x << " i=" << i;
    }
}

// ---------- ClearBit randomized ----------

// Round-trip: after clear_bit, get_bit must return 0.
TEST(ClearBitRandom, ClearThenGetReturnsZero) {
    std::mt19937 rng(0xC0FFEE + 5);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        EXPECT_EQ(get_bit(clear_bit(x, i), i), 0)
            << "x=" << x << " i=" << i;
    }
}

// Other bits must be unchanged after clear_bit.
TEST(ClearBitRandom, DoesNotTouchOtherBits) {
    std::mt19937 rng(0xC0FFEE + 6);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 500; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        uint32_t result = clear_bit(x, i);
        uint32_t mask   = 1u << static_cast<unsigned>(i);
        EXPECT_EQ(result & ~mask, x & ~mask)
            << "x=" << x << " i=" << i;
    }
}

// Idempotent: clearing an already-cleared bit changes nothing.
TEST(ClearBitRandom, IdempotentOnClearedBit) {
    std::mt19937 rng(0xC0FFEE + 7);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 500; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        // Force bit i to zero first, then clear again — must not change x.
        uint32_t cleared_once = clear_bit(x, i);
        EXPECT_EQ(clear_bit(cleared_once, i), cleared_once)
            << "x=" << x << " i=" << i;
    }
}

// ---------- ToggleBit randomized ----------

// Round-trip: toggle twice must restore the original value.
TEST(ToggleBitRandom, TwiceRestoresOriginal) {
    std::mt19937 rng(0xC0FFEE + 8);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        EXPECT_EQ(toggle_bit(toggle_bit(x, i), i), x)
            << "x=" << x << " i=" << i;
    }
}

// Toggle flips the bit: get_bit after toggle must differ from before.
TEST(ToggleBitRandom, ActuallyFlipsBit) {
    std::mt19937 rng(0xC0FFEE + 9);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x   = dist_x(rng);
        int i        = dist_i(rng);
        int before   = get_bit(x, i);
        int after    = get_bit(toggle_bit(x, i), i);
        EXPECT_NE(before, after) << "x=" << x << " i=" << i;
    }
}

// Toggle must not touch any bit other than position i.
TEST(ToggleBitRandom, DoesNotTouchOtherBits) {
    std::mt19937 rng(0xC0FFEE + 10);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 500; ++iter) {
        uint32_t x      = dist_x(rng);
        int i           = dist_i(rng);
        uint32_t result = toggle_bit(x, i);
        uint32_t mask   = 1u << static_cast<unsigned>(i);
        EXPECT_EQ(result & ~mask, x & ~mask)
            << "x=" << x << " i=" << i;
    }
}

// ---------- is_power_of_two randomized ----------

// Oracle: x != 0 && (x & (x-1)) == 0.
TEST(IsPowerOfTwoRandom, MatchesOracle) {
    std::mt19937 rng(0xC0FFEE + 11);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x   = dist_x(rng);
        int expected = (x != 0u && (x & (x - 1u)) == 0u) ? 1 : 0;
        EXPECT_EQ(is_power_of_two(x), expected) << "x=" << x;
    }
}

// All exact powers of two in [2^0 .. 2^31] must return 1.
TEST(IsPowerOfTwoRandom, AllExactPowers) {
    for (int k = 0; k < 32; ++k) {
        uint32_t p = 1u << static_cast<unsigned>(k);
        EXPECT_EQ(is_power_of_two(p), 1) << "2^" << k << "=" << p;
    }
}

// Numbers with two or more bits set must return 0.
TEST(IsPowerOfTwoRandom, TwoBitsSetIsNotPower) {
    std::mt19937 rng(0xC0FFEE + 12);
    std::uniform_int_distribution<int> dist_i(0, 30);
    for (int iter = 0; iter < 500; ++iter) {
        int lo     = dist_i(rng);
        int hi     = lo + 1 + (dist_i(rng) % (31 - lo));  // hi > lo, both in [0,31]
        uint32_t x = (1u << static_cast<unsigned>(lo)) | (1u << static_cast<unsigned>(hi));
        EXPECT_EQ(is_power_of_two(x), 0)
            << "x=" << x << " (bits " << lo << " and " << hi << " set)";
    }
}

// Edge: zero must return 0.
TEST(IsPowerOfTwoRandom, ZeroReturnsFalse) {
    EXPECT_EQ(is_power_of_two(0u), 0);
}

// ---------- reverse_bytes randomized ----------

// Round-trip: reverse_bytes applied twice gives back the original.
TEST(ReverseBytesRandom, TwiceRestoresOriginal) {
    std::mt19937 rng(0xC0FFEE + 13);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        EXPECT_EQ(reverse_bytes(reverse_bytes(x)), x) << "x=" << x;
    }
}

// Oracle: manually extract and re-assemble the four bytes.
TEST(ReverseBytesRandom, MatchesOracle) {
    std::mt19937 rng(0xC0FFEE + 14);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        uint32_t b0 = (x      ) & 0xFFu;
        uint32_t b1 = (x >>  8) & 0xFFu;
        uint32_t b2 = (x >> 16) & 0xFFu;
        uint32_t b3 = (x >> 24) & 0xFFu;
        uint32_t expected = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
        EXPECT_EQ(reverse_bytes(x), expected) << "x=0x" << std::hex << x;
    }
}

// Invariant: each individual byte in the result is the mirror byte from input.
TEST(ReverseBytesRandom, EachByteInCorrectPosition) {
    std::mt19937 rng(0xC0FFEE + 15);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    for (int iter = 0; iter < 500; ++iter) {
        uint32_t x   = dist_x(rng);
        uint32_t rev = reverse_bytes(x);
        // Byte at position k in rev must equal byte at position (3-k) in x.
        for (int k = 0; k < 4; ++k) {
            uint32_t byte_rev = (rev >> (8 * static_cast<unsigned>(k))) & 0xFFu;
            uint32_t byte_src = (x  >> (8 * static_cast<unsigned>(3 - k))) & 0xFFu;
            EXPECT_EQ(byte_rev, byte_src)
                << "x=0x" << std::hex << x << " byte position " << k;
        }
    }
}

// ---------- Cross-function invariants ----------

// set_bit then clear_bit on the same position restores original.
TEST(CrossFunctionRandom, SetThenClearRestoresOriginal) {
    std::mt19937 rng(0xC0FFEE + 16);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        // If bit i was originally 0: set then clear must return x.
        if (get_bit(x, i) == 0) {
            EXPECT_EQ(clear_bit(set_bit(x, i), i), x)
                << "x=" << x << " i=" << i;
        }
    }
}

// clear_bit then set_bit on the same position restores original.
TEST(CrossFunctionRandom, ClearThenSetRestoresOriginal) {
    std::mt19937 rng(0xC0FFEE + 17);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 1000; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        // If bit i was originally 1: clear then set must return x.
        if (get_bit(x, i) == 1) {
            EXPECT_EQ(set_bit(clear_bit(x, i), i), x)
                << "x=" << x << " i=" << i;
        }
    }
}

// toggle_bit on a 0 bit is equivalent to set_bit on the same position.
TEST(CrossFunctionRandom, ToggleOnZeroBitEqualsSetBit) {
    std::mt19937 rng(0xC0FFEE + 18);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 500; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        if (get_bit(x, i) == 0) {
            EXPECT_EQ(toggle_bit(x, i), set_bit(x, i))
                << "x=" << x << " i=" << i;
        }
    }
}

// toggle_bit on a 1 bit is equivalent to clear_bit on the same position.
TEST(CrossFunctionRandom, ToggleOnOneBitEqualsClearBit) {
    std::mt19937 rng(0xC0FFEE + 19);
    std::uniform_int_distribution<uint32_t> dist_x(0, UINT32_MAX);
    std::uniform_int_distribution<int>      dist_i(0, 31);
    for (int iter = 0; iter < 500; ++iter) {
        uint32_t x = dist_x(rng);
        int i      = dist_i(rng);
        if (get_bit(x, i) == 1) {
            EXPECT_EQ(toggle_bit(x, i), clear_bit(x, i))
                << "x=" << x << " i=" << i;
        }
    }
}
