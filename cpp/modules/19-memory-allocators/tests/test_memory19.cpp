#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <vector>
#include "memory19.hpp"

using namespace m19;

// ── helpers ──────────────────────────────────────────────────────────────────
static bool is_aligned(const void* p, std::size_t a) {
    return (reinterpret_cast<std::uintptr_t>(p) % a) == 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Задание 1. BumpAllocator
// ─────────────────────────────────────────────────────────────────────────────
TEST(BumpAllocator, SequentialAllocationsMoveCursor) {
    alignas(std::max_align_t) std::byte buf[256];
    BumpAllocator a(buf, sizeof(buf));
    EXPECT_EQ(a.used(), 0u);
    EXPECT_EQ(a.capacity(), 256u);

    void* p1 = a.allocate(16, 8);
    void* p2 = a.allocate(16, 8);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    // Второй блок идёт ровно за первым (16 байт, выравнивание уже кратно 8).
    EXPECT_EQ(static_cast<std::byte*>(p2) - static_cast<std::byte*>(p1), 16);
    EXPECT_EQ(a.used(), 32u);
}

TEST(BumpAllocator, RespectsAlignment) {
    alignas(std::max_align_t) std::byte buf[256];
    BumpAllocator a(buf, sizeof(buf));
    a.allocate(1, 1);            // курсор теперь на смещении 1 (нечётный)
    void* p = a.allocate(8, 16); // должен быть выровнен вверх до кратного 16
    ASSERT_NE(p, nullptr);
    EXPECT_TRUE(is_aligned(p, 16));
}

TEST(BumpAllocator, ResetReleasesEverything) {
    alignas(std::max_align_t) std::byte buf[64];
    BumpAllocator a(buf, sizeof(buf));
    void* first = a.allocate(16, 8);
    a.allocate(16, 8);
    EXPECT_EQ(a.used(), 32u);
    a.reset();
    EXPECT_EQ(a.used(), 0u);
    void* again = a.allocate(16, 8);
    // После reset снова выдаётся самый первый адрес буфера.
    EXPECT_EQ(again, first);
}

TEST(BumpAllocator, ThrowsBadAllocWhenFull) {
    alignas(std::max_align_t) std::byte buf[32];
    BumpAllocator a(buf, sizeof(buf));
    a.allocate(32, 1);                         // занять весь буфер
    EXPECT_THROW(a.allocate(1, 1), std::bad_alloc);
}

// ─────────────────────────────────────────────────────────────────────────────
// Задание 2. UniquePtr
// ─────────────────────────────────────────────────────────────────────────────
TEST(UniquePtr, OwnsAndDereferences) {
    UniquePtr<int> p(new int(42));
    ASSERT_TRUE(static_cast<bool>(p));
    EXPECT_EQ(*p, 42);
    *p = 7;
    EXPECT_EQ(*p, 7);
}

TEST(UniquePtr, ArrowOperator) {
    struct Point { int x, y; };
    UniquePtr<Point> p(new Point{3, 4});
    EXPECT_TRUE(static_cast<bool>(p));   // владеющий указатель «истинный»
    EXPECT_EQ(p->x, 3);
    EXPECT_EQ(p->y, 4);
}

TEST(UniquePtr, DefaultIsEmptyAndBecomesOwning) {
    UniquePtr<int> p;
    EXPECT_FALSE(static_cast<bool>(p));   // пустой
    EXPECT_EQ(p.get(), nullptr);
    p.reset(new int(8));                  // теперь владеет объектом
    EXPECT_TRUE(static_cast<bool>(p));
    ASSERT_NE(p.get(), nullptr);
    EXPECT_EQ(*p, 8);
}

TEST(UniquePtr, MoveTransfersOwnership) {
    UniquePtr<int> a(new int(1));
    int* raw = a.get();
    UniquePtr<int> b(std::move(a));
    EXPECT_EQ(a.get(), nullptr);
    ASSERT_EQ(b.get(), raw);   // владение должно перейти к b
    EXPECT_EQ(*b, 1);
}

TEST(UniquePtr, MoveAssignReleasesOld) {
    UniquePtr<int> a(new int(1));
    UniquePtr<int> b(new int(2));
    b = std::move(a);
    EXPECT_EQ(a.get(), nullptr);
    ASSERT_TRUE(static_cast<bool>(b));
    EXPECT_EQ(*b, 1);
}

TEST(UniquePtr, ReleaseGivesUpOwnership) {
    UniquePtr<int> a(new int(5));
    int* raw = a.release();
    EXPECT_EQ(a.get(), nullptr);
    ASSERT_NE(raw, nullptr);
    EXPECT_EQ(*raw, 5);
    delete raw;  // владение теперь у нас
}

TEST(UniquePtr, ResetReplacesObject) {
    UniquePtr<int> a(new int(5));
    a.reset(new int(9));
    ASSERT_TRUE(static_cast<bool>(a));
    EXPECT_EQ(*a, 9);
    a.reset();
    EXPECT_EQ(a.get(), nullptr);
}

TEST(UniquePtr, DestructorRunsObjectDestructor) {
    static int alive = 0;
    struct Tracked { Tracked() { ++alive; } ~Tracked() { --alive; } };
    alive = 0;
    {
        UniquePtr<Tracked> p(new Tracked());
        EXPECT_EQ(alive, 1);
    }
    EXPECT_EQ(alive, 0);  // деструктор должен был удалить объект
}

TEST(UniquePtr, MakeUnique) {
    auto p = make_unique19<std::string>("hi");
    ASSERT_TRUE(static_cast<bool>(p));
    EXPECT_EQ(*p, "hi");
}

// ─────────────────────────────────────────────────────────────────────────────
// Задание 3. SharedPtr / WeakPtr
// ─────────────────────────────────────────────────────────────────────────────
TEST(SharedPtr, BasicUseCount) {
    SharedPtr<int> a(new int(10));
    ASSERT_EQ(a.use_count(), 1);     // контрол-блок должен существовать, strong==1
    ASSERT_NE(a.get(), nullptr);
    EXPECT_EQ(*a, 10);
    {
        SharedPtr<int> b = a;       // копия
        EXPECT_EQ(a.use_count(), 2);
        EXPECT_EQ(b.use_count(), 2);
        EXPECT_EQ(b.get(), a.get());
    }
    EXPECT_EQ(a.use_count(), 1);     // b разрушился
}

TEST(SharedPtr, DefaultEmpty) {
    SharedPtr<int> a;
    EXPECT_EQ(a.use_count(), 0);
    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_EQ(a.get(), nullptr);
}

TEST(SharedPtr, MoveDoesNotBumpCount) {
    SharedPtr<int> a(new int(1));
    int* raw = a.get();
    SharedPtr<int> b(std::move(a));
    EXPECT_EQ(b.use_count(), 1);
    EXPECT_EQ(b.get(), raw);
    EXPECT_EQ(a.get(), nullptr);
}

TEST(SharedPtr, CopyAssignAdjustsCounts) {
    SharedPtr<int> a(new int(1));
    SharedPtr<int> b(new int(2));
    b = a;
    EXPECT_EQ(a.use_count(), 2);
    EXPECT_EQ(b.use_count(), 2);
    ASSERT_EQ(b.get(), a.get());   // после b=a оба указывают на один объект
    EXPECT_EQ(*b, 1);
}

TEST(SharedPtr, DestroysObjectAtZero) {
    static int alive = 0;
    struct Tracked { Tracked() { ++alive; } ~Tracked() { --alive; } };
    alive = 0;
    {
        SharedPtr<Tracked> a(new Tracked());
        EXPECT_EQ(alive, 1);
        {
            SharedPtr<Tracked> b = a;
            EXPECT_EQ(alive, 1);   // ещё жив, две сильные ссылки
        }
        EXPECT_EQ(alive, 1);       // одна сильная ссылка ушла, объект жив
    }
    EXPECT_EQ(alive, 0);           // последняя ссылка ушла — объект разрушен
}

TEST(WeakPtr, LockWhileAlive) {
    SharedPtr<int> a(new int(77));
    WeakPtr<int> w(a);
    EXPECT_FALSE(w.expired());
    SharedPtr<int> locked = w.lock();
    ASSERT_TRUE(static_cast<bool>(locked));
    EXPECT_EQ(*locked, 77);
    EXPECT_EQ(a.use_count(), 2);   // lock дал ещё одну сильную ссылку
}

TEST(WeakPtr, ExpiresAfterStrongGone) {
    WeakPtr<int> w;
    {
        SharedPtr<int> a(new int(1));
        w = WeakPtr<int>(a);
        EXPECT_FALSE(w.expired());
    }
    EXPECT_TRUE(w.expired());       // strong-ссылок не осталось
    SharedPtr<int> locked = w.lock();
    EXPECT_FALSE(static_cast<bool>(locked));  // lock мёртвого — пустой
}

TEST(WeakPtr, DoesNotKeepObjectAlive) {
    static int alive = 0;
    struct Tracked { Tracked() { ++alive; } ~Tracked() { --alive; } };
    alive = 0;
    WeakPtr<Tracked> w;
    {
        SharedPtr<Tracked> a(new Tracked());
        w = WeakPtr<Tracked>(a);
        EXPECT_EQ(alive, 1);
    }
    // Несмотря на живой weak, объект должен быть разрушен (weak не продлевает жизнь).
    EXPECT_EQ(alive, 0);
    EXPECT_TRUE(w.expired());
}

// ─────────────────────────────────────────────────────────────────────────────
// Задание 4. PoolAllocator
// ─────────────────────────────────────────────────────────────────────────────
TEST(PoolAllocator, AllocatesAllBlocksThenNull) {
    constexpr std::size_t BS = sizeof(void*) > 16 ? sizeof(void*) : 16;
    constexpr std::size_t N  = 4;
    alignas(std::max_align_t) std::byte buf[BS * N];
    PoolAllocator pool(buf, BS, N);
    EXPECT_EQ(pool.free_count(), N);
    EXPECT_EQ(pool.block_size(), BS);

    void* blocks[N];
    for (std::size_t i = 0; i < N; ++i) {
        blocks[i] = pool.allocate();
        ASSERT_NE(blocks[i], nullptr);
    }
    EXPECT_EQ(pool.free_count(), 0u);
    EXPECT_EQ(pool.allocate(), nullptr);  // пул исчерпан

    // Все выданные блоки различны.
    for (std::size_t i = 0; i < N; ++i)
        for (std::size_t j = i + 1; j < N; ++j)
            EXPECT_NE(blocks[i], blocks[j]);
}

TEST(PoolAllocator, DeallocateReturnsBlock) {
    constexpr std::size_t BS = 32;
    constexpr std::size_t N  = 3;
    alignas(std::max_align_t) std::byte buf[BS * N];
    PoolAllocator pool(buf, BS, N);

    void* a = pool.allocate();
    void* b = pool.allocate();
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(pool.free_count(), 1u);

    pool.deallocate(a);
    EXPECT_EQ(pool.free_count(), 2u);
    void* c = pool.allocate();   // снова можно выдать
    EXPECT_NE(c, nullptr);
    EXPECT_EQ(pool.free_count(), 1u);
}

TEST(PoolAllocator, BlocksLieInsideBuffer) {
    constexpr std::size_t BS = 16;
    constexpr std::size_t N  = 5;
    alignas(std::max_align_t) std::byte buf[BS * N];
    PoolAllocator pool(buf, BS, N);
    for (std::size_t i = 0; i < N; ++i) {
        std::byte* p = static_cast<std::byte*>(pool.allocate());
        ASSERT_NE(p, nullptr);
        EXPECT_GE(p, buf);
        EXPECT_LT(p, buf + sizeof(buf));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Задание 5. placement new / явный деструктор
// ─────────────────────────────────────────────────────────────────────────────
TEST(PlacementNew, ConstructsInRawBuffer) {
    alignas(std::string) std::byte buf[sizeof(std::string)];
    std::string* s = construct_at19<std::string>(buf, "hello world");
    ASSERT_NE(s, nullptr);
    // Объект построен прямо в нашем буфере.
    EXPECT_EQ(static_cast<void*>(s), static_cast<void*>(buf));
    EXPECT_EQ(*s, "hello world");
    destroy_at19(s);   // явный деструктор; память buf при этом не освобождается
}

TEST(PlacementNew, DestroyCallsDestructor) {
    static int alive = 0;
    struct Tracked { Tracked() { ++alive; } ~Tracked() { --alive; } };
    alive = 0;
    alignas(Tracked) std::byte buf[sizeof(Tracked)];
    Tracked* t = construct_at19<Tracked>(buf);
    EXPECT_EQ(alive, 1);
    destroy_at19(t);
    EXPECT_EQ(alive, 0);   // деструктор должен был отработать
}

TEST(PlacementNew, ForwardsConstructorArgs) {
    alignas(std::vector<int>) std::byte buf[sizeof(std::vector<int>)];
    auto* v = construct_at19<std::vector<int>>(buf, std::size_t{3}, 7);
    ASSERT_NE(v, nullptr);
    ASSERT_EQ(v->size(), 3u);
    EXPECT_EQ((*v)[0], 7);
    EXPECT_EQ((*v)[2], 7);
    destroy_at19(v);
}
