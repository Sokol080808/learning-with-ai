#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <vector>
#include <random>
#include <set>
#include <algorithm>
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

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Эти тесты не повторяют фиксированные примеры, а прогоняют сотни сгенерированных
// входов и проверяют ИНВАРИАНТЫ:
//   * BumpAllocator: выровненность, монотонность курсора, отсутствие наложений,
//     согласованность used() и эталонный (oracle) расчёт смещения/исчерпания.
//   * UniquePtr:    передача владения move'ом, round-trip release/reset, bool.
//   * SharedPtr/WeakPtr: use_count == числу живых владельцев, жизнь объекта,
//     weak не продлевает жизнь, lock как функция от expired().
//   * PoolAllocator: free_count + выданные == N, различимость и принадлежность
//     буферу, round-trip allocate/deallocate.
//   * placement new: round-trip значений в сыром буфере, вызов деструктора.
// Сид фиксирован (0xC0FFEE) — CI детерминирован, никогда не «мигает».

namespace {

// align — степень двойки. Эталонное (oracle) выравнивание адреса вверх.
inline std::uintptr_t align_up_oracle(std::uintptr_t v, std::uintptr_t align) {
    return (v + (align - 1)) & ~(align - 1);
}

}  // namespace

// ── Задание 1. BumpAllocator — property ──────────────────────────────────────

// Каждый выданный блок выровнен по запрошенному align, лежит внутри буфера,
// не накладывается на предыдущий, а курсор (used) монотонно неубывает.
TEST(BumpAllocatorProps, AlignedNonOverlappingInBoundsMonotone) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> align_pow(0, 6);   // 2^0..2^6 = 1..64
    std::uniform_int_distribution<std::size_t> size_dist(1, 24);

    for (int iter = 0; iter < 300; ++iter) {
        alignas(std::max_align_t) std::byte buf[512];
        BumpAllocator a(buf, sizeof(buf));

        std::byte* prev_end = nullptr;
        std::size_t prev_used = 0;
        for (int step = 0; step < 40; ++step) {
            const std::size_t align = std::size_t{1} << align_pow(rng);
            const std::size_t n     = size_dist(rng);
            void* p = nullptr;
            try {
                p = a.allocate(n, align);
            } catch (const std::bad_alloc&) {
                // Буфер исчерпан — used() при этом не должен был измениться.
                EXPECT_EQ(a.used(), prev_used);
                break;
            }
            ASSERT_NE(p, nullptr);
            std::byte* bp = static_cast<std::byte*>(p);
            // Выровнен по align.
            EXPECT_TRUE(is_aligned(p, align));
            // Внутри буфера, и блок целиком помещается.
            EXPECT_GE(bp, buf);
            EXPECT_LE(bp + n, buf + sizeof(buf));
            // Не накладывается на предыдущий блок (адреса не убывают).
            if (prev_end) EXPECT_GE(bp, prev_end);
            // used() монотонно растёт и согласован с положением блока.
            EXPECT_GE(a.used(), prev_used);
            EXPECT_EQ(a.used(), static_cast<std::size_t>(bp + n - buf));
            prev_end  = bp + n;
            prev_used = a.used();
        }
    }
}

// Эталон (oracle): зная курсор до allocate, можно ровно предсказать адрес
// выданного блока и итоговый used(); они должны совпадать с реализацией.
TEST(BumpAllocatorProps, MatchesOracleOffsetAndUsed) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> align_pow(0, 5);   // 1..32
    std::uniform_int_distribution<std::size_t> size_dist(1, 20);

    for (int iter = 0; iter < 300; ++iter) {
        alignas(std::max_align_t) std::byte buf[256];
        const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(buf);
        BumpAllocator a(buf, sizeof(buf));

        std::size_t cursor = 0;  // модель курсора как смещение от base
        for (int step = 0; step < 30; ++step) {
            const std::size_t align = std::size_t{1} << align_pow(rng);
            const std::size_t n     = size_dist(rng);
            const std::uintptr_t aligned = align_up_oracle(base + cursor, align);
            const std::size_t off = static_cast<std::size_t>(aligned - base);
            const bool fits = off <= sizeof(buf) && (sizeof(buf) - off) >= n;
            if (!fits) {
                EXPECT_THROW(a.allocate(n, align), std::bad_alloc);
                EXPECT_EQ(a.used(), cursor);  // неудача не сдвигает курсор
                continue;
            }
            void* p = a.allocate(n, align);
            // Адрес ровно по эталонному смещению.
            EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p), aligned);
            cursor = off + n;
            EXPECT_EQ(a.used(), cursor);
        }
    }
}

// reset() всегда обнуляет used() и снова выдаёт самый первый адрес буфера.
TEST(BumpAllocatorProps, ResetAlwaysRewindsToStart) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<std::size_t> size_dist(1, 30);
    std::uniform_int_distribution<int> reps(1, 6);

    alignas(std::max_align_t) std::byte buf[128];
    for (int iter = 0; iter < 250; ++iter) {
        BumpAllocator a(buf, sizeof(buf));
        const int k = reps(rng);
        for (int j = 0; j < k; ++j) {
            try { a.allocate(size_dist(rng), 1); } catch (const std::bad_alloc&) { break; }
        }
        a.reset();
        EXPECT_EQ(a.used(), 0u);
        EXPECT_EQ(a.capacity(), sizeof(buf));  // ёмкость инвариантна
        void* first = a.allocate(1, 1);
        EXPECT_EQ(first, static_cast<void*>(buf));  // снова с начала
    }
}

// Краевые случаи: n == 0 всегда успешно и не сдвигает курсор; запрос ровно на
// всю оставшуюся ёмкость проходит, а на байт больше — бросает bad_alloc.
TEST(BumpAllocatorProps, EdgeZeroAndExactCapacity) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<std::size_t> cap_dist(1, 200);
    for (int iter = 0; iter < 200; ++iter) {
        const std::size_t cap = cap_dist(rng);
        std::vector<std::byte> storage(cap);
        BumpAllocator a(storage.data(), cap);
        // n == 0 при align 1 — курсор на месте.
        a.allocate(0, 1);
        EXPECT_EQ(a.used(), 0u);
        // Ровно вся ёмкость влезает.
        EXPECT_NO_THROW(a.allocate(cap, 1));
        EXPECT_EQ(a.used(), cap);
        // Ещё один байт — уже нет.
        EXPECT_THROW(a.allocate(1, 1), std::bad_alloc);
    }
}

// ── Задание 2. UniquePtr — property ──────────────────────────────────────────

// Move-конструктор и move-присваивание ВСЕГДА передают тот же сырой указатель,
// источник становится пустым, значение сохраняется.
TEST(UniquePtrProps, MoveTransfersOwnershipExactly) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-100000, 100000);
    for (int iter = 0; iter < 400; ++iter) {
        const int v = val(rng);
        UniquePtr<int> a(new int(v));
        int* raw = a.get();
        ASSERT_NE(raw, nullptr);

        if (iter & 1) {
            UniquePtr<int> b(std::move(a));
            EXPECT_EQ(a.get(), nullptr);     // источник опустел
            EXPECT_FALSE(static_cast<bool>(a));
            ASSERT_EQ(b.get(), raw);         // ровно тот же указатель
            EXPECT_EQ(*b, v);                // и то же значение
        } else {
            UniquePtr<int> b;
            b = std::move(a);
            EXPECT_EQ(a.get(), nullptr);
            ASSERT_EQ(b.get(), raw);
            EXPECT_EQ(*b, v);
        }
    }
}

// release() round-trip: отдаёт сырой указатель, обнуляет себя; reset(p) удаляет
// старый и берёт новый; bool == (get()!=nullptr) во всех состояниях.
TEST(UniquePtrProps, ReleaseResetRoundTripAndBool) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-50000, 50000);
    for (int iter = 0; iter < 400; ++iter) {
        const int v1 = val(rng), v2 = val(rng);
        UniquePtr<int> p(new int(v1));
        EXPECT_TRUE(static_cast<bool>(p));
        EXPECT_EQ(static_cast<bool>(p), p.get() != nullptr);

        int* taken = p.release();
        EXPECT_EQ(p.get(), nullptr);
        EXPECT_FALSE(static_cast<bool>(p));
        ASSERT_NE(taken, nullptr);
        EXPECT_EQ(*taken, v1);

        // Возвращаем владение через reset — старого объекта у p нет, утечки нет.
        p.reset(taken);
        EXPECT_TRUE(static_cast<bool>(p));
        EXPECT_EQ(*p, v1);

        // reset на новый объект заменяет значение.
        p.reset(new int(v2));
        EXPECT_EQ(*p, v2);

        p.reset();
        EXPECT_EQ(p.get(), nullptr);
        EXPECT_FALSE(static_cast<bool>(p));
    }
}

// Время жизни: сколько объектов создали (с учётом move/reset), столько и
// разрушили — счётчик живых всегда возвращается к 0.
TEST(UniquePtrProps, LifetimeBalancedNoLeak) {
    static int alive = 0;
    struct Tracked { int v; explicit Tracked(int x): v(x){ ++alive; } ~Tracked(){ --alive; } };
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> op(0, 3);
    for (int iter = 0; iter < 300; ++iter) {
        alive = 0;
        {
            UniquePtr<Tracked> a(new Tracked(1));
            UniquePtr<Tracked> b(new Tracked(2));
            EXPECT_EQ(alive, 2);
            switch (op(rng)) {
                case 0: b = std::move(a); break;          // объект a разрушится сейчас
                case 1: a.reset(new Tracked(3)); break;   // старый a разрушится
                case 2: { UniquePtr<Tracked> c(std::move(b)); break; } // c уносит b
                case 3: a.reset(); break;                  // a опустошается
            }
        }
        EXPECT_EQ(alive, 0);  // все Tracked разрушены к выходу из блока
    }
}

// ── Задание 3. SharedPtr / WeakPtr — property ────────────────────────────────

// use_count() == числу живых SharedPtr, разделяющих один контрол-блок.
// Моделируем вектором копий: добавление/удаление копий двигает счётчик ±1.
TEST(SharedPtrProps, UseCountEqualsLiveOwners) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-1000, 1000);
    std::uniform_int_distribution<int> coin(0, 1);
    for (int iter = 0; iter < 200; ++iter) {
        const int v = val(rng);
        std::vector<SharedPtr<int>> owners;
        owners.emplace_back(new int(v));          // strong == 1
        int* raw = owners.front().get();
        EXPECT_EQ(owners.front().use_count(), 1);

        for (int step = 0; step < 30; ++step) {
            const bool grow = owners.size() == 1 || coin(rng);
            if (grow) {
                owners.push_back(owners.back());  // копия → ++strong
            } else {
                owners.pop_back();                // разрушение копии → --strong
            }
            const long expected = static_cast<long>(owners.size());
            for (const auto& sp : owners) {
                EXPECT_EQ(sp.use_count(), expected);
                EXPECT_EQ(sp.get(), raw);         // все на один объект
                EXPECT_EQ(*sp, v);
            }
        }
    }
}

// Жизнь объекта: пока есть хоть одна сильная ссылка — объект жив; когда все
// сильные исчезают — разрушается. weak-ссылки на это не влияют.
TEST(SharedPtrProps, ObjectAliveIffStrongPositive) {
    static int alive = 0;
    struct Tracked { Tracked(){ ++alive; } ~Tracked(){ --alive; } };
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> nweak(0, 4);
    for (int iter = 0; iter < 200; ++iter) {
        alive = 0;
        {
            SharedPtr<Tracked> a(new Tracked());
            EXPECT_EQ(alive, 1);
            // Навешиваем сколько-то weak — они не должны менять жизнь объекта.
            std::vector<WeakPtr<Tracked>> watchers;
            const int wk = nweak(rng);
            for (int i = 0; i < wk; ++i) watchers.emplace_back(a);
            EXPECT_EQ(alive, 1);
            for (const auto& w : watchers) EXPECT_FALSE(w.expired());

            {
                SharedPtr<Tracked> b = a;     // 2 сильные
                EXPECT_EQ(alive, 1);
            }
            EXPECT_EQ(alive, 1);              // осталась одна сильная
            // weak ещё живы (в watchers), но объект жив именно из-за a.
        }
        EXPECT_EQ(alive, 0);                  // сильных не осталось → разрушен
    }
}

// lock() — чистая функция от expired(): живой weak даёт владеющий SharedPtr с
// поднятым счётчиком; протухший weak даёт пустой SharedPtr.
TEST(SharedPtrProps, LockConsistentWithExpired) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(0, 9999);
    for (int iter = 0; iter < 300; ++iter) {
        const int v = val(rng);
        WeakPtr<int> w;
        {
            SharedPtr<int> a(new int(v));
            w = WeakPtr<int>(a);
            EXPECT_FALSE(w.expired());
            const long before = a.use_count();
            SharedPtr<int> locked = w.lock();
            ASSERT_TRUE(static_cast<bool>(locked));        // жив → не пустой
            EXPECT_EQ(*locked, v);
            EXPECT_EQ(locked.get(), a.get());
            EXPECT_EQ(a.use_count(), before + 1);          // lock поднял strong
            // locked разрушится здесь, вернув счётчик.
        }
        // a мёртв → weak протух.
        EXPECT_TRUE(w.expired());
        SharedPtr<int> dead = w.lock();
        EXPECT_FALSE(static_cast<bool>(dead));             // протух → пустой
        EXPECT_EQ(dead.use_count(), 0);
    }
}

// ── Задание 4. PoolAllocator — property ──────────────────────────────────────

// Инвариант сохранения числа блоков: (выдано + free_count) == N всегда; все
// одновременно выданные блоки различны, выровнены под блок и лежат в буфере.
TEST(PoolAllocatorProps, ConservationDistinctInBuffer) {
    std::mt19937 rng(0xC0FFEEu);
    constexpr std::size_t BS = sizeof(void*) > 24 ? sizeof(void*) : 24;
    std::uniform_int_distribution<std::size_t> count_dist(1, 16);
    std::uniform_int_distribution<int> coin(0, 1);

    for (int iter = 0; iter < 200; ++iter) {
        const std::size_t N = count_dist(rng);
        std::vector<std::byte> storage(BS * N);
        std::byte* base = storage.data();
        PoolAllocator pool(base, BS, N);
        EXPECT_EQ(pool.free_count(), N);
        EXPECT_EQ(pool.block_size(), BS);

        std::vector<void*> live;  // сейчас выданные
        for (int step = 0; step < 60; ++step) {
            const bool want_alloc = live.empty() || (live.size() < N && coin(rng));
            if (want_alloc) {
                void* p = pool.allocate();
                ASSERT_NE(p, nullptr);                      // место есть
                std::byte* bp = static_cast<std::byte*>(p);
                EXPECT_GE(bp, base);
                EXPECT_LT(bp, base + BS * N);
                EXPECT_EQ(static_cast<std::size_t>(bp - base) % BS, 0u);  // на границе блока
                live.push_back(p);
            } else {
                // Вернуть случайный из выданных.
                std::uniform_int_distribution<std::size_t> idx(0, live.size() - 1);
                std::size_t i = idx(rng);
                pool.deallocate(live[i]);
                live[i] = live.back();
                live.pop_back();
            }
            // Инвариант сохранения.
            EXPECT_EQ(live.size() + pool.free_count(), N);
            // Все одновременно выданные блоки различны.
            std::set<void*> uniq(live.begin(), live.end());
            EXPECT_EQ(uniq.size(), live.size());
        }
        // Когда все выданы — следующий allocate даёт nullptr.
        while (pool.free_count() > 0) { ASSERT_NE(pool.allocate(), nullptr); }
        EXPECT_EQ(pool.allocate(), nullptr);
    }
}

// Round-trip: deallocate ровно одного блока всегда поднимает free_count на 1 и
// позволяет немедленно снова выдать непустой блок.
TEST(PoolAllocatorProps, DeallocateAllocateRoundTrip) {
    std::mt19937 rng(0xC0FFEEu);
    constexpr std::size_t BS = 32;
    std::uniform_int_distribution<std::size_t> count_dist(2, 12);
    for (int iter = 0; iter < 250; ++iter) {
        const std::size_t N = count_dist(rng);
        std::vector<std::byte> storage(BS * N);
        PoolAllocator pool(storage.data(), BS, N);

        // Выдаём все.
        std::vector<void*> blocks;
        for (std::size_t i = 0; i < N; ++i) blocks.push_back(pool.allocate());
        EXPECT_EQ(pool.free_count(), 0u);

        // Возвращаем случайный, free_count +1, и снова выдаём непустой.
        std::uniform_int_distribution<std::size_t> idx(0, N - 1);
        std::size_t i = idx(rng);
        pool.deallocate(blocks[i]);
        EXPECT_EQ(pool.free_count(), 1u);
        void* again = pool.allocate();
        ASSERT_NE(again, nullptr);
        EXPECT_EQ(pool.free_count(), 0u);
    }
}

// ── Задание 5. placement new — property ──────────────────────────────────────

// construct_at19 строит объект ровно в переданном буфере, и значение
// round-trip'ится (записали через конструктор — прочитали из объекта).
TEST(PlacementNewProps, ConstructRoundTripsValueInBuffer) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<long long> val(-1000000000LL, 1000000000LL);
    for (int iter = 0; iter < 400; ++iter) {
        const long long v = val(rng);
        alignas(long long) std::byte buf[sizeof(long long)];
        long long* p = construct_at19<long long>(buf, v);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(static_cast<void*>(p), static_cast<void*>(buf));  // именно в buf
        EXPECT_EQ(*p, v);                                           // round-trip значения
        destroy_at19(p);
    }
}

// Строим vector с пробросом аргументов конструктора в сыром буфере; содержимое
// совпадает с эталонным std::vector; destroy вызывает деструктор (нет утечки).
TEST(PlacementNewProps, ForwardsArgsMatchesStdVector) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<std::size_t> len(0, 12);
    std::uniform_int_distribution<int> fill(-1000, 1000);
    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = len(rng);
        const int f = fill(rng);
        alignas(std::vector<int>) std::byte buf[sizeof(std::vector<int>)];
        auto* v = construct_at19<std::vector<int>>(buf, n, f);
        ASSERT_NE(v, nullptr);
        const std::vector<int> oracle(n, f);  // эталон
        ASSERT_EQ(v->size(), n);
        EXPECT_TRUE(std::equal(v->begin(), v->end(), oracle.begin()));
        destroy_at19(v);
    }
}

// destroy_at19 всегда вызывает деструктор: на серии построений счётчик живых
// после каждого destroy возвращается к нулю.
TEST(PlacementNewProps, DestroyAlwaysRunsDestructor) {
    static int alive = 0;
    struct Tracked { Tracked(){ ++alive; } ~Tracked(){ --alive; } };
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> reps(1, 8);
    for (int iter = 0; iter < 300; ++iter) {
        alive = 0;
        const int k = reps(rng);
        for (int j = 0; j < k; ++j) {
            alignas(Tracked) std::byte buf[sizeof(Tracked)];
            Tracked* t = construct_at19<Tracked>(buf);
            EXPECT_EQ(alive, 1);
            destroy_at19(t);
            EXPECT_EQ(alive, 0);
        }
    }
}
