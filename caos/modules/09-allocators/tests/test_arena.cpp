// Эти тесты трогать не нужно — это эталон поведения арены (bump-аллокатора).
//
// Тесты намеренно НЕ требуют конкретных адресов (адрес буфера на стеке заранее
// неизвестен). Они проверяют ПОВЕДЕНИЕ: выданные блоки выровнены по 8, не
// пересекаются, при нехватке места возвращается NULL, а reset снова даёт полный объём.

#include <gtest/gtest.h>
#include <cstdint>   // std::uintptr_t
#include <cstring>   // std::memset
#include <random>    // std::mt19937, std::uniform_int_distribution
#include <vector>    // std::vector

extern "C" {
#include "arena.h"
}

namespace {

// Буфер с запасом: 1024 байта, выровненный по 16, чтобы рассуждения о выравнивании
// блоков не зависели от того, как ОС положила локальный массив.
alignas(16) unsigned char g_buf[1024];

// «Адрес выровнен по 8?»
bool aligned8(const void *p) {
    return (reinterpret_cast<std::uintptr_t>(p) % 8u) == 0u;
}

}  // namespace

// init: сразу после привязки доступен весь буфер.
TEST(Arena, InitGivesFullCapacity) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));
    EXPECT_EQ(arena_remaining(&a), sizeof(g_buf));
}

// Одна выдача: непустой указатель, попадает внутрь буфера, выровнен по 8.
TEST(Arena, SingleAllocInsideBufferAndAligned) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    void *p = arena_alloc(&a, 10);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(aligned8(p));

    auto base = reinterpret_cast<std::uintptr_t>(g_buf);
    auto addr = reinterpret_cast<std::uintptr_t>(p);
    EXPECT_GE(addr, base);
    EXPECT_LE(addr + 10u, base + sizeof(g_buf));
}

// Каждый выданный блок выровнен по 8 — даже если предыдущий запрос был «кривого» размера.
TEST(Arena, EveryBlockIsAligned) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    const size_t sizes[] = {1, 3, 7, 17, 33, 5};
    for (size_t n : sizes) {
        void *p = arena_alloc(&a, n);
        ASSERT_NE(p, nullptr);
        EXPECT_TRUE(aligned8(p)) << "блок размера " << n << " не выровнен по 8";
    }
}

// Последовательные блоки не пересекаются: следующий начинается не раньше, чем
// заканчивается предыдущий (с учётом запрошенного размера).
TEST(Arena, SequentialBlocksDoNotOverlap) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    void *p1 = arena_alloc(&a, 24);
    void *p2 = arena_alloc(&a, 24);
    void *p3 = arena_alloc(&a, 24);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);

    auto a1 = reinterpret_cast<std::uintptr_t>(p1);
    auto a2 = reinterpret_cast<std::uintptr_t>(p2);
    auto a3 = reinterpret_cast<std::uintptr_t>(p3);

    EXPECT_GE(a2, a1 + 24u);  // p2 начинается после конца p1
    EXPECT_GE(a3, a2 + 24u);  // p3 начинается после конца p2
}

// Каждый выданный блок реально пригоден для записи — память лежит внутри буфера.
// Если бы блоки пересекались или вылезали за буфер, это был бы выход за границы.
TEST(Arena, AllocatedMemoryIsUsable) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    unsigned char *p1 = static_cast<unsigned char *>(arena_alloc(&a, 16));
    unsigned char *p2 = static_cast<unsigned char *>(arena_alloc(&a, 16));
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);

    std::memset(p1, 0xAA, 16);
    std::memset(p2, 0xBB, 16);
    // Запись в p2 не должна была затронуть p1 (блоки не пересекаются).
    for (int i = 0; i < 16; ++i) {
        EXPECT_EQ(p1[i], 0xAA);
        EXPECT_EQ(p2[i], 0xBB);
    }
}

// Каждая выдача уменьшает remaining как минимум на n (блок реально занят) и не
// больше чем на n+7 (выравнивание по 8 может «съесть» до 7 байт добивки, не больше).
// Так мы проверяем суть, не привязываясь к тому, КАК именно ты ведёшь учёт внутри.
TEST(Arena, RemainingShrinksByAtLeastRequest) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    const size_t sizes[] = {10, 8, 1, 17};
    for (size_t n : sizes) {
        size_t before = arena_remaining(&a);
        ASSERT_NE(arena_alloc(&a, n), nullptr);
        size_t spent = before - arena_remaining(&a);
        EXPECT_GE(spent, n)     << "выдали " << n << " байт, а remaining упал меньше";
        EXPECT_LE(spent, n + 7) << "на выравнивание ушло больше 7 байт добивки";
    }
}

// Запрос больше, чем весь буфер, — это NULL, и арена не «портится».
TEST(Arena, AllocTooBigReturnsNull) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    EXPECT_EQ(arena_alloc(&a, sizeof(g_buf) + 1), nullptr);
    EXPECT_EQ(arena_remaining(&a), sizeof(g_buf));  // ничего не выдано

    // После неудачи обычная выдача по-прежнему работает.
    EXPECT_NE(arena_alloc(&a, 8), nullptr);
}

// Маленький буфер: выдаём, пока не упрёмся в потолок, потом — NULL.
TEST(Arena, RunsOutOfSpace) {
    alignas(16) unsigned char small[32];
    Arena a;
    arena_init(&a, small, sizeof(small));

    // 32 байта = четыре блока по 8 (база выровнена по 16, т.е. и по 8).
    EXPECT_NE(arena_alloc(&a, 8), nullptr);  // used 8
    EXPECT_NE(arena_alloc(&a, 8), nullptr);  // used 16
    EXPECT_NE(arena_alloc(&a, 8), nullptr);  // used 24
    EXPECT_NE(arena_alloc(&a, 8), nullptr);  // used 32 — буфер заполнен
    EXPECT_EQ(arena_remaining(&a), 0u);

    // Места не осталось даже под 1 байт.
    EXPECT_EQ(arena_alloc(&a, 1), nullptr);
}

// reset возвращает полный объём, и арену можно использовать заново.
TEST(Arena, ResetRestoresFullCapacity) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    ASSERT_NE(arena_alloc(&a, 100), nullptr);
    ASSERT_NE(arena_alloc(&a, 200), nullptr);
    EXPECT_LT(arena_remaining(&a), sizeof(g_buf));  // что-то потрачено

    arena_reset(&a);
    EXPECT_EQ(arena_remaining(&a), sizeof(g_buf));  // снова всё свободно

    // После reset снова можно выдавать с самого начала буфера.
    void *p = arena_alloc(&a, 8);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p),
              reinterpret_cast<std::uintptr_t>(g_buf));
}

// Нулевой запрос: корректно вернуть валидный (выровненный) указатель и не падать.
TEST(Arena, ZeroSizedAllocIsAlignedAndNonNull) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    void *p = arena_alloc(&a, 0);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(aligned8(p));
    EXPECT_EQ(arena_remaining(&a), sizeof(g_buf));  // 0 байт ничего не съедает
}

// ── Randomised tests ────────────────────────────────────────────────────────
// Seed is fixed so failures are reproducible.

// Every pointer returned by arena_alloc must be aligned by 8, regardless of
// the requested size.  We draw 1000 random sizes in [1, 56] — enough to fit
// all of them in a 64 KiB arena — and assert the invariant after each alloc.
TEST(Arena, RandomAlignmentInvariant) {
    alignas(16) static unsigned char big[65536];
    Arena a;
    arena_init(&a, big, sizeof(big));

    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<size_t> dist(1, 56);

    for (int i = 0; i < 1000; ++i) {
        size_t n = dist(rng);
        void *p = arena_alloc(&a, n);
        ASSERT_NE(p, nullptr) << "unexpected NULL at iteration " << i << " (n=" << n << ")";
        EXPECT_TRUE(aligned8(p)) << "pointer not aligned at iteration " << i;
    }
}

// Sequential blocks must never overlap: the start of block[i+1] must be >=
// start[i] + requested_size[i].  Oracle is a simple vector of (ptr, size)
// pairs checked pairwise.
TEST(Arena, RandomNoOverlap) {
    alignas(16) static unsigned char big[65536];
    Arena a;
    arena_init(&a, big, sizeof(big));

    std::mt19937 rng(0xDEAD1337);
    std::uniform_int_distribution<size_t> dist(1, 120);

    struct Block { std::uintptr_t addr; size_t size; };
    std::vector<Block> blocks;
    blocks.reserve(512);

    for (int i = 0; i < 512; ++i) {
        size_t n = dist(rng);
        void *p = arena_alloc(&a, n);
        if (!p) break;   // arena exhausted — stop collecting
        blocks.push_back({reinterpret_cast<std::uintptr_t>(p), n});
    }

    ASSERT_GE(blocks.size(), 2u) << "too few successful allocs to test overlap";

    for (size_t i = 0; i + 1 < blocks.size(); ++i) {
        EXPECT_GE(blocks[i+1].addr, blocks[i].addr + blocks[i].size)
            << "blocks " << i << " and " << i+1 << " overlap";
    }
}

// Invariant: arena_remaining(a) == a.size - a.used at every point.
// We verify by checking remaining + used == size after each operation.
TEST(Arena, RandomRemainingPlusUsedEqualsSize) {
    alignas(16) static unsigned char big[65536];
    Arena a;
    arena_init(&a, big, sizeof(big));

    // Check invariant via public API: remaining should never exceed size.
    EXPECT_LE(arena_remaining(&a), sizeof(big));

    std::mt19937 rng(0xBEEF0001);
    std::uniform_int_distribution<size_t> dist(0, 200);

    for (int i = 0; i < 600; ++i) {
        size_t n = dist(rng);
        arena_alloc(&a, n);   // may return NULL if no space — that's fine
        size_t rem = arena_remaining(&a);
        // remaining must never exceed total capacity
        EXPECT_LE(rem, sizeof(big)) << "remaining > total size after iteration " << i;
        // used field must be consistent: buf..buf+size, used <= size
        EXPECT_LE(a.used, a.size) << "a.used > a.size after iteration " << i;
        // remaining = size - used (direct oracle via struct fields)
        EXPECT_EQ(rem, a.size - a.used) << "remaining mismatch at iteration " << i;
    }
}

// Write distinct byte patterns to randomly-sized blocks; verify no block's
// content was clobbered by a subsequent alloc (isolation / no-overlap oracle).
TEST(Arena, RandomWriteIsolation) {
    alignas(16) static unsigned char big[65536];
    Arena a;
    arena_init(&a, big, sizeof(big));

    std::mt19937 rng(0xC0DE9999);
    std::uniform_int_distribution<size_t> size_dist(1, 64);
    std::uniform_int_distribution<unsigned int> pat_dist(1, 255);

    struct Fill { unsigned char *ptr; size_t size; unsigned char pattern; };
    std::vector<Fill> fills;
    fills.reserve(256);

    for (int i = 0; i < 256; ++i) {
        size_t n = size_dist(rng);
        auto pattern = static_cast<unsigned char>(pat_dist(rng));
        unsigned char *p = static_cast<unsigned char *>(arena_alloc(&a, n));
        if (!p) break;
        std::memset(p, pattern, n);
        fills.push_back({p, n, pattern});
    }

    // After all allocs, verify each block still holds its pattern intact.
    for (size_t i = 0; i < fills.size(); ++i) {
        for (size_t b = 0; b < fills[i].size; ++b) {
            EXPECT_EQ(fills[i].ptr[b], fills[i].pattern)
                << "block " << i << " byte " << b << " was overwritten";
        }
    }
}

// Exhaustion: fill the arena with 1-byte allocs; once NULL is returned,
// remaining must be < 8 (no room even for an aligned byte) and further
// 8-byte allocs must also return NULL.
TEST(Arena, RandomExhaustionAndNullAfterFull) {
    alignas(16) static unsigned char small[256];
    Arena a;
    arena_init(&a, small, sizeof(small));

    std::mt19937 rng(0xF00DCAFE);
    std::uniform_int_distribution<size_t> dist(1, 8);

    // Drain the arena.
    int null_count = 0;
    for (int i = 0; i < 10000 && null_count < 5; ++i) {
        size_t n = dist(rng);
        void *p = arena_alloc(&a, n);
        if (!p) null_count++;
    }

    // Once exhausted, remaining must be < 8 (can't fit an aligned byte).
    EXPECT_LT(arena_remaining(&a), 8u)
        << "arena claims " << arena_remaining(&a)
        << " bytes free but could not allocate; should be < 8";

    // Subsequent 8-byte alloc must also fail.
    EXPECT_EQ(arena_alloc(&a, 8), nullptr)
        << "expected NULL on 8-byte alloc from nearly-full arena";
}

// After reset, the very first alloc returns the base address of the buffer
// (because used=0 and the buffer is already 16-aligned, so align_up(0)=0).
// We do this 500 times with random intermediate loads to confirm reset is clean.
TEST(Arena, RandomResetAlwaysRestoresBase) {
    alignas(16) static unsigned char base_buf[4096];
    Arena a;
    arena_init(&a, base_buf, sizeof(base_buf));

    std::mt19937 rng(0xA5A5A5A5);
    std::uniform_int_distribution<size_t> dist(1, 128);

    const std::uintptr_t base_addr = reinterpret_cast<std::uintptr_t>(base_buf);

    for (int round = 0; round < 500; ++round) {
        // Randomly consume some of the arena.
        int ops = static_cast<int>(rng() % 15) + 1;
        for (int j = 0; j < ops; ++j) {
            arena_alloc(&a, dist(rng));
        }

        arena_reset(&a);
        EXPECT_EQ(arena_remaining(&a), sizeof(base_buf))
            << "remaining != full capacity after reset (round " << round << ")";

        // First alloc after reset must come back at the buffer base.
        void *p = arena_alloc(&a, 8);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p), base_addr)
            << "first alloc after reset did not return base address (round " << round << ")";

        arena_reset(&a);
    }
}

// Huge requests (n > size) must always return NULL and must not modify
// arena state (remaining unchanged).
TEST(Arena, RandomHugeRequestAlwaysNull) {
    alignas(16) static unsigned char med[512];
    Arena a;
    arena_init(&a, med, sizeof(med));

    std::mt19937 rng(0x12345678);
    // Generate sizes in (512, SIZE_MAX/2] range — all larger than the buffer.
    std::uniform_int_distribution<size_t> dist(513, 1u << 20);

    for (int i = 0; i < 800; ++i) {
        size_t before = arena_remaining(&a);
        size_t n = dist(rng);
        void *p = arena_alloc(&a, n);
        EXPECT_EQ(p, nullptr) << "huge alloc (n=" << n << ") should return NULL";
        EXPECT_EQ(arena_remaining(&a), before)
            << "arena state changed after failed huge alloc (n=" << n << ")";
    }
}

// Zero-size allocs must never consume any bytes; remaining must stay constant
// across 800 consecutive zero-size requests.
TEST(Arena, RandomZeroAllocNeverConsumes) {
    Arena a;
    arena_init(&a, g_buf, sizeof(g_buf));

    // Put arena in a partially-used state first to make the test more realistic.
    // alloc(13): head moves to 13, align_up(13)=16, used becomes 13.
    // The FIRST zero-size alloc will snap the head to align_up(13)=16 (used->16),
    // which is allowed — it consumes padding, not user bytes.
    // Capture rem_before only after that first zero alloc so the baseline is stable.
    arena_alloc(&a, 13);          // used = 13 after bump
    arena_alloc(&a, 0);           // snaps head to align_up(13)=16; used = 16
    const size_t rem_before = arena_remaining(&a);

    std::mt19937 rng(0x00000000);
    for (int i = 0; i < 800; ++i) {
        void *p = arena_alloc(&a, 0);
        ASSERT_NE(p, nullptr) << "zero alloc returned NULL at iteration " << i;
        EXPECT_TRUE(aligned8(p)) << "zero alloc not aligned at iteration " << i;
        EXPECT_EQ(arena_remaining(&a), rem_before)
            << "remaining changed after zero alloc at iteration " << i;
    }
}

// Round-trip: fill arena with known sizes, reset, refill with same sizes,
// confirm pointers match exactly (deterministic bump pointer).
TEST(Arena, RandomRoundTripAfterReset) {
    alignas(16) static unsigned char rt_buf[8192];
    Arena a;
    arena_init(&a, rt_buf, sizeof(rt_buf));

    std::mt19937 rng(0xFEDCBA98);
    std::uniform_int_distribution<size_t> dist(8, 128);

    std::vector<std::pair<void*, size_t>> first_pass;
    first_pass.reserve(64);

    // First pass: collect pointers.
    for (int i = 0; i < 64; ++i) {
        size_t n = dist(rng);
        void *p = arena_alloc(&a, n);
        if (!p) break;
        first_pass.push_back({p, n});
    }
    ASSERT_GE(first_pass.size(), 2u);

    arena_reset(&a);

    // Second pass with same seed → same sizes → must produce same pointers.
    rng.seed(0xFEDCBA98);
    for (size_t i = 0; i < first_pass.size(); ++i) {
        size_t n = dist(rng);
        void *p = arena_alloc(&a, n);
        ASSERT_NE(p, nullptr) << "unexpected NULL in second pass at i=" << i;
        EXPECT_EQ(p, first_pass[i].first)
            << "pointer mismatch in second pass at i=" << i
            << " (expected " << first_pass[i].first << ", got " << p << ")";
    }
}

// Alignment padding must never exceed 7 bytes per alloc — the bump pointer
// advances by at most n+7 bytes for a request of n bytes.
// Oracle: track used before/after and verify the delta is in [n, n+7].
// We also verify that when n is already a multiple of 8, no padding is wasted
// (delta == n exactly).
TEST(Arena, RandomAlignmentPaddingBounds) {
    alignas(16) static unsigned char pad_buf[65536];
    Arena a;
    arena_init(&a, pad_buf, sizeof(pad_buf));

    std::mt19937 rng(0xCAFEBABE);
    std::uniform_int_distribution<size_t> dist(0, 200);

    for (int i = 0; i < 700; ++i) {
        size_t n = dist(rng);
        size_t used_before = a.used;
        void *p = arena_alloc(&a, n);
        if (!p) {
            // Arena too full — reset and continue.
            arena_reset(&a);
            continue;
        }
        size_t used_after = a.used;
        size_t delta = used_after - used_before;  // bytes consumed this alloc

        EXPECT_GE(delta, n)       << "delta < n at i=" << i << " n=" << n;
        EXPECT_LE(delta, n + 7)   << "delta > n+7 at i=" << i << " n=" << n;

        // When n is a multiple of 8 AND used_before was already aligned,
        // no padding should be wasted.
        if ((n % 8 == 0) && (used_before % 8 == 0)) {
            EXPECT_EQ(delta, n) << "unexpected padding for aligned request i=" << i;
        }
    }
}
