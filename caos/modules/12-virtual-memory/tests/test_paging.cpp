// Эти тесты трогать не нужно — это эталон поведения.
// Они на C++ (GoogleTest), но вызывают твои C-функции через extern "C".
#include <gtest/gtest.h>

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
