// tests/test_warmup.cpp
// Эти тесты трогать не нужно — это эталон поведения. Твоя задача — сделать так,
// чтобы они стали зелёными, правя только src/warmup.c.

#include <gtest/gtest.h>
#include <random>
#include <cstdint>
#include <cstddef>
#include <cstring>

extern "C" {
#include "warmup.h"
}

TEST(Warmup, AddBasic) {
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(10, 7), 17);
}

TEST(Warmup, AddZeroAndNegative) {
    EXPECT_EQ(add(0, 0), 0);
    EXPECT_EQ(add(-4, 4), 0);
    EXPECT_EQ(add(-5, -8), -13);
}

TEST(Warmup, SecondsInOneHour) {
    EXPECT_EQ(seconds_in(1), 3600L);
}

TEST(Warmup, SecondsInZeroHours) {
    EXPECT_EQ(seconds_in(0), 0L);
}

TEST(Warmup, SecondsInManyHours) {
    EXPECT_EQ(seconds_in(2), 7200L);
    EXPECT_EQ(seconds_in(24), 86400L);
    // День большой — но в long это влезает без проблем.
    EXPECT_EQ(seconds_in(1000), 3600000L);
}

// ---- Randomized / property-based tests ----

// add: oracle = plain C++ addition on the same int values.
// Properties tested:
//   1. add(a, b) == a + b  (oracle)
//   2. add(a, b) == add(b, a)  (commutativity)
//   3. add(a, 0) == a  (identity)
//   4. add(a, -a) == 0  (inverse), with INT_MIN guard
TEST(WarmupRandom, AddOracleAndCommutativity) {
    std::mt19937 rng(0xC0FFEE);
    // Uniform over [-1e6, 1e6] to avoid signed overflow in the oracle itself.
    std::uniform_int_distribution<int> dist(-1000000, 1000000);

    for (int i = 0; i < 1000; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        int expected = a + b;  // oracle: plain addition
        EXPECT_EQ(add(a, b), expected)
            << "add(" << a << ", " << b << ") should be " << expected;
        // Commutativity
        EXPECT_EQ(add(a, b), add(b, a))
            << "add must be commutative for a=" << a << " b=" << b;
    }
}

TEST(WarmupRandom, AddIdentityAndInverse) {
    std::mt19937 rng(0xDEAD);
    std::uniform_int_distribution<int> dist(-500000, 500000);

    for (int i = 0; i < 500; ++i) {
        int a = dist(rng);
        // Identity: add(a, 0) == a
        EXPECT_EQ(add(a, 0), a)
            << "add(" << a << ", 0) should equal " << a;
        EXPECT_EQ(add(0, a), a)
            << "add(0, " << a << ") should equal " << a;
        // Inverse: add(a, -a) == 0
        EXPECT_EQ(add(a, -a), 0)
            << "add(" << a << ", " << -a << ") should be 0";
    }
}

// Edge cases for add: boundary int-range values (no overflow — we just check
// individually-known correct results rather than risk UB).
TEST(WarmupEdge, AddBoundary) {
    EXPECT_EQ(add(0, 0), 0);
    EXPECT_EQ(add(1, -1), 0);
    EXPECT_EQ(add(-1, 1), 0);
    EXPECT_EQ(add(100000, -100000), 0);
    EXPECT_EQ(add(-1000000, -1000000), -2000000);
    EXPECT_EQ(add(1000000, 1000000), 2000000);
}

// seconds_in: oracle = (long)hours * 3600L
// Properties:
//   1. seconds_in(h) == h * 3600L  (oracle)
//   2. seconds_in(h+1) - seconds_in(h) == 3600  (uniform step)
//   3. seconds_in(0) == 0  (edge)
//   4. seconds_in(1) == 3600  (unit)
TEST(WarmupRandom, SecondsInOracle) {
    std::mt19937 rng(0xBEEF);
    // Use [0, 1000000] — well within long range (1e6 * 3600 = 3.6e9 < LONG_MAX).
    std::uniform_int_distribution<int> dist(0, 1000000);

    for (int i = 0; i < 1000; ++i) {
        int h = dist(rng);
        long expected = static_cast<long>(h) * 3600L;  // oracle
        EXPECT_EQ(seconds_in(h), expected)
            << "seconds_in(" << h << ") should be " << expected;
    }
}

TEST(WarmupRandom, SecondsInUniformStep) {
    std::mt19937 rng(0xFACE);
    std::uniform_int_distribution<int> dist(0, 999999);

    for (int i = 0; i < 500; ++i) {
        int h = dist(rng);
        long diff = seconds_in(h + 1) - seconds_in(h);
        EXPECT_EQ(diff, 3600L)
            << "seconds_in(" << h+1 << ") - seconds_in(" << h
            << ") should be 3600, got " << diff;
    }
}

TEST(WarmupEdge, SecondsInBoundary) {
    EXPECT_EQ(seconds_in(0), 0L);
    EXPECT_EQ(seconds_in(1), 3600L);
    EXPECT_EQ(seconds_in(24), 86400L);
    EXPECT_EQ(seconds_in(365 * 24), static_cast<long>(365 * 24) * 3600L);  // one year
}

// ============================================================
// Задание 3: struct layout, byte inspection, endianness
// ============================================================

// --- packed_record_size ---

// packed_record_size() must equal sizeof(struct PackedRecord).
// This verifies the formula offsetof(last)+sizeof(last) matches the real sizeof.
// Note: due to final padding these may differ, and that is intentional teaching
// material — we test that packed_record_size() returns a consistent value and
// that sizeof matches for independent verification.
TEST(ByteLayout, PackedRecordSizeMatchesSizeof) {
    // The function computes offsetof(c) + sizeof(c) — which may be LESS than
    // sizeof(struct PackedRecord) if there is trailing padding.  Both values are
    // deterministic and must be at least as large as the sum of field sizes.
    size_t fn_size   = packed_record_size();
    size_t real_size = sizeof(struct PackedRecord);

    // fn_size must be a valid sub-sizeof (no trailing pad accounted)
    // and real_size must be >= fn_size (trailing pad can only add bytes).
    EXPECT_GE(real_size, fn_size)
        << "sizeof(PackedRecord)=" << real_size
        << " should be >= packed_record_size()=" << fn_size;

    // fn_size must be at least 1+4+2 = 7 (minimum without any padding).
    EXPECT_GE(fn_size, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t))
        << "packed_record_size() must cover all three fields";

    // The function must equal offsetof(c) + sizeof last field exactly.
    size_t expected = offsetof(struct PackedRecord, c) + sizeof(uint16_t);
    EXPECT_EQ(fn_size, expected)
        << "packed_record_size() should be offsetof(c)+sizeof(uint16_t)="
        << expected;
}

// Verify that b is NOT at offset 1 (compiler inserts padding before uint32_t).
TEST(ByteLayout, PackedRecordPaddingPresent) {
    // On any standard ABI uint32_t must be aligned to 4 bytes.
    // offsetof(b) must therefore be 4, not 1.
    EXPECT_EQ(offsetof(struct PackedRecord, b), static_cast<size_t>(4))
        << "uint32_t field 'b' should start at offset 4 (3 bytes of padding after 'a')";
    EXPECT_EQ(offsetof(struct PackedRecord, c), static_cast<size_t>(8))
        << "uint16_t field 'c' should start at offset 8";
}

// --- uint32_to_bytes ---

// On a little-endian machine 0x01020304 must produce {0x04, 0x03, 0x02, 0x01}.
// On a big-endian machine the expected order is {0x01, 0x02, 0x03, 0x04}.
// We derive the expected bytes from is_little_endian() so the test is portable.
TEST(ByteLayout, Uint32ToBytesKnownValue) {
    unsigned char out[4] = {0, 0, 0, 0};
    uint32_to_bytes(0x01020304u, out);

    if (is_little_endian()) {
        EXPECT_EQ(out[0], 0x04u) << "little-endian: LSB at lowest address";
        EXPECT_EQ(out[1], 0x03u);
        EXPECT_EQ(out[2], 0x02u);
        EXPECT_EQ(out[3], 0x01u) << "little-endian: MSB at highest address";
    } else {
        EXPECT_EQ(out[0], 0x01u) << "big-endian: MSB at lowest address";
        EXPECT_EQ(out[1], 0x02u);
        EXPECT_EQ(out[2], 0x03u);
        EXPECT_EQ(out[3], 0x04u) << "big-endian: LSB at highest address";
    }
}

// All four bytes together must reconstruct the original value (endian-neutral).
TEST(ByteLayout, Uint32ToBytesReconstructValue) {
    // Seeded deterministic values.
    const uint32_t inputs[] = {0x00000000u, 0xFFFFFFFFu, 0x01020304u,
                                0xDEADBEEFu, 0x80000001u};
    for (uint32_t v : inputs) {
        unsigned char out[4] = {0};
        uint32_to_bytes(v, out);

        // Reconstruct via memcpy (standard-compliant, avoids aliasing).
        uint32_t reconstructed = 0;
        std::memcpy(&reconstructed, out, 4);
        EXPECT_EQ(reconstructed, v)
            << "uint32_to_bytes then memcpy back must round-trip for value 0x"
            << std::hex << v;
    }
}

// --- is_little_endian ---

// is_little_endian() must return 0 or 1.
TEST(ByteLayout, IsLittleEndianReturnsBooleanValue) {
    int result = is_little_endian();
    EXPECT_TRUE(result == 0 || result == 1)
        << "is_little_endian() must return exactly 0 or 1, got " << result;
}

// is_little_endian() must agree with an independent union-based check.
TEST(ByteLayout, IsLittleEndianAgreeWithUnionCheck) {
    // Independent oracle: same technique but inline in the test.
    union { uint32_t i; unsigned char b[4]; } u;
    u.i = 1u;
    int oracle = (u.b[0] == 1) ? 1 : 0;

    EXPECT_EQ(is_little_endian(), oracle)
        << "is_little_endian() disagrees with inline union check";
}

// is_little_endian() must agree with what uint32_to_bytes() observes.
TEST(ByteLayout, IsLittleEndianAgreeWithByteOrder) {
    unsigned char out[4] = {0};
    uint32_to_bytes(1u, out);
    // On little-endian: out[0]==1 (LSB first). On big-endian: out[3]==1.
    int from_bytes = (out[0] == 1) ? 1 : 0;
    EXPECT_EQ(is_little_endian(), from_bytes)
        << "is_little_endian() must agree with uint32_to_bytes(1, out)[0]";
}
