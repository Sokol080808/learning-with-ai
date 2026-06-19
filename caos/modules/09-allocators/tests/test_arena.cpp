// Эти тесты трогать не нужно — это эталон поведения арены (bump-аллокатора).
//
// Тесты намеренно НЕ требуют конкретных адресов (адрес буфера на стеке заранее
// неизвестен). Они проверяют ПОВЕДЕНИЕ: выданные блоки выровнены по 8, не
// пересекаются, при нехватке места возвращается NULL, а reset снова даёт полный объём.

#include <gtest/gtest.h>
#include <cstdint>   // std::uintptr_t
#include <cstring>   // std::memset

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
