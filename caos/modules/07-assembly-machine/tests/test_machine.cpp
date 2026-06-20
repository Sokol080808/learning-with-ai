// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, ЧТО три функции обязаны возвращать. Если тест красный — смотри,
// что ожидалось (Expected) и что получилось (Actual), и правь src/machine.c.

#include <gtest/gtest.h>

extern "C" {
#include "machine.h"
}

// --- Листинг A: asm_a(x) = x*x + 1 ---

TEST(AsmA, Zero) {
    EXPECT_EQ(asm_a(0), 1);     // 0*0 + 1
}

TEST(AsmA, One) {
    EXPECT_EQ(asm_a(1), 2);     // 1*1 + 1
}

TEST(AsmA, SmallPositive) {
    EXPECT_EQ(asm_a(5), 26);    // 25 + 1
    EXPECT_EQ(asm_a(10), 101);  // 100 + 1
}

TEST(AsmA, Negative) {
    EXPECT_EQ(asm_a(-3), 10);   // (-3)*(-3) + 1 = 9 + 1
}

// --- Листинг B: asm_b(a, b) = max(a, b) ---

TEST(AsmB, FirstLarger) {
    EXPECT_EQ(asm_b(7, 3), 7);
}

TEST(AsmB, SecondLarger) {
    EXPECT_EQ(asm_b(3, 7), 7);
}

TEST(AsmB, Equal) {
    EXPECT_EQ(asm_b(4, 4), 4);
}

TEST(AsmB, Negatives) {
    EXPECT_EQ(asm_b(-5, -2), -2);  // -2 > -5
    EXPECT_EQ(asm_b(-9, 0), 0);
}

// --- Листинг C: asm_c(n) = 1 + 2 + ... + n ---

TEST(AsmC, One) {
    EXPECT_EQ(asm_c(1), 1);
}

TEST(AsmC, Five) {
    EXPECT_EQ(asm_c(5), 15);    // 1+2+3+4+5
}

TEST(AsmC, Hundred) {
    EXPECT_EQ(asm_c(100), 5050);  // классическая сумма Гаусса
}

TEST(AsmC, ZeroOrNegative) {
    EXPECT_EQ(asm_c(0), 0);     // нечего суммировать
    EXPECT_EQ(asm_c(-4), 0);    // тоже 0: цикл не выполняется ни разу
}

// ============================================================
// Randomized / property tests (seeded, deterministic)
// ============================================================

#include <random>
#include <cstdlib>   // std::abs
#include <algorithm> // std::max

// --- asm_a randomized: oracle = x*x + 1 ---

TEST(AsmA, RandomPositive) {
    std::mt19937 rng(0xC0FFEE);
    // draw from [0, 1000] — squares stay well within int range
    std::uniform_int_distribution<int> dist(0, 1000);
    for (int i = 0; i < 500; ++i) {
        int x = dist(rng);
        EXPECT_EQ(asm_a(x), x * x + 1) << "x=" << x;
    }
}

TEST(AsmA, RandomNegative) {
    std::mt19937 rng(0xDEADBEEF);
    std::uniform_int_distribution<int> dist(-1000, -1);
    for (int i = 0; i < 500; ++i) {
        int x = dist(rng);
        EXPECT_EQ(asm_a(x), x * x + 1) << "x=" << x;
    }
}

TEST(AsmA, RandomMixed) {
    std::mt19937 rng(0xFEEDFACE);
    std::uniform_int_distribution<int> dist(-500, 500);
    for (int i = 0; i < 1000; ++i) {
        int x = dist(rng);
        EXPECT_EQ(asm_a(x), x * x + 1) << "x=" << x;
    }
}

// asm_a result is always >= 1 (x*x >= 0, +1 > 0)
TEST(AsmA, ResultAlwaysAtLeastOne) {
    std::mt19937 rng(0xCAFEBABE);
    std::uniform_int_distribution<int> dist(-1000, 1000);
    for (int i = 0; i < 1000; ++i) {
        int x = dist(rng);
        EXPECT_GE(asm_a(x), 1) << "x=" << x;
    }
}

// asm_a is symmetric: asm_a(x) == asm_a(-x)
TEST(AsmA, Symmetry) {
    std::mt19937 rng(0xABCDEF01);
    std::uniform_int_distribution<int> dist(0, 1000);
    for (int i = 0; i < 500; ++i) {
        int x = dist(rng);
        EXPECT_EQ(asm_a(x), asm_a(-x)) << "x=" << x;
    }
}

// Edge cases: INT boundary neighbours (no overflow: 46340*46340+1 < INT_MAX)
TEST(AsmA, EdgeCases) {
    EXPECT_EQ(asm_a(0),  1);
    EXPECT_EQ(asm_a(1),  2);
    EXPECT_EQ(asm_a(-1), 2);
    EXPECT_EQ(asm_a(46340), 46340 * 46340 + 1);  // largest safe square in int
}

// --- asm_b randomized: oracle = std::max(a, b) ---

TEST(AsmB, RandomPairs) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 1000; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        EXPECT_EQ(asm_b(a, b), std::max(a, b)) << "a=" << a << " b=" << b;
    }
}

// Idempotence: asm_b(x, x) == x
TEST(AsmB, Idempotent) {
    std::mt19937 rng(0xDEADC0DE);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 500; ++i) {
        int x = dist(rng);
        EXPECT_EQ(asm_b(x, x), x) << "x=" << x;
    }
}

// Commutativity: asm_b(a, b) == asm_b(b, a)
TEST(AsmB, Commutative) {
    std::mt19937 rng(0x12345678);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 1000; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        EXPECT_EQ(asm_b(a, b), asm_b(b, a)) << "a=" << a << " b=" << b;
    }
}

// Result is always >= both inputs
TEST(AsmB, DominatesInputs) {
    std::mt19937 rng(0x87654321);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 1000; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        int m = asm_b(a, b);
        EXPECT_GE(m, a) << "a=" << a << " b=" << b;
        EXPECT_GE(m, b) << "a=" << a << " b=" << b;
    }
}

// Result must equal one of the inputs (not something invented)
TEST(AsmB, IsOneOfInputs) {
    std::mt19937 rng(0xAA55AA55);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 1000; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        int m = asm_b(a, b);
        EXPECT_TRUE(m == a || m == b) << "a=" << a << " b=" << b << " got=" << m;
    }
}

// Edge cases: boundary values
TEST(AsmB, EdgeCases) {
    EXPECT_EQ(asm_b(0, 0), 0);
    EXPECT_EQ(asm_b(INT_MIN, INT_MAX), INT_MAX);
    EXPECT_EQ(asm_b(INT_MAX, INT_MIN), INT_MAX);
    EXPECT_EQ(asm_b(INT_MIN, INT_MIN), INT_MIN);
    EXPECT_EQ(asm_b(INT_MAX, INT_MAX), INT_MAX);
    EXPECT_EQ(asm_b(-1, 0), 0);
    EXPECT_EQ(asm_b(0, -1), 0);
}

// --- asm_c randomized: oracle = n*(n+1)/2 (Gauss) ---

TEST(AsmC, RandomSmall) {
    std::mt19937 rng(0xC0FFEE);
    // keep n small so n*(n+1)/2 doesn't overflow int (n <= 65535 is safe)
    std::uniform_int_distribution<int> dist(1, 1000);
    for (int i = 0; i < 1000; ++i) {
        int n = dist(rng);
        int expected = n * (n + 1) / 2;
        EXPECT_EQ(asm_c(n), expected) << "n=" << n;
    }
}

// Non-positive n always returns 0
TEST(AsmC, NonPositiveAlwaysZero) {
    std::mt19937 rng(0xBEEFCAFE);
    std::uniform_int_distribution<int> dist(-10000, 0);
    for (int i = 0; i < 500; ++i) {
        int n = dist(rng);
        EXPECT_EQ(asm_c(n), 0) << "n=" << n;
    }
}

// Monotone: asm_c(n+1) == asm_c(n) + (n+1) for n >= 0
TEST(AsmC, Monotone) {
    std::mt19937 rng(0x55AAFF00);
    std::uniform_int_distribution<int> dist(0, 999);
    for (int i = 0; i < 500; ++i) {
        int n = dist(rng);
        EXPECT_EQ(asm_c(n + 1), asm_c(n) + (n + 1)) << "n=" << n;
    }
}

// Edge cases: n=1,2 and boundary between positive/non-positive
TEST(AsmC, EdgeCases) {
    EXPECT_EQ(asm_c(2), 3);
    EXPECT_EQ(asm_c(3), 6);
    EXPECT_EQ(asm_c(-1), 0);
    EXPECT_EQ(asm_c(INT_MIN), 0);
}

// ============================================================
// Задание 4: frame_read_local — чтение локальных переменных
// из снимка кадра стека с корректным знаковым расширением.
// ============================================================

// Вспомогательный builder: заполняет байты нужного слота в снимке.
// frame_size байт, слот по offset от rbp (отрицательный) размером size.
static void write_slot(uint8_t *buf, int frame_size, int offset, int size,
                        int64_t value) {
    // rbp = buf + frame_size; слот начинается по buf + frame_size + offset
    uint8_t *ptr = buf + frame_size + offset;
    memcpy(ptr, &value, (size_t)size);
}

// --- 8-байтный слот (int64_t) ---

TEST(FrameReadLocal, Size8Positive) {
    const int FSIZE = 32;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-8, 8}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -8, 8, (int64_t)0x0123456789ABCDEFLL);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)0x0123456789ABCDEFLL);
}

TEST(FrameReadLocal, Size8Negative) {
    const int FSIZE = 32;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-8, 8}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -8, 8, (int64_t)-42LL);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)-42LL);
}

// --- 4-байтный слот (int32_t), знаковое расширение до int64_t ---

TEST(FrameReadLocal, Size4Positive) {
    const int FSIZE = 32;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-4, 4}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -4, 4, (int64_t)123456);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)123456);
}

TEST(FrameReadLocal, Size4NegativeSignExtended) {
    const int FSIZE = 32;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-4, 4}};
    FrameLayout layout = {FSIZE, 1, slots};

    // -1 как int32_t = 0xFFFFFFFF; после расширения должно быть -1LL (0xFFFFFFFFFFFFFFFF)
    write_slot(frame, FSIZE, -4, 4, (int64_t)(int32_t)-1);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)-1LL);
}

TEST(FrameReadLocal, Size4MinInt) {
    const int FSIZE = 32;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-4, 4}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -4, 4, (int64_t)INT32_MIN);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)INT32_MIN);
}

// --- 2-байтный слот (int16_t), знаковое расширение ---

TEST(FrameReadLocal, Size2Positive) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-2, 2}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -2, 2, (int64_t)1000);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)1000LL);
}

TEST(FrameReadLocal, Size2NegativeSignExtended) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-2, 2}};
    FrameLayout layout = {FSIZE, 1, slots};

    // 0xFF00 в виде int16_t = -256; расширяется до -256LL
    write_slot(frame, FSIZE, -2, 2, (int64_t)(int16_t)-256);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)-256LL);
}

// --- 1-байтный слот (int8_t), знаковое расширение ---

TEST(FrameReadLocal, Size1Positive) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-1, 1}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -1, 1, (int64_t)100);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)100LL);
}

TEST(FrameReadLocal, Size1NegativeSignExtended) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-1, 1}};
    FrameLayout layout = {FSIZE, 1, slots};

    // 0xFF как int8_t = -1; должно расшириться до -1LL, не до 255LL
    write_slot(frame, FSIZE, -1, 1, (int64_t)(int8_t)-1);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)-1LL);
}

TEST(FrameReadLocal, Size1MinusTwenty) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-1, 1}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -1, 1, (int64_t)(int8_t)-20);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)-20LL);
}

// --- Несколько слотов в одном кадре ---

TEST(FrameReadLocal, MultipleSlots) {
    // Кадр: int64_t a @ [rbp-8], int32_t b @ [rbp-12], int8_t c @ [rbp-13]
    const int FSIZE = 32;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {
        {-8,  8},   // slot 0: int64_t
        {-12, 4},   // slot 1: int32_t
        {-13, 1},   // slot 2: int8_t
    };
    FrameLayout layout = {FSIZE, 3, slots};

    write_slot(frame, FSIZE, -8,  8, (int64_t)9000000000LL);
    write_slot(frame, FSIZE, -12, 4, (int64_t)(int32_t)-7);
    write_slot(frame, FSIZE, -13, 1, (int64_t)(int8_t)42);

    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)9000000000LL);
    EXPECT_EQ(frame_read_local(&layout, frame, 1), (int64_t)-7LL);
    EXPECT_EQ(frame_read_local(&layout, frame, 2), (int64_t)42LL);
}

// --- Граничные значения типов ---

TEST(FrameReadLocal, BoundaryInt8) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-1, 1}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -1, 1, (int64_t)INT8_MAX);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)127LL);

    write_slot(frame, FSIZE, -1, 1, (int64_t)(int8_t)INT8_MIN);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)-128LL);
}

TEST(FrameReadLocal, BoundaryInt16) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-2, 2}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -2, 2, (int64_t)INT16_MAX);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)32767LL);

    write_slot(frame, FSIZE, -2, 2, (int64_t)(int16_t)INT16_MIN);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)-32768LL);
}

TEST(FrameReadLocal, BoundaryInt32) {
    const int FSIZE = 16;
    uint8_t frame[FSIZE] = {};
    SlotInfo slots[] = {{-4, 4}};
    FrameLayout layout = {FSIZE, 1, slots};

    write_slot(frame, FSIZE, -4, 4, (int64_t)INT32_MAX);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)INT32_MAX);

    write_slot(frame, FSIZE, -4, 4, (int64_t)INT32_MIN);
    EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)INT32_MIN);
}

// --- Рандомизированный тест: знаковое расширение для всех четырёх размеров ---

TEST(FrameReadLocal, RandomizedSignExtension) {
    std::mt19937_64 rng(0xC0DEC4050700ULL);
    std::uniform_int_distribution<int64_t> dist64(INT64_MIN, INT64_MAX);

    const int FSIZE = 64;
    uint8_t frame[FSIZE];

    // size=8
    {
        SlotInfo s = {-8, 8};
        FrameLayout layout = {FSIZE, 1, &s};
        for (int i = 0; i < 200; ++i) {
            int64_t val = dist64(rng);
            memset(frame, 0, FSIZE);
            write_slot(frame, FSIZE, -8, 8, val);
            EXPECT_EQ(frame_read_local(&layout, frame, 0), val) << "size=8 val=" << val;
        }
    }
    // size=4
    {
        SlotInfo s = {-4, 4};
        FrameLayout layout = {FSIZE, 1, &s};
        std::uniform_int_distribution<int32_t> d32(INT32_MIN, INT32_MAX);
        for (int i = 0; i < 200; ++i) {
            int32_t val = d32(rng);
            memset(frame, 0, FSIZE);
            write_slot(frame, FSIZE, -4, 4, (int64_t)val);
            EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)val)
                << "size=4 val=" << val;
        }
    }
    // size=2
    {
        SlotInfo s = {-2, 2};
        FrameLayout layout = {FSIZE, 1, &s};
        std::uniform_int_distribution<int16_t> d16(INT16_MIN, INT16_MAX);
        for (int i = 0; i < 200; ++i) {
            int16_t val = d16(rng);
            memset(frame, 0, FSIZE);
            write_slot(frame, FSIZE, -2, 2, (int64_t)val);
            EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)val)
                << "size=2 val=" << val;
        }
    }
    // size=1
    {
        SlotInfo s = {-1, 1};
        FrameLayout layout = {FSIZE, 1, &s};
        std::uniform_int_distribution<int16_t> d8(-128, 127);
        for (int i = 0; i < 200; ++i) {
            int8_t val = (int8_t)d8(rng);
            memset(frame, 0, FSIZE);
            write_slot(frame, FSIZE, -1, 1, (int64_t)val);
            EXPECT_EQ(frame_read_local(&layout, frame, 0), (int64_t)val)
                << "size=1 val=" << (int)val;
        }
    }
}
