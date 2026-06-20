// Эти тесты трогать не нужно — это эталон поведения.
// Они на C++ (GoogleTest), но вызывают твои C-функции через extern "C".
#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <climits>

extern "C" {
#include "paging.h"
}

// ---------- page_number: какой странице принадлежит адрес ----------
// page_size — степень двойки; номер страницы = addr / page_size.

TEST(PageNumber, FirstPageIsZero) {
    // Любой адрес внутри первой страницы (0..page_size-1) даёт номер 0.
    EXPECT_EQ(page_number(0, 4096), 0u);
    EXPECT_EQ(page_number(1, 4096), 0u);
    EXPECT_EQ(page_number(4095, 4096), 0u);  // последний байт страницы 0
}

TEST(PageNumber, CrossingTheBoundary) {
    // Ровно на границе начинается следующая страница.
    EXPECT_EQ(page_number(4096, 4096), 1u);  // первый байт страницы 1
    EXPECT_EQ(page_number(8195, 4096), 2u);  // 8195 = 2*4096 + 3
}

TEST(PageNumber, OtherPageSizes) {
    // Работает для любой степени двойки, не только 4096.
    EXPECT_EQ(page_number(1234, 256), 4u);   // 1234 = 4*256 + 210
    EXPECT_EQ(page_number(256, 256), 1u);
    EXPECT_EQ(page_number(255, 256), 0u);
    EXPECT_EQ(page_number(1024, 1024), 1u);
}

// ---------- page_offset: смещение внутри страницы ----------
// offset = addr % page_size, всегда в [0, page_size-1].

TEST(PageOffset, BasicOffsets) {
    EXPECT_EQ(page_offset(0, 4096), 0u);
    EXPECT_EQ(page_offset(1, 4096), 1u);
    EXPECT_EQ(page_offset(4095, 4096), 4095u);  // последний байт страницы
    EXPECT_EQ(page_offset(4096, 4096), 0u);     // снова с нуля на новой странице
}

TEST(PageOffset, OtherPageSizes) {
    EXPECT_EQ(page_offset(8195, 4096), 3u);    // 8195 = 2*4096 + 3
    EXPECT_EQ(page_offset(1234, 256), 210u);   // 1234 = 4*256 + 210
    EXPECT_EQ(page_offset(1023, 1024), 1023u);
    EXPECT_EQ(page_offset(1024, 1024), 0u);
}

TEST(PageNumberOffset, ReconstructAddress) {
    // Тождество: addr == page_number*page_size + page_offset.
    const size_t ps = 4096;
    const size_t addr = 43981;  // 0xABCD
    EXPECT_EQ(page_number(addr, ps) * ps + page_offset(addr, ps), addr);
    EXPECT_EQ(page_number(addr, ps), 10u);
    EXPECT_EQ(page_offset(addr, ps), 3021u);
}

// ---------- FIFO: число страничных промахов ----------
// Память пуста вначале -> первые загрузки тоже считаются (холодные промахи).

TEST(FifoPageFaults, AllUniqueAllCold) {
    // Все обращения к разным страницам -> каждое промах (5 холодных).
    const int refs[] = {5, 4, 3, 2, 1};
    EXPECT_EQ(fifo_page_faults(refs, 5, 3), 5);
}

TEST(FifoPageFaults, AllSameOnlyOneCold) {
    // Одна страница, к ней обращаемся 4 раза: 1 холодный промах, дальше попадания.
    const int refs[] = {9, 9, 9, 9};
    EXPECT_EQ(fifo_page_faults(refs, 4, 2), 1);
}

TEST(FifoPageFaults, FramesCoverWorkingSet) {
    // Кадров хватает на все различные страницы (3 шт.) -> только холодные промахи (3).
    const int refs[] = {1, 2, 3, 1, 2, 3};
    EXPECT_EQ(fifo_page_faults(refs, 6, 3), 3);
}

TEST(FifoPageFaults, ClassicBeladyThreeFrames) {
    // Хрестоматийная строка. При 3 кадрах FIFO даёт 9 промахов.
    const int refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    EXPECT_EQ(fifo_page_faults(refs, 12, 3), 9);
}

TEST(FifoPageFaults, BeladyAnomalyFourFrames) {
    // Та же строка при 4 кадрах даёт 10 промахов — БОЛЬШЕ, чем при 3!
    // Это «аномалия Белади»: у FIFO добавка кадров иногда УХУДШАЕТ результат.
    const int refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    EXPECT_EQ(fifo_page_faults(refs, 12, 4), 10);
}

TEST(FifoPageFaults, TextbookString) {
    // Классическая строка из учебников ОС, 3 кадра -> 10 промахов у FIFO.
    const int refs[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};
    EXPECT_EQ(fifo_page_faults(refs, 13, 3), 10);
}

// ---------- LRU: число страничных промахов ----------

TEST(LruPageFaults, AllUniqueAllCold) {
    const int refs[] = {5, 4, 3, 2, 1};
    EXPECT_EQ(lru_page_faults(refs, 5, 3), 5);
}

TEST(LruPageFaults, AllSameOnlyOneCold) {
    const int refs[] = {9, 9, 9, 9};
    EXPECT_EQ(lru_page_faults(refs, 4, 2), 1);
}

TEST(LruPageFaults, FramesCoverWorkingSet) {
    const int refs[] = {1, 2, 3, 1, 2, 3};
    EXPECT_EQ(lru_page_faults(refs, 6, 3), 3);
}

TEST(LruPageFaults, TextbookStringBeatsFifo) {
    // Та же учебная строка, 3 кадра: LRU даёт 9 промахов (против 10 у FIFO).
    // LRU «помнит свежесть» и реже выгоняет нужное — обычно он не хуже FIFO.
    const int refs[] = {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2};
    EXPECT_EQ(lru_page_faults(refs, 13, 3), 9);
}

TEST(LruPageFaults, NoAnomalyMoreFramesNotWorse) {
    // На строке Белади у LRU при 4 кадрах 8 промахов (при 3 их 10) — стало ЛУЧШЕ.
    // У LRU аномалии Белади не бывает: больше кадров — промахов не больше.
    const int refs[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    EXPECT_EQ(lru_page_faults(refs, 12, 3), 10);
    EXPECT_EQ(lru_page_faults(refs, 12, 4), 8);
}

TEST(LruVsFifo, CyclicWorstCaseTies) {
    // Циклический проход по 4 страницам при 3 кадрах — «пробуксовка» (thrashing):
    // каждая страница выгоняется ровно перед тем, как снова понадобится.
    // Здесь и FIFO, и LRU промахивают на КАЖДОМ обращении (12 из 12).
    const int refs[] = {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4};
    EXPECT_EQ(fifo_page_faults(refs, 12, 3), 12);
    EXPECT_EQ(lru_page_faults(refs, 12, 3), 12);
}

// ============================================================
// RANDOMIZED / PROPERTY TESTS
// ============================================================

// ----------------------------------------------------------
// Helper: naive (brute-force) FIFO oracle
// ----------------------------------------------------------
static int fifo_oracle(const std::vector<int>& refs, int frames) {
    std::vector<int> mem;   // loaded pages in load order
    int faults = 0;
    int next_evict = 0;     // circular pointer into mem[]
    for (int p : refs) {
        auto it = std::find(mem.begin(), mem.end(), p);
        if (it != mem.end()) continue;  // hit
        ++faults;
        if ((int)mem.size() < frames) {
            mem.push_back(p);
        } else {
            mem[next_evict] = p;
            next_evict = (next_evict + 1) % frames;
        }
    }
    return faults;
}

// ----------------------------------------------------------
// Helper: naive (brute-force) LRU oracle
// ----------------------------------------------------------
static int lru_oracle(const std::vector<int>& refs, int frames) {
    // mem[i] = {page, last_use_time}
    std::vector<std::pair<int,int>> mem;
    int faults = 0;
    for (int t = 0; t < (int)refs.size(); ++t) {
        int p = refs[t];
        // Check hit
        bool hit = false;
        for (auto& slot : mem) {
            if (slot.first == p) { slot.second = t; hit = true; break; }
        }
        if (hit) continue;
        ++faults;
        if ((int)mem.size() < frames) {
            mem.push_back({p, t});
        } else {
            // evict LRU: minimum last_use_time
            int evict_idx = 0;
            for (int i = 1; i < (int)mem.size(); ++i)
                if (mem[i].second < mem[evict_idx].second) evict_idx = i;
            mem[evict_idx] = {p, t};
        }
    }
    return faults;
}

// ----------------------------------------------------------
// page_number + page_offset: round-trip identity
//   page_number(addr, ps) * ps + page_offset(addr, ps) == addr
// ----------------------------------------------------------
TEST(PageNumberRandom, RoundTripIdentity) {
    std::mt19937 rng(0xC0FFEE);
    // page sizes: powers of two from 64 to 65536
    const size_t page_sizes[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
    std::uniform_int_distribution<size_t> addr_dist(0, (size_t)1 << 24);

    for (int iter = 0; iter < 1000; ++iter) {
        size_t ps = page_sizes[rng() % 11];
        size_t addr = addr_dist(rng);
        size_t pn = page_number(addr, ps);
        size_t po = page_offset(addr, ps);
        ASSERT_EQ(pn * ps + po, addr)
            << "round-trip failed: addr=" << addr << " ps=" << ps;
    }
}

// ----------------------------------------------------------
// page_offset: always in [0, page_size-1]
// ----------------------------------------------------------
TEST(PageOffsetRandom, AlwaysInRange) {
    std::mt19937 rng(0xC0FFEE + 1);
    const size_t page_sizes[] = {64, 128, 256, 512, 1024, 2048, 4096, 8192, 65536};
    std::uniform_int_distribution<size_t> addr_dist(0, (size_t)1 << 25);

    for (int iter = 0; iter < 1000; ++iter) {
        size_t ps = page_sizes[rng() % 9];
        size_t addr = addr_dist(rng);
        size_t off = page_offset(addr, ps);
        ASSERT_LT(off, ps)
            << "offset out of range: addr=" << addr << " ps=" << ps << " off=" << off;
    }
}

// ----------------------------------------------------------
// page_number: addresses within same page share a page number;
//              address at page boundary starts a new page.
// ----------------------------------------------------------
TEST(PageNumberRandom, SamePageSameNumber) {
    std::mt19937 rng(0xDEAD);
    const size_t page_sizes[] = {256, 512, 1024, 4096, 8192};
    std::uniform_int_distribution<size_t> addr_dist(0, (size_t)1 << 22);

    for (int iter = 0; iter < 500; ++iter) {
        size_t ps = page_sizes[rng() % 5];
        size_t addr = addr_dist(rng);
        // all addresses in [page_start, page_start + ps - 1] share the same page number
        size_t page_start = (addr / ps) * ps;
        ASSERT_EQ(page_number(page_start, ps), page_number(page_start + ps - 1, ps))
            << "start and end of same page have different page numbers";
        // next page starts a new number
        ASSERT_EQ(page_number(page_start + ps, ps), page_number(page_start, ps) + 1)
            << "page boundary does not increment page number";
    }
}

// ----------------------------------------------------------
// page_number: monotone — larger address in same/later page
// ----------------------------------------------------------
TEST(PageNumberRandom, MonotoneLargerAddress) {
    std::mt19937 rng(0xBEEF);
    const size_t page_sizes[] = {256, 1024, 4096};
    std::uniform_int_distribution<size_t> addr_dist(0, (size_t)1 << 20);

    for (int iter = 0; iter < 500; ++iter) {
        size_t ps = page_sizes[rng() % 3];
        size_t a = addr_dist(rng);
        size_t b = addr_dist(rng);
        if (a > b) std::swap(a, b);
        ASSERT_LE(page_number(a, ps), page_number(b, ps))
            << "page_number not monotone: a=" << a << " b=" << b << " ps=" << ps;
    }
}

// ----------------------------------------------------------
// FIFO oracle agreement: function matches brute-force reference
// ----------------------------------------------------------
TEST(FifoRandom, MatchesOracleSmallWorkingSet) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> page_dist(0, 7);   // 8 distinct pages
    std::uniform_int_distribution<int> frames_dist(1, 4);

    for (int iter = 0; iter < 800; ++iter) {
        int n = 12 + (int)(rng() % 9);  // length 12..20
        int frames = frames_dist(rng);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        int got = fifo_page_faults(refs.data(), n, frames);
        int exp = fifo_oracle(refs, frames);
        ASSERT_EQ(got, exp)
            << "FIFO mismatch: frames=" << frames << " refs=[";
    }
}

TEST(FifoRandom, MatchesOracleLargerWorkingSet) {
    std::mt19937 rng(0xFEEDBEEF);
    std::uniform_int_distribution<int> page_dist(0, 15);
    std::uniform_int_distribution<int> frames_dist(1, 6);

    for (int iter = 0; iter < 500; ++iter) {
        int n = 20 + (int)(rng() % 31);  // length 20..50
        int frames = frames_dist(rng);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        int got = fifo_page_faults(refs.data(), n, frames);
        int exp = fifo_oracle(refs, frames);
        ASSERT_EQ(got, exp)
            << "FIFO large mismatch: frames=" << frames << " n=" << n;
    }
}

// ----------------------------------------------------------
// LRU oracle agreement
// ----------------------------------------------------------
TEST(LruRandom, MatchesOracleSmallWorkingSet) {
    std::mt19937 rng(0xC0FFEE + 42);
    std::uniform_int_distribution<int> page_dist(0, 7);
    std::uniform_int_distribution<int> frames_dist(1, 4);

    for (int iter = 0; iter < 800; ++iter) {
        int n = 12 + (int)(rng() % 9);
        int frames = frames_dist(rng);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        int got = lru_page_faults(refs.data(), n, frames);
        int exp = lru_oracle(refs, frames);
        ASSERT_EQ(got, exp)
            << "LRU mismatch: frames=" << frames;
    }
}

TEST(LruRandom, MatchesOracleLargerWorkingSet) {
    std::mt19937 rng(0xABCDEF01);
    std::uniform_int_distribution<int> page_dist(0, 15);
    std::uniform_int_distribution<int> frames_dist(1, 6);

    for (int iter = 0; iter < 500; ++iter) {
        int n = 20 + (int)(rng() % 31);
        int frames = frames_dist(rng);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        int got = lru_page_faults(refs.data(), n, frames);
        int exp = lru_oracle(refs, frames);
        ASSERT_EQ(got, exp)
            << "LRU large mismatch: frames=" << frames << " n=" << n;
    }
}

// ----------------------------------------------------------
// Invariant: faults in [distinct_pages, n]
//   - lower bound: must fault at least once for each distinct page seen
//     (cold miss, unavoidable)
//   - upper bound: cannot fault more than n times (one per ref)
// ----------------------------------------------------------
TEST(FaultBounds, FifoFaultsInRange) {
    std::mt19937 rng(0x12345678);
    std::uniform_int_distribution<int> page_dist(0, 9);
    std::uniform_int_distribution<int> frames_dist(1, 4);

    for (int iter = 0; iter < 600; ++iter) {
        int n = 10 + (int)(rng() % 21);
        int frames = frames_dist(rng);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        std::unordered_set<int> distinct(refs.begin(), refs.end());
        int cold = (int)distinct.size();

        int got = fifo_page_faults(refs.data(), n, frames);
        ASSERT_GE(got, std::min(cold, n))
            << "FIFO below cold-miss lower bound";
        ASSERT_LE(got, n)
            << "FIFO above total-refs upper bound";
    }
}

TEST(FaultBounds, LruFaultsInRange) {
    std::mt19937 rng(0x87654321);
    std::uniform_int_distribution<int> page_dist(0, 9);
    std::uniform_int_distribution<int> frames_dist(1, 4);

    for (int iter = 0; iter < 600; ++iter) {
        int n = 10 + (int)(rng() % 21);
        int frames = frames_dist(rng);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        std::unordered_set<int> distinct(refs.begin(), refs.end());
        int cold = (int)distinct.size();

        int got = lru_page_faults(refs.data(), n, frames);
        ASSERT_GE(got, std::min(cold, n))
            << "LRU below cold-miss lower bound";
        ASSERT_LE(got, n)
            << "LRU above total-refs upper bound";
    }
}

// ----------------------------------------------------------
// Monotone frames: more frames => faults never increase for LRU
// (LRU has the stack property — no Belady anomaly)
// ----------------------------------------------------------
TEST(LruRandom, MoreFramesNeverWorse) {
    std::mt19937 rng(0x9999CAFE);
    std::uniform_int_distribution<int> page_dist(0, 9);

    for (int iter = 0; iter < 400; ++iter) {
        int n = 12 + (int)(rng() % 13);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        // For any two frame counts f1 < f2, LRU(f1) >= LRU(f2)
        for (int f1 = 1; f1 <= 4; ++f1) {
            int faults_f1 = lru_page_faults(refs.data(), n, f1);
            int faults_f2 = lru_page_faults(refs.data(), n, f1 + 1);
            ASSERT_GE(faults_f1, faults_f2)
                << "LRU anomaly: more frames gave more faults: f1=" << f1;
        }
    }
}

// ----------------------------------------------------------
// Frames >= working set => only cold misses for both FIFO and LRU
// ----------------------------------------------------------
TEST(FaultBounds, EnoughFramesOnlyColdMisses) {
    std::mt19937 rng(0xCAFEBABE);
    std::uniform_int_distribution<int> page_dist(0, 7);

    for (int iter = 0; iter < 400; ++iter) {
        int n = 8 + (int)(rng() % 13);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        std::unordered_set<int> distinct(refs.begin(), refs.end());
        int cold = (int)distinct.size();
        int frames = cold;  // exactly enough to hold entire working set

        int fifo_f = fifo_page_faults(refs.data(), n, frames);
        int lru_f  = lru_page_faults(refs.data(), n, frames);

        ASSERT_EQ(fifo_f, cold)
            << "FIFO: frames>=working_set but got extra faults";
        ASSERT_EQ(lru_f, cold)
            << "LRU: frames>=working_set but got extra faults";
    }
}

// ----------------------------------------------------------
// Single distinct page: exactly 1 fault regardless of length or frames
// ----------------------------------------------------------
TEST(FaultEdgeCases, SingleDistinctPage) {
    std::mt19937 rng(0x111);
    for (int iter = 0; iter < 300; ++iter) {
        int n = 1 + (int)(rng() % 50);
        int frames = 1 + (int)(rng() % 5);
        int page = (int)(rng() % 100);
        std::vector<int> refs(n, page);

        ASSERT_EQ(fifo_page_faults(refs.data(), n, frames), 1)
            << "FIFO single distinct page: expected 1 fault";
        ASSERT_EQ(lru_page_faults(refs.data(), n, frames), 1)
            << "LRU single distinct page: expected 1 fault";
    }
}

// ----------------------------------------------------------
// frames = 1: every change of page is a fault
// ----------------------------------------------------------
TEST(FaultEdgeCases, OneFrame) {
    std::mt19937 rng(0x222);
    std::uniform_int_distribution<int> page_dist(0, 4);

    for (int iter = 0; iter < 400; ++iter) {
        int n = 5 + (int)(rng() % 16);
        std::vector<int> refs(n);
        for (int& r : refs) r = page_dist(rng);

        // With 1 frame: fault on ref[0], then fault whenever ref[i] != ref[i-1]
        int expected = 1;
        for (int i = 1; i < n; ++i)
            if (refs[i] != refs[i-1]) ++expected;

        ASSERT_EQ(fifo_page_faults(refs.data(), n, 1), expected)
            << "FIFO 1-frame mismatch";
        ASSERT_EQ(lru_page_faults(refs.data(), n, 1), expected)
            << "LRU 1-frame mismatch";
    }
}

// ----------------------------------------------------------
// All unique refs: every ref is a cold miss (faults == n)
// ----------------------------------------------------------
TEST(FaultEdgeCases, AllUniqueFaultsEqualsN) {
    std::mt19937 rng(0x333);

    for (int iter = 0; iter < 300; ++iter) {
        int n = 4 + (int)(rng() % 17);
        int frames = 1 + (int)(rng() % (n < 4 ? n : 4));
        // Build a sequence of n distinct pages
        std::vector<int> refs(n);
        std::iota(refs.begin(), refs.end(), 0);
        std::shuffle(refs.begin(), refs.end(), rng);

        ASSERT_EQ(fifo_page_faults(refs.data(), n, frames), n)
            << "FIFO all-unique: expected n faults";
        ASSERT_EQ(lru_page_faults(refs.data(), n, frames), n)
            << "LRU all-unique: expected n faults";
    }
}

// ----------------------------------------------------------
// page_number / page_offset edge cases
// ----------------------------------------------------------
TEST(PageEdgeCases, AddressZero) {
    const size_t page_sizes[] = {64, 256, 1024, 4096};
    for (size_t ps : page_sizes) {
        EXPECT_EQ(page_number(0, ps), 0u);
        EXPECT_EQ(page_offset(0, ps), 0u);
    }
}

TEST(PageEdgeCases, ExactlyAtPageBoundary) {
    // addr == ps: page 1, offset 0
    const size_t page_sizes[] = {64, 256, 1024, 4096};
    for (size_t ps : page_sizes) {
        EXPECT_EQ(page_number(ps, ps), 1u);
        EXPECT_EQ(page_offset(ps, ps), 0u);
        // one before boundary: page 0, offset ps-1
        EXPECT_EQ(page_number(ps - 1, ps), 0u);
        EXPECT_EQ(page_offset(ps - 1, ps), ps - 1);
    }
}

TEST(PageEdgeCases, LargeAddressSmallPage) {
    // addr = 0xFFFFFF, ps = 64: page = addr/64, offset = addr%64
    size_t addr = 0xFFFFFF;
    size_t ps   = 64;
    EXPECT_EQ(page_number(addr, ps), addr / ps);
    EXPECT_EQ(page_offset(addr, ps), addr % ps);
}

TEST(PageEdgeCases, MinimalPageSize) {
    // page_size = 1 (degenerate power of two): every address is its own page
    // The module guarantees page_size is a power of two >= 1
    // page_number(addr, 1) == addr, page_offset(addr, 1) == 0
    for (size_t addr = 0; addr < 1000; ++addr) {
        EXPECT_EQ(page_number(addr, 1), addr);
        EXPECT_EQ(page_offset(addr, 1), 0u);
    }
}

// ============================================================
//  Two-level page table simulator: pt_create / pt_map / pt_walk
// ============================================================
//
// vaddr = [ L1: 10 bits | L2: 10 bits | offset: 12 bits ].
// pt_map(v, p) stores the FRAME of p for the page of v.
// pt_walk(v, 0) reconstructs (frame << 12) | (v & 0xFFF), or -1 if unmapped.
// pt_walk(v, 1) lazily allocates a fresh frame if the page is unmapped.

// Helpers that mirror the address layout the simulator must use.
static uint32_t mk_vaddr(uint32_t l1, uint32_t l2, uint32_t off) {
    return (l1 << 22) | (l2 << 12) | off;
}

TEST(PageTableWalk, MapThenWalkReturnsMappedFrame) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    // Map virtual page (l1=1, l2=2) onto physical frame 5 (paddr 5*4096).
    uint32_t v = mk_vaddr(1, 2, 0);
    uint32_t p = 5u << 12;  // frame 5, offset 0
    ASSERT_EQ(pt_map(pt, v, p), 0);

    // walk without alloc must find it: physical = frame5 | offset.
    EXPECT_EQ(pt_walk(pt, v, 0), (int64_t)(5u << 12));
    // Offset within the page is carried through unchanged.
    EXPECT_EQ(pt_walk(pt, v | 0x123, 0), (int64_t)((5u << 12) | 0x123));

    pt_destroy(pt);
}

TEST(PageTableWalk, UnmappedReturnsMinusOne) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    // Nothing mapped yet: any walk without alloc is a "page fault" -> -1.
    EXPECT_EQ(pt_walk(pt, 0x00000000u, 0), -1);
    EXPECT_EQ(pt_walk(pt, 0xDEADBEEFu, 0), -1);

    // Map ONE page; a *different* page in the same L2 table is still unmapped.
    uint32_t v = mk_vaddr(3, 7, 0);
    ASSERT_EQ(pt_map(pt, v, 9u << 12), 0);
    EXPECT_EQ(pt_walk(pt, v, 0), (int64_t)(9u << 12));         // mapped
    EXPECT_EQ(pt_walk(pt, mk_vaddr(3, 8, 0), 0), -1);          // neighbour in same L2
    EXPECT_EQ(pt_walk(pt, mk_vaddr(4, 7, 0), 0), -1);          // different L1 entry

    pt_destroy(pt);
}

TEST(PageTableWalk, OffsetPreservedAcrossTranslation) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    uint32_t base = mk_vaddr(10, 20, 0);
    ASSERT_EQ(pt_map(pt, base, 42u << 12), 0);  // page -> frame 42

    // Every byte inside the page keeps its 12-bit offset, only the frame swaps.
    for (uint32_t off = 0; off < PT_PAGE_SIZE; off += 257) {
        int64_t got = pt_walk(pt, base | off, 0);
        EXPECT_EQ(got, (int64_t)((42u << 12) | off)) << "offset " << off;
    }

    pt_destroy(pt);
}

TEST(PageTableWalk, DistinctPagesMapIndependently) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    // Three different virtual pages -> three different frames; no interference.
    struct { uint32_t l1, l2, frame; } cases[] = {
        {0,    0,    100},
        {0,    1,    200},   // same L2 table as above, neighbouring slot
        {1023, 1023, 300},   // top of the address space, separate L2 table
    };
    for (auto &c : cases) {
        ASSERT_EQ(pt_map(pt, mk_vaddr(c.l1, c.l2, 0), c.frame << 12), 0);
    }
    for (auto &c : cases) {
        EXPECT_EQ(pt_walk(pt, mk_vaddr(c.l1, c.l2, 0), 0),
                  (int64_t)(c.frame << 12))
            << "l1=" << c.l1 << " l2=" << c.l2;
    }

    pt_destroy(pt);
}

TEST(PageTableWalk, AllocCreatesFreshFrameAndIsStable) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    uint32_t v = mk_vaddr(2, 5, 0);
    // First touch with alloc: a frame is assigned (not -1).
    int64_t first = pt_walk(pt, v, 1);
    ASSERT_NE(first, -1);
    EXPECT_EQ(first & (PT_PAGE_SIZE - 1), 0);  // offset 0 stays 0

    // The mapping must now be stable: walking again (even without alloc)
    // returns the SAME physical address.
    EXPECT_EQ(pt_walk(pt, v, 0), first);
    EXPECT_EQ(pt_walk(pt, v, 1), first);

    // The same frame, accessed at offset 0x40, just adds the offset.
    EXPECT_EQ(pt_walk(pt, v | 0x40, 0), first | 0x40);

    pt_destroy(pt);
}

TEST(PageTableWalk, AllocGivesDistinctFramesToDistinctPages) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    // Touch several distinct virtual pages with alloc; each must get its own
    // physical frame (distinct frame numbers -> distinct page-aligned paddrs).
    const uint32_t vaddrs[] = {
        mk_vaddr(0, 0, 0),
        mk_vaddr(0, 1, 0),
        mk_vaddr(5, 5, 0),
        mk_vaddr(1023, 0, 0),
    };
    std::unordered_set<int64_t> frames;
    for (uint32_t v : vaddrs) {
        int64_t pa = pt_walk(pt, v, 1);
        ASSERT_NE(pa, -1);
        frames.insert(pa >> PT_PAGE_BITS);   // collect frame numbers
    }
    EXPECT_EQ(frames.size(), 4u) << "alloc handed out a frame twice";

    pt_destroy(pt);
}

TEST(PageTableWalk, MapIgnoresPhysicalOffset) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    // pt_map maps PAGES, not bytes: the low 12 bits of paddr are irrelevant.
    uint32_t v = mk_vaddr(7, 7, 0);
    ASSERT_EQ(pt_map(pt, v, (8u << 12) | 0xABC), 0);  // frame 8, junk offset

    // Resulting frame is 8 regardless of the offset bits passed to pt_map.
    EXPECT_EQ(pt_walk(pt, v, 0), (int64_t)(8u << 12));
    EXPECT_EQ(pt_walk(pt, v | 0x0FF, 0), (int64_t)((8u << 12) | 0x0FF));

    pt_destroy(pt);
}

TEST(PageTableWalk, RemapOverwritesMapping) {
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    uint32_t v = mk_vaddr(4, 4, 0);
    ASSERT_EQ(pt_map(pt, v, 11u << 12), 0);
    EXPECT_EQ(pt_walk(pt, v, 0), (int64_t)(11u << 12));

    // Mapping the same page again replaces the frame.
    ASSERT_EQ(pt_map(pt, v, 22u << 12), 0);
    EXPECT_EQ(pt_walk(pt, v, 0), (int64_t)(22u << 12));

    pt_destroy(pt);
}

// Randomized cross-check: a model std::map<page, frame> must agree with the
// simulator for an arbitrary stream of maps and walks. Deterministic seed.
TEST(PageTableRandom, MatchesModelMap) {
    std::mt19937 rng(0x9A12B3);
    PageTable *pt = pt_create();
    ASSERT_NE(pt, nullptr);

    std::unordered_map<uint32_t, uint32_t> model;  // virtual page -> frame
    std::uniform_int_distribution<uint32_t> l1d(0, PT_ENTRIES - 1);
    std::uniform_int_distribution<uint32_t> l2d(0, PT_ENTRIES - 1);
    std::uniform_int_distribution<uint32_t> framed(0, 100000);
    std::uniform_int_distribution<uint32_t> offd(0, PT_PAGE_SIZE - 1);

    for (int i = 0; i < 4000; ++i) {
        uint32_t l1 = l1d(rng), l2 = l2d(rng), off = offd(rng);
        uint32_t vpage = (l1 << 10) | l2;            // identity of the page
        uint32_t v = mk_vaddr(l1, l2, off);

        if ((rng() & 1u) == 0u) {
            // map operation
            uint32_t frame = framed(rng);
            ASSERT_EQ(pt_map(pt, v, frame << 12), 0);
            model[vpage] = frame;
        } else {
            // walk without alloc: must match the model exactly
            int64_t got = pt_walk(pt, v, 0);
            auto it = model.find(vpage);
            if (it == model.end()) {
                ASSERT_EQ(got, -1) << "expected unmapped at iter " << i;
            } else {
                ASSERT_EQ(got, (int64_t)(((uint64_t)it->second << 12) | off))
                    << "translation mismatch at iter " << i;
            }
        }
    }

    pt_destroy(pt);
}
