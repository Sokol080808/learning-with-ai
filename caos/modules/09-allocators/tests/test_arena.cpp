// Эти тесты трогать не нужно — это эталон поведения арены (bump-аллокатора).
//
// Тесты намеренно НЕ требуют конкретных адресов (адрес буфера на стеке заранее
// неизвестен). Они проверяют ПОВЕДЕНИЕ: выданные блоки выровнены по 8, не
// пересекаются, при нехватке места возвращается NULL, а reset снова даёт полный объём.

#include <gtest/gtest.h>
#include <algorithm> // std::shuffle (детерминированная перетасовка в heap-тестах)
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

// ── heap_* : настоящий аллокатор с освобождением блоков ──────────────────────
// Эти тесты проверяют ПОВЕДЕНИЕ heap_init / heap_malloc / heap_free поверх
// неявного списка (implicit free list): выравнивание payload по 8, отсутствие
// перекрытий, переиспользование освобождённой памяти и слияние соседей.
//
// Все тесты сначала зовут heap_init(), чтобы стартовать с чистой кучи —
// глобальный буфер общий между тестами. heap_* трогаются ТОЛЬКО в рантайме,
// поэтому файл компилируется и против заглушки.

namespace {

// «Указатель payload выровнен по 8?» — то же, что aligned8, отдельное имя
// для читаемости heap-тестов.
bool heap_aligned8(const void *p) {
    return (reinterpret_cast<std::uintptr_t>(p) % 8u) == 0u;
}

// Записать узнаваемый паттерн во весь блок (проверка, что память пригодна и не
// пересекается с чужой).
void fill_pattern(void *p, size_t n, unsigned char pat) {
    std::memset(p, pat, n);
}
bool check_pattern(const void *p, size_t n, unsigned char pat) {
    const unsigned char *b = static_cast<const unsigned char *>(p);
    for (size_t i = 0; i < n; ++i) {
        if (b[i] != pat) return false;
    }
    return true;
}

}  // namespace

// Базовая выдача: непустой указатель, выровнен по 8, в него можно писать.
TEST(Heap, SingleMallocAlignedAndUsable) {
    heap_init();
    void *p = heap_malloc(40);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(heap_aligned8(p));
    fill_pattern(p, 40, 0x5A);
    EXPECT_TRUE(check_pattern(p, 40, 0x5A));
    heap_free(p);
}

// Любой выданный payload выровнен по 8, даже при «кривых» размерах запроса.
TEST(Heap, EveryPayloadAligned) {
    heap_init();
    const size_t sizes[] = {1, 3, 7, 8, 9, 17, 31, 33, 64, 100};
    std::vector<void *> ptrs;
    for (size_t n : sizes) {
        void *p = heap_malloc(n);
        ASSERT_NE(p, nullptr) << "NULL for n=" << n;
        EXPECT_TRUE(heap_aligned8(p)) << "payload for n=" << n << " not aligned by 8";
        ptrs.push_back(p);
    }
    for (void *p : ptrs) heap_free(p);
}

// Несколько живых блоков не пересекаются: интервалы [p, p+n) попарно не
// перекрываются. Заодно пишем разные паттерны и проверяем их целостность.
TEST(Heap, LiveBlocksDoNotOverlap) {
    heap_init();
    struct B { unsigned char *p; size_t n; unsigned char pat; };
    std::vector<B> bs;
    const size_t sizes[] = {16, 24, 40, 8, 72, 100, 33};
    unsigned char pat = 1;
    for (size_t n : sizes) {
        auto *p = static_cast<unsigned char *>(heap_malloc(n));
        ASSERT_NE(p, nullptr) << "NULL for n=" << n;
        fill_pattern(p, n, pat);
        bs.push_back({p, n, pat});
        ++pat;
    }
    // Попарная проверка непересечения интервалов.
    for (size_t i = 0; i < bs.size(); ++i) {
        for (size_t j = i + 1; j < bs.size(); ++j) {
            auto ai = reinterpret_cast<std::uintptr_t>(bs[i].p);
            auto aj = reinterpret_cast<std::uintptr_t>(bs[j].p);
            bool disjoint = (ai + bs[i].n <= aj) || (aj + bs[j].n <= ai);
            EXPECT_TRUE(disjoint) << "blocks " << i << " and " << j << " overlap";
        }
    }
    // Все паттерны целы — никто никого не затёр.
    for (auto &b : bs) {
        EXPECT_TRUE(check_pattern(b.p, b.n, b.pat)) << "block clobbered";
    }
    for (auto &b : bs) heap_free(b.p);
}

// Освобождённую память можно переиспользовать: после free того же размера запрос
// снова проходит (а не упирается в потолок). Гоняем много циклов alloc/free
// одного размера — куча не должна «протекать».
TEST(Heap, FreeEnablesReuse) {
    heap_init();
    void *first = heap_malloc(1000);
    ASSERT_NE(first, nullptr);
    heap_free(first);

    // 10000 раз выделить и сразу освободить 1000 байт — если бы free не
    // возвращал память, куча 64 KiB кончилась бы за ~64 итерации.
    for (int i = 0; i < 10000; ++i) {
        void *p = heap_malloc(1000);
        ASSERT_NE(p, nullptr) << "reuse failed at iteration " << i;
        heap_free(p);
    }
}

// Слияние соседей (coalescing): три смежных блока, освобождаем все три, после
// чего должен помещаться запрос крупнее любого по отдельности.
TEST(Heap, CoalesceAdjacentFreesAllowsBigAlloc) {
    heap_init();
    // Три блока по 5000 байт подряд.
    void *a = heap_malloc(5000);
    void *b = heap_malloc(5000);
    void *c = heap_malloc(5000);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);

    // Освобождаем в «неудобном» порядке: края, потом середину. Если слияние
    // работает, три дырки по ~5000 сольются в один блок ~15000.
    heap_free(a);
    heap_free(c);
    heap_free(b);

    // Запрос 14000 больше любого отдельного блока (5000), но влезет в слитую
    // область. Без coalescing это вернуло бы NULL.
    void *big = heap_malloc(14000);
    EXPECT_NE(big, nullptr) << "coalescing failed: 14000 did not fit after freeing 3x5000";
    heap_free(big);
}

// Слияние с ЛЕВЫМ соседом по футеру: выделяем A,B; освобождаем A, затем B.
// При free(B) блок должен слиться с уже свободным A слева (boundary tag).
TEST(Heap, CoalesceWithLeftNeighbor) {
    heap_init();
    void *a = heap_malloc(6000);
    void *b = heap_malloc(6000);
    void *tail = heap_malloc(8);   // барьер, чтобы B не слился вправо со всем хвостом
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(tail, nullptr);

    heap_free(a);   // A свободен
    heap_free(b);   // B освобождаем — должен прилипнуть к A слева

    // Слитый блок A+B вмещает ~12000; запрос 11000 пройти обязан.
    void *merged = heap_malloc(11000);
    EXPECT_NE(merged, nullptr) << "left-coalescing failed: 11000 did not fit into A+B";
    heap_free(merged);
    heap_free(tail);
}

// heap_malloc(0): валидный ненулевой указатель, безопасный для heap_free.
TEST(Heap, ZeroMallocReturnsFreeablePointer) {
    heap_init();
    void *p = heap_malloc(0);
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(heap_aligned8(p));
    heap_free(p);  // не должно падать
}

// heap_free(NULL) — пустая операция, ничего не ломает.
TEST(Heap, FreeNullIsNoop) {
    heap_init();
    heap_free(nullptr);  // не падать
    void *p = heap_malloc(16);
    EXPECT_NE(p, nullptr);
    heap_free(p);
}

// Запрос больше всей кучи — NULL, и куча остаётся работоспособной.
TEST(Heap, TooBigReturnsNull) {
    heap_init();
    EXPECT_EQ(heap_malloc(64 * 1024 + 1), nullptr);
    // Куча не испортилась — обычная выдача работает.
    void *p = heap_malloc(64);
    EXPECT_NE(p, nullptr);
    heap_free(p);
}

// ── heap_* рандомизированные тесты (seed фиксирован) ─────────────────────────

// Любой указатель из heap_malloc выровнен по 8 при случайных размерах.
TEST(Heap, RandomAlignmentInvariant) {
    heap_init();
    std::mt19937 rng(0x9A11C0DE);
    std::uniform_int_distribution<size_t> dist(1, 200);

    std::vector<void *> live;
    for (int i = 0; i < 400; ++i) {
        size_t n = dist(rng);
        void *p = heap_malloc(n);
        if (!p) break;
        EXPECT_TRUE(heap_aligned8(p)) << "unaligned payload at i=" << i << " n=" << n;
        live.push_back(p);
    }
    for (void *p : live) heap_free(p);
}

// Случайный поток alloc/free: ни один живой блок не должен быть затёрт чужим
// (каждому присваиваем уникальный байтовый паттерн и периодически проверяем).
TEST(Heap, RandomNoOverlapAndIntegrity) {
    heap_init();
    std::mt19937 rng(0x1234ABCD);
    std::uniform_int_distribution<size_t> size_dist(1, 300);
    std::uniform_int_distribution<int> coin(0, 1);

    struct Live { unsigned char *p; size_t n; unsigned char pat; };
    std::vector<Live> live;
    unsigned char next_pat = 1;

    for (int step = 0; step < 4000; ++step) {
        // С вероятностью ~1/2 освобождаем случайный живой блок, иначе выделяем.
        if (!live.empty() && coin(rng)) {
            size_t idx = rng() % live.size();
            // Перед освобождением убеждаемся, что блок ещё цел.
            EXPECT_TRUE(check_pattern(live[idx].p, live[idx].n, live[idx].pat))
                << "block corrupted before free at step " << step;
            heap_free(live[idx].p);
            live[idx] = live.back();
            live.pop_back();
        } else {
            size_t n = size_dist(rng);
            auto *p = static_cast<unsigned char *>(heap_malloc(n));
            if (!p) continue;  // куча временно заполнена — ок
            unsigned char pat = next_pat ? next_pat : 1;
            ++next_pat;
            fill_pattern(p, n, pat);
            live.push_back({p, n, pat});
        }
        // Каждые 200 шагов проверяем целостность ВСЕХ живых блоков.
        if (step % 200 == 0) {
            for (auto &b : live) {
                EXPECT_TRUE(check_pattern(b.p, b.n, b.pat))
                    << "live block corrupted at step " << step;
            }
        }
    }
    for (auto &b : live) heap_free(b.p);
}

// После полного освобождения всех блоков coalescing должен восстановить кучу
// почти целиком: запрос, близкий к полному объёму, снова проходит.
TEST(Heap, FullFreeRestoresLargeCapacity) {
    heap_init();
    std::mt19937 rng(0x0FF1CE55);
    std::uniform_int_distribution<size_t> dist(16, 256);

    std::vector<void *> live;
    // Набиваем кучу множеством мелких блоков.
    for (int i = 0; i < 5000; ++i) {
        void *p = heap_malloc(dist(rng));
        if (!p) break;
        live.push_back(p);
    }
    ASSERT_GE(live.size(), 50u) << "too few allocs to be a meaningful test";

    // Освобождаем в перемешанном (но детерминированном) порядке.
    std::shuffle(live.begin(), live.end(), rng);
    for (void *p : live) heap_free(p);

    // Теперь вся куча должна слиться обратно: крупный запрос (60 KiB) проходит.
    void *big = heap_malloc(60 * 1024);
    EXPECT_NE(big, nullptr)
        << "after freeing everything, 60 KiB alloc failed — coalescing incomplete";
    heap_free(big);
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
