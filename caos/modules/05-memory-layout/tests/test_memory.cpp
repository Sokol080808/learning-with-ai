// Эти тесты трогать не нужно — это эталон поведения.
// Если хоть один красный — чини src/memory.c, а не этот файл.

#include <gtest/gtest.h>
#include <cstdint>   // uintptr_t — для аккуратной проверки выравнивания
#include <random>    // std::mt19937, std::uniform_int_distribution
#include <cstring>   // memcpy

extern "C" {
#include "memory.h"
}

// --- align_up -------------------------------------------------------------

TEST(AlignUp, RoundsUpToBoundary) {
    EXPECT_EQ(align_up(13, 8), 16u);
    EXPECT_EQ(align_up(1, 8), 8u);
    EXPECT_EQ(align_up(7, 8), 8u);
    EXPECT_EQ(align_up(9, 8), 16u);
}

TEST(AlignUp, AlreadyAlignedStaysSame) {
    EXPECT_EQ(align_up(0, 8), 0u);
    EXPECT_EQ(align_up(8, 8), 8u);
    EXPECT_EQ(align_up(16, 8), 16u);
    EXPECT_EQ(align_up(64, 16), 64u);
}

TEST(AlignUp, OtherPowersOfTwo) {
    EXPECT_EQ(align_up(5, 1), 5u);    // выравнивание по 1 ничего не меняет
    EXPECT_EQ(align_up(3, 4), 4u);
    EXPECT_EQ(align_up(17, 16), 32u);
    EXPECT_EQ(align_up(1000, 64), 1024u);
}

// --- is_aligned -----------------------------------------------------------

TEST(IsAligned, AlignByOneAlwaysTrue) {
    int x = 0;
    EXPECT_EQ(is_aligned(&x, 1), 1);
}

TEST(IsAligned, IntIsAlignedToFour) {
    // Компилятор размещает int на адресе, кратном alignof(int) == 4.
    int x = 0;
    EXPECT_EQ(is_aligned(&x, 4), 1);
}

TEST(IsAligned, DetectsMisaligned) {
    // Берём байтовый буфер и сдвигаем указатель на 1 байт — он гарантированно
    // НЕ кратен 2. А сам буфер выровняем явно, чтобы старт был чётным.
    alignas(8) unsigned char buf[8] = {0};
    const void* base   = &buf[0];   // адрес кратен 8 -> и 2 тоже
    const void* shift1 = &buf[1];   // на 1 байт правее -> нечётный

    EXPECT_EQ(is_aligned(base, 2), 1);
    EXPECT_EQ(is_aligned(shift1, 2), 0);
}

TEST(IsAligned, MatchesLowBitsOfAddress) {
    // Независимая проверка: ответ функции должен совпадать с "младшие биты адреса нули".
    alignas(16) unsigned char buf[16] = {0};
    for (size_t i = 0; i < sizeof(buf); ++i) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(&buf[i]);
        int expected8 = (addr % 8 == 0) ? 1 : 0;
        EXPECT_EQ(is_aligned(&buf[i], 8), expected8) << "i=" << i;
    }
}

// --- nth_element ----------------------------------------------------------

TEST(NthElement, ReturnsAddressOfElement) {
    int arr[5] = {10, 20, 30, 40, 50};
    EXPECT_EQ(nth_element(arr, 0), &arr[0]);
    EXPECT_EQ(nth_element(arr, 2), &arr[2]);
    EXPECT_EQ(nth_element(arr, 4), &arr[4]);
}

TEST(NthElement, PointsToCorrectValue) {
    int arr[5] = {10, 20, 30, 40, 50};
    // Сначала убеждаемся, что вернулся НЕ NULL: ASSERT прерывает только этот
    // тест, поэтому заглушка `return NULL;` даёт честный красный, а не краш.
    int* p3 = nth_element(arr, 3);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(*p3, 40);
    int* p0 = nth_element(arr, 0);
    ASSERT_NE(p0, nullptr);
    EXPECT_EQ(*p0, 10);
}

TEST(NthElement, MovesByElementsNotBytes) {
    // Разница адресов между соседними элементами == sizeof(int) байт.
    int arr[3] = {1, 2, 3};
    char* a0 = reinterpret_cast<char*>(nth_element(arr, 0));
    char* a1 = reinterpret_cast<char*>(nth_element(arr, 1));
    EXPECT_EQ(a1 - a0, static_cast<long>(sizeof(int)));
}

// --- ptr_distance ---------------------------------------------------------

TEST(PtrDistance, CountsElementsForward) {
    int arr[10] = {0};
    EXPECT_EQ(ptr_distance(&arr[5], &arr[2]), 3);
    EXPECT_EQ(ptr_distance(&arr[9], &arr[0]), 9);
}

TEST(PtrDistance, ZeroWhenSame) {
    int arr[4] = {0};
    EXPECT_EQ(ptr_distance(&arr[2], &arr[2]), 0);
    EXPECT_EQ(ptr_distance(&arr[0], &arr[0]), 0);
    // Контрольная не-нулевая проверка в том же кейсе: иначе заглушка
    // `return 0;` дала бы ложно-зелёный результат.
    EXPECT_EQ(ptr_distance(&arr[3], &arr[1]), 2);
}

TEST(PtrDistance, NegativeWhenBackward) {
    int arr[10] = {0};
    EXPECT_EQ(ptr_distance(&arr[1], &arr[7]), -6);
}

TEST(PtrDistance, ConsistentWithNthElement) {
    int arr[8] = {0};
    int* a = nth_element(arr, 6);
    int* b = nth_element(arr, 1);
    EXPECT_EQ(ptr_distance(a, b), 5);
}

// ============================================================
// Randomised / property tests — deterministic seed 0xC0FFEE
// ============================================================

// --- align_up: invariants over ~1000 random (n, align) pairs ---------------

TEST(AlignUpRandom, ResultIsAlignedAndGeN) {
    // Property 1: result >= n
    // Property 2: result % align == 0
    // Property 3: result < n + align  (i.e. the smallest such multiple)
    std::mt19937 rng(0xC0FFEE);
    // align is a power of two in [1, 4096]
    std::uniform_int_distribution<size_t> dist_shift(0, 12); // 2^0..2^12
    std::uniform_int_distribution<size_t> dist_n(0, 65535);

    for (int i = 0; i < 1000; ++i) {
        size_t align = size_t(1) << dist_shift(rng);
        size_t n = dist_n(rng);
        size_t r = align_up(n, align);

        EXPECT_GE(r, n)           << "align_up(" << n << ", " << align << ") < n";
        EXPECT_EQ(r % align, 0u)  << "align_up(" << n << ", " << align << ") not aligned";
        // must be the SMALLEST such multiple: r - align < n  (or r == 0 when n==0)
        if (n > 0) {
            EXPECT_LT(r, n + align)
                << "align_up(" << n << ", " << align << ") skipped a boundary";
        }
    }
}

TEST(AlignUpRandom, AlreadyAlignedIsIdempotent) {
    // align_up of an already-aligned value must return that value unchanged.
    std::mt19937 rng(0xC0FFEE + 1);
    std::uniform_int_distribution<size_t> dist_shift(0, 12);
    std::uniform_int_distribution<size_t> dist_k(0, 1023);

    for (int i = 0; i < 1000; ++i) {
        size_t align = size_t(1) << dist_shift(rng);
        size_t n = dist_k(rng) * align;   // guaranteed multiple of align
        EXPECT_EQ(align_up(n, align), n)
            << "align_up(" << n << ", " << align << ") changed an already-aligned value";
    }
}

TEST(AlignUpRandom, OracleComparison) {
    // Oracle: plain arithmetic (n == 0 ? 0 : ((n - 1) / align + 1) * align).
    // The function under test must match for every random pair.
    std::mt19937 rng(0xC0FFEE + 2);
    std::uniform_int_distribution<size_t> dist_shift(0, 10);
    std::uniform_int_distribution<size_t> dist_n(0, 100000);

    for (int i = 0; i < 1000; ++i) {
        size_t align = size_t(1) << dist_shift(rng);
        size_t n     = dist_n(rng);
        size_t expected = (n == 0) ? 0u : ((n - 1) / align + 1) * align;
        EXPECT_EQ(align_up(n, align), expected)
            << "align_up(" << n << ", " << align << ") mismatch with oracle";
    }
}

TEST(AlignUpRandom, SpecialValues) {
    // Edge cases: n==0 always returns 0; align==1 is identity.
    EXPECT_EQ(align_up(0, 1),    0u);
    EXPECT_EQ(align_up(0, 4),    0u);
    EXPECT_EQ(align_up(0, 4096), 0u);

    for (size_t n = 0; n <= 256; ++n) {
        EXPECT_EQ(align_up(n, 1), n) << "align_up(" << n << ", 1) != " << n;
    }

    // Large power-of-two boundary
    size_t big = size_t(1) << 20;  // 1 MiB
    EXPECT_EQ(align_up(big, big), big);
    EXPECT_EQ(align_up(big + 1, big), big * 2);
}

// --- is_aligned: invariants over byte-buffer offsets -----------------------

TEST(IsAlignedRandom, ConsistentWithModuloOracle) {
    // For every byte offset in an aligned buffer, is_aligned must agree with
    // (address % align == 0) for all power-of-two aligns 1..64.
    alignas(64) unsigned char buf[128] = {0};
    std::mt19937 rng(0xC0FFEE + 3);
    std::uniform_int_distribution<size_t> dist_off(0, 127);
    std::uniform_int_distribution<size_t> dist_shift(0, 6); // 2^0..2^6=64

    for (int i = 0; i < 1000; ++i) {
        size_t off   = dist_off(rng);
        size_t align = size_t(1) << dist_shift(rng);
        const void* p = &buf[off];
        uintptr_t addr = reinterpret_cast<uintptr_t>(p);
        int expected = (addr % align == 0) ? 1 : 0;
        EXPECT_EQ(is_aligned(p, align), expected)
            << "is_aligned(buf+" << off << ", " << align << ")";
    }
}

TEST(IsAlignedRandom, AlignByOneAlwaysTrue) {
    alignas(64) unsigned char buf[64] = {0};
    for (size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(is_aligned(&buf[i], 1), 1) << "i=" << i;
    }
}

TEST(IsAlignedRandom, AlignedAddressReturnsOne) {
    // An address that we KNOW is aligned must return 1.
    alignas(64) unsigned char buf[64] = {0};
    // buf itself is aligned to 64, so aligned to 1/2/4/8/16/32/64 too.
    for (size_t shift = 0; shift <= 6; ++shift) {
        size_t align = size_t(1) << shift;
        EXPECT_EQ(is_aligned(&buf[0], align), 1)
            << "align=" << align;
    }
}

TEST(IsAlignedRandom, MisalignedAddressReturnsZero) {
    // buf[0] aligned to 64 -> buf[1] is NOT aligned to 2/4/8/16/32/64.
    alignas(64) unsigned char buf[64] = {0};
    for (size_t shift = 1; shift <= 6; ++shift) {
        size_t align = size_t(1) << shift;
        EXPECT_EQ(is_aligned(&buf[1], align), 0)
            << "align=" << align;
    }
}

// --- nth_element: randomised array + index ---------------------------------

TEST(NthElementRandom, PointerEqualsBaseOffsetByN) {
    // nth_element(base, n) must return exactly base + n (pointer arithmetic).
    std::mt19937 rng(0xC0FFEE + 4);
    const size_t MAXN = 512;
    std::vector<int> arr(MAXN);
    for (size_t i = 0; i < MAXN; ++i) arr[i] = static_cast<int>(i * 7 + 3);

    std::uniform_int_distribution<size_t> dist_idx(0, MAXN - 1);

    for (int trial = 0; trial < 1000; ++trial) {
        size_t n = dist_idx(rng);
        int* got = nth_element(arr.data(), n);
        EXPECT_EQ(got, arr.data() + n) << "nth_element index=" << n;
    }
}

TEST(NthElementRandom, DereferencedValueMatchesArray) {
    // Reading through the returned pointer must give the correct value.
    std::mt19937 rng(0xC0FFEE + 5);
    const size_t MAXN = 256;
    std::vector<int> arr(MAXN);
    for (size_t i = 0; i < MAXN; ++i) arr[i] = static_cast<int>(rng());

    std::uniform_int_distribution<size_t> dist_idx(0, MAXN - 1);

    for (int trial = 0; trial < 1000; ++trial) {
        size_t n = dist_idx(rng);
        int* p = nth_element(arr.data(), n);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(*p, arr[n]) << "value mismatch at index=" << n;
    }
}

TEST(NthElementRandom, ConsecutiveElementsAreOneIntApart) {
    // The byte distance between nth_element(base,i) and nth_element(base,i+1)
    // must be exactly sizeof(int), regardless of the base address.
    std::mt19937 rng(0xC0FFEE + 6);
    const size_t MAXN = 200;
    std::vector<int> arr(MAXN);
    std::uniform_int_distribution<size_t> dist_idx(0, MAXN - 2);

    for (int trial = 0; trial < 500; ++trial) {
        size_t n = dist_idx(rng);
        char* p0 = reinterpret_cast<char*>(nth_element(arr.data(), n));
        char* p1 = reinterpret_cast<char*>(nth_element(arr.data(), n + 1));
        EXPECT_EQ(p1 - p0, static_cast<ptrdiff_t>(sizeof(int)))
            << "stride wrong at index=" << n;
    }
}

TEST(NthElementRandom, IndexZeroIsBaseItself) {
    std::mt19937 rng(0xC0FFEE + 7);
    const size_t MAXN = 64;
    std::vector<int> arr(MAXN);
    for (int trial = 0; trial < 200; ++trial) {
        // Shuffle arr to make sure the test isn't accidentally trivial.
        for (auto& v : arr) v = static_cast<int>(rng());
        EXPECT_EQ(nth_element(arr.data(), 0), arr.data());
    }
}

// --- ptr_distance: randomised pairs ----------------------------------------

TEST(PtrDistanceRandom, MatchesSubtractionOracle) {
    // ptr_distance(a, b) must equal (a - b) in elements.
    // Oracle: ((char*)a - (char*)b) / sizeof(int)
    std::mt19937 rng(0xC0FFEE + 8);
    const size_t MAXN = 512;
    std::vector<int> arr(MAXN, 0);

    std::uniform_int_distribution<size_t> dist_idx(0, MAXN - 1);

    for (int trial = 0; trial < 1000; ++trial) {
        size_t ia = dist_idx(rng);
        size_t ib = dist_idx(rng);
        long expected = static_cast<long>(ia) - static_cast<long>(ib);
        EXPECT_EQ(ptr_distance(&arr[ia], &arr[ib]), expected)
            << "ptr_distance(arr+" << ia << ", arr+" << ib << ")";
    }
}

TEST(PtrDistanceRandom, SignIsCorrect) {
    // When ia > ib the result must be positive; ia < ib → negative; ia==ib → 0.
    std::mt19937 rng(0xC0FFEE + 9);
    const size_t MAXN = 256;
    std::vector<int> arr(MAXN, 0);
    std::uniform_int_distribution<size_t> dist_idx(0, MAXN - 1);

    int positives = 0, negatives = 0;
    for (int trial = 0; trial < 1000; ++trial) {
        size_t ia = dist_idx(rng);
        size_t ib = dist_idx(rng);
        long d = ptr_distance(&arr[ia], &arr[ib]);
        if (ia > ib) { EXPECT_GT(d, 0L); ++positives; }
        else if (ia < ib) { EXPECT_LT(d, 0L); ++negatives; }
        else { EXPECT_EQ(d, 0L); }
    }
    // Sanity: the random seed must actually exercise all three branches.
    EXPECT_GT(positives, 0);
    EXPECT_GT(negatives, 0);
}

TEST(PtrDistanceRandom, AntiSymmetry) {
    // ptr_distance(a, b) == -ptr_distance(b, a) for all pairs.
    std::mt19937 rng(0xC0FFEE + 10);
    const size_t MAXN = 256;
    std::vector<int> arr(MAXN, 0);
    std::uniform_int_distribution<size_t> dist_idx(0, MAXN - 1);

    for (int trial = 0; trial < 1000; ++trial) {
        size_t ia = dist_idx(rng);
        size_t ib = dist_idx(rng);
        long dab = ptr_distance(&arr[ia], &arr[ib]);
        long dba = ptr_distance(&arr[ib], &arr[ia]);
        EXPECT_EQ(dab, -dba)
            << "anti-symmetry violated: ia=" << ia << " ib=" << ib;
    }
}

TEST(PtrDistanceRandom, ConsistentWithNthElementRandom) {
    // Combine nth_element + ptr_distance: they must agree with direct index arithmetic.
    std::mt19937 rng(0xC0FFEE + 11);
    const size_t MAXN = 300;
    std::vector<int> arr(MAXN, 0);
    std::uniform_int_distribution<size_t> dist_idx(0, MAXN - 1);

    for (int trial = 0; trial < 800; ++trial) {
        size_t ia = dist_idx(rng);
        size_t ib = dist_idx(rng);
        int* pa = nth_element(arr.data(), ia);
        int* pb = nth_element(arr.data(), ib);
        long expected = static_cast<long>(ia) - static_cast<long>(ib);
        EXPECT_EQ(ptr_distance(pa, pb), expected)
            << "via nth_element: ia=" << ia << " ib=" << ib;
    }
}

TEST(PtrDistanceRandom, MaxDistanceEdge) {
    // Check the maximum signed distance within a large array doesn't overflow.
    const size_t MAXN = 1024;
    std::vector<int> arr(MAXN, 0);
    long d_fwd = ptr_distance(&arr[MAXN - 1], &arr[0]);
    long d_rev = ptr_distance(&arr[0], &arr[MAXN - 1]);
    EXPECT_EQ(d_fwd, static_cast<long>(MAXN - 1));
    EXPECT_EQ(d_rev, -static_cast<long>(MAXN - 1));
}

// ============================================================
// layout_compute — инспектор раскладки структур (задание 5)
// ============================================================

// Вспомогательная функция: поле-дескриптор.
static Field make_field(const char* name, size_t size, size_t align) {
    Field f;
    f.name  = name;
    f.size  = size;
    f.align = align;
    return f;
}

// --- Известные раскладки против компилятора ----------------------------

// struct { char c; int n; }  -> {0, 4}, sizeof=8
TEST(LayoutCompute, CharThenInt) {
    Field fields[2] = {
        make_field("c", 1, 1),
        make_field("n", 4, 4),
    };
    size_t offsets[2];
    size_t sz = layout_compute(fields, 2, offsets);

    EXPECT_EQ(offsets[0], 0u);   // char at 0
    EXPECT_EQ(offsets[1], 4u);   // int at 4 (3 bytes padding)
    EXPECT_EQ(sz,         8u);   // sizeof == 8
}

// struct { int n; char c; }  -> {0, 4}, sizeof=8
TEST(LayoutCompute, IntThenChar) {
    Field fields[2] = {
        make_field("n", 4, 4),
        make_field("c", 1, 1),
    };
    size_t offsets[2];
    size_t sz = layout_compute(fields, 2, offsets);

    EXPECT_EQ(offsets[0], 0u);   // int at 0
    EXPECT_EQ(offsets[1], 4u);   // char at 4
    EXPECT_EQ(sz,         8u);   // tail-padding to align 4
}

// struct { double b; char a; char c; }  -> {0, 8, 9}, sizeof=16
TEST(LayoutCompute, DoubleTwoChars) {
    Field fields[3] = {
        make_field("b", 8, 8),
        make_field("a", 1, 1),
        make_field("c", 1, 1),
    };
    size_t offsets[3];
    size_t sz = layout_compute(fields, 3, offsets);

    EXPECT_EQ(offsets[0],  0u);  // double at 0
    EXPECT_EQ(offsets[1],  8u);  // char at 8
    EXPECT_EQ(offsets[2],  9u);  // char at 9
    EXPECT_EQ(sz,         16u);  // tail-padding to align 8
}

// struct { char a; double b; char c; }  -> {0, 8, 16}, sizeof=24  (BAD layout)
TEST(LayoutCompute, BadLayout_CharDoublChar) {
    Field fields[3] = {
        make_field("a", 1, 1),
        make_field("b", 8, 8),
        make_field("c", 1, 1),
    };
    size_t offsets[3];
    size_t sz = layout_compute(fields, 3, offsets);

    EXPECT_EQ(offsets[0],  0u);  // char at 0
    EXPECT_EQ(offsets[1],  8u);  // double at 8  (7 bytes padding)
    EXPECT_EQ(offsets[2], 16u);  // char at 16
    EXPECT_EQ(sz,         24u);  // tail-padding to align 8
}

// Single field, no padding anywhere.
TEST(LayoutCompute, SingleField) {
    Field f = make_field("x", 4, 4);
    size_t offset;
    size_t sz = layout_compute(&f, 1, &offset);
    EXPECT_EQ(offset, 0u);
    EXPECT_EQ(sz, 4u);
}

// struct { short s; short t; int n; }  -> {0, 2, 4}, sizeof=8
TEST(LayoutCompute, TwoShortsThenInt) {
    Field fields[3] = {
        make_field("s", 2, 2),
        make_field("t", 2, 2),
        make_field("n", 4, 4),
    };
    size_t offsets[3];
    size_t sz = layout_compute(fields, 3, offsets);

    EXPECT_EQ(offsets[0], 0u);
    EXPECT_EQ(offsets[1], 2u);
    EXPECT_EQ(offsets[2], 4u);
    EXPECT_EQ(sz,         8u);
}

// struct { char a; char b; char c; char d; }  -> all at 0..3, sizeof=4
TEST(LayoutCompute, FourChars) {
    Field fields[4] = {
        make_field("a", 1, 1), make_field("b", 1, 1),
        make_field("c", 1, 1), make_field("d", 1, 1),
    };
    size_t offsets[4];
    size_t sz = layout_compute(fields, 4, offsets);

    for (int i = 0; i < 4; ++i) EXPECT_EQ(offsets[i], static_cast<size_t>(i));
    EXPECT_EQ(sz, 4u);
}

// --- Свойство: sizeof кратен строжайшему align -------------------------

TEST(LayoutComputeProps, SizeofMultipleOfMaxAlign) {
    // For all layouts, sizeof must be a multiple of max(align).
    // Use a fixed deterministic set (seed is implicit in static data).
    struct TestCase {
        Field   fields[4];
        int     n;
    };
    // Build test cases statically (no random, fully deterministic).
    Field tc0[3] = { make_field("a",1,1), make_field("b",4,4), make_field("c",1,1) };
    Field tc1[2] = { make_field("a",8,8), make_field("b",1,1) };
    Field tc2[4] = { make_field("a",1,1), make_field("b",2,2), make_field("c",4,4), make_field("d",8,8) };
    Field tc3[1] = { make_field("x",3,1) };

    auto check = [](const Field* f, int n) {
        std::vector<size_t> off(static_cast<size_t>(n));
        size_t sz = layout_compute(f, n, off.data());
        size_t max_align = 1;
        for (int i = 0; i < n; ++i) if (f[i].align > max_align) max_align = f[i].align;
        EXPECT_EQ(sz % max_align, 0u) << "sizeof not multiple of max_align=" << max_align;
    };

    check(tc0, 3);
    check(tc1, 2);
    check(tc2, 4);
    check(tc3, 1);
}

// --- Свойство: каждое смещение кратно своему align ---------------------

TEST(LayoutComputeProps, EachOffsetIsAligned) {
    Field fields[4] = {
        make_field("a", 1, 1), make_field("b", 4, 4),
        make_field("c", 2, 2), make_field("d", 8, 8),
    };
    size_t offsets[4];
    layout_compute(fields, 4, offsets);

    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(offsets[i] % fields[i].align, 0u)
            << "field " << i << " offset=" << offsets[i]
            << " not aligned to " << fields[i].align;
    }
}

// --- Свойство: смещения строго не убывают, нет перекрытий --------------

TEST(LayoutComputeProps, OffsetsNonDecreasingNoOverlap) {
    Field fields[5] = {
        make_field("a", 1, 1), make_field("b", 8, 8),
        make_field("c", 4, 4), make_field("d", 2, 2),
        make_field("e", 1, 1),
    };
    size_t offsets[5];
    size_t sz = layout_compute(fields, 5, offsets);

    for (int i = 1; i < 5; ++i) {
        // offset[i] >= offset[i-1] + size[i-1]  (no overlap)
        EXPECT_GE(offsets[i], offsets[i-1] + fields[i-1].size)
            << "overlap between fields " << i-1 << " and " << i;
    }
    // last field end <= sizeof
    EXPECT_LE(offsets[4] + fields[4].size, sz);
}

// --- Рандомизированный тест против оракула (deterministic seed) --------

TEST(LayoutComputeRandom, MatchesOracleOnRandomLayouts) {
    // Oracle: same algorithm, independently coded using modulo arithmetic.
    auto oracle_layout = [](const Field* f, int n, size_t* off_out) -> size_t {
        size_t cursor = 0, max_a = 1;
        for (int i = 0; i < n; i++) {
            size_t a = f[i].align;
            if (a > max_a) max_a = a;
            // align up using division (deliberately different bit arithmetic)
            if (cursor % a != 0) cursor += a - (cursor % a);
            off_out[i] = cursor;
            cursor += f[i].size;
        }
        if (cursor % max_a != 0) cursor += max_a - (cursor % max_a);
        return cursor;
    };

    std::mt19937 rng(0xC0FFEE + 42);
    // Allowed alignments: 1, 2, 4, 8
    size_t aligns[] = {1, 2, 4, 8};
    std::uniform_int_distribution<int>    dist_n(1, 6);
    std::uniform_int_distribution<size_t> dist_a(0, 3);
    std::uniform_int_distribution<size_t> dist_s(1, 8);

    for (int trial = 0; trial < 500; ++trial) {
        int n = dist_n(rng);
        std::vector<Field> fields(static_cast<size_t>(n));
        for (int i = 0; i < n; i++) {
            size_t a = aligns[dist_a(rng)];
            fields[static_cast<size_t>(i)] = make_field("x", dist_s(rng), a);
        }
        std::vector<size_t> got(static_cast<size_t>(n));
        std::vector<size_t> exp(static_cast<size_t>(n));

        size_t sz_got = layout_compute(fields.data(), n, got.data());
        size_t sz_exp = oracle_layout(fields.data(), n, exp.data());

        EXPECT_EQ(sz_got, sz_exp) << "trial=" << trial << " sizeof mismatch";
        for (int i = 0; i < n; i++) {
            EXPECT_EQ(got[static_cast<size_t>(i)], exp[static_cast<size_t>(i)])
                << "trial=" << trial << " field " << i << " offset mismatch";
        }
    }
}
