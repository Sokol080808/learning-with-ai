#include <gtest/gtest.h>
#include <utility>      // std::move
#include <stdexcept>    // std::out_of_range
#include <type_traits>  // std::is_copy_constructible и т.п.
#include "int_vector.hpp"

TEST(IntVector, EmptyByDefault) {
    IntVector v;
    EXPECT_EQ(v.size(), 0u);
    EXPECT_TRUE(v.empty());
}

TEST(IntVector, FillConstructor) {
    IntVector v(3, 7);
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0], 7);
    EXPECT_EQ(v[1], 7);
    EXPECT_EQ(v[2], 7);
    EXPECT_GE(v.capacity(), v.size());
}

TEST(IntVector, PushBackGrows) {
    IntVector v;
    for (int i = 0; i < 10; ++i) v.push_back(i * i);
    ASSERT_EQ(v.size(), 10u);
    EXPECT_GE(v.capacity(), 10u);
    EXPECT_EQ(v[0], 0);
    EXPECT_EQ(v[3], 9);
    EXPECT_EQ(v[9], 81);
}

TEST(IntVector, IndexIsWritable) {
    IntVector v(2, 0);
    v[0] = 42;
    EXPECT_EQ(v[0], 42);
}

TEST(IntVector, DeepCopyIndependent) {
    IntVector a(3, 1);
    IntVector b = a;        // копирующий конструктор
    b[0] = 99;
    EXPECT_EQ(a[0], 1);     // оригинал НЕ изменился => копия глубокая
    EXPECT_EQ(b[0], 99);
    EXPECT_EQ(a.size(), b.size());
}

TEST(IntVector, CopyAssignment) {
    IntVector a(2, 5);
    IntVector b;
    b = a;                  // копирующее присваивание
    ASSERT_EQ(b.size(), 2u);
    b[1] = 0;
    EXPECT_EQ(a[1], 5);
}

TEST(IntVector, SelfAssignmentSafe) {
    IntVector a(3, 4);
    a = a;                  // не должно ломаться/течь
    ASSERT_EQ(a.size(), 3u);
    EXPECT_EQ(a[2], 4);
}

TEST(IntVector, MoveConstructorStealsBuffer) {
    IntVector a(4, 8);
    IntVector b = std::move(a);  // перемещающий конструктор
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b[0], 8);
    EXPECT_EQ(a.size(), 0u);     // источник стал пустым
}

TEST(IntVector, MoveAssignment) {
    IntVector a(4, 2);
    IntVector b;
    b = std::move(a);
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(a.size(), 0u);
}

// ───────────────────────────────────────────────────────────────────────────
// Задание 2: Buffer
// ───────────────────────────────────────────────────────────────────────────

TEST(Buffer, FillConstructor) {
    Buffer b(4, 2.5);
    ASSERT_EQ(b.size(), 4u);
    ASSERT_NE(b.data(), nullptr);
    EXPECT_DOUBLE_EQ(b[0], 2.5);
    EXPECT_DOUBLE_EQ(b[3], 2.5);
}

TEST(Buffer, DefaultValueIsZero) {
    Buffer b(3);
    ASSERT_EQ(b.size(), 3u);
    EXPECT_DOUBLE_EQ(b[0], 0.0);
    EXPECT_DOUBLE_EQ(b[1], 0.0);
    EXPECT_DOUBLE_EQ(b[2], 0.0);
}

TEST(Buffer, EmptyBufferHasNullData) {
    Buffer empty(0);
    EXPECT_EQ(empty.size(), 0u);
    EXPECT_EQ(empty.data(), nullptr);
    // а вот НЕпустой буфер обязан иметь реальную память — это отличает
    // настоящую реализацию от заглушки, всегда отдающей nullptr
    Buffer nonempty(4);
    EXPECT_EQ(nonempty.size(), 4u);
    EXPECT_NE(nonempty.data(), nullptr);
}

TEST(Buffer, IndexIsWritable) {
    Buffer b(2, 0.0);
    b[1] = 9.0;
    EXPECT_DOUBLE_EQ(b[1], 9.0);
    EXPECT_DOUBLE_EQ(b[0], 0.0);
}

TEST(Buffer, FillOverwritesAll) {
    Buffer b(3, 1.0);
    b.fill(-4.0);
    EXPECT_DOUBLE_EQ(b[0], -4.0);
    EXPECT_DOUBLE_EQ(b[1], -4.0);
    EXPECT_DOUBLE_EQ(b[2], -4.0);
}

TEST(Buffer, AtChecksBounds) {
    Buffer b(2, 1.0);
    EXPECT_DOUBLE_EQ(b.at(0), 1.0);
    EXPECT_DOUBLE_EQ(b.at(1), 1.0);
    EXPECT_THROW(b.at(2), std::out_of_range);   // ровно std::out_of_range
    EXPECT_THROW(b.at(999), std::out_of_range);
}

TEST(Buffer, CopyIsDeepAndIndependent) {
    Buffer a(3, 7.0);
    Buffer b = a;                 // копирующий конструктор
    ASSERT_EQ(b.size(), 3u);
    ASSERT_NE(b.data(), a.data()); // РАЗНЫЕ буферы
    b[0] = 100.0;
    EXPECT_DOUBLE_EQ(a[0], 7.0);   // оригинал не тронут
    EXPECT_DOUBLE_EQ(b[0], 100.0);
}

TEST(Buffer, CopyAssignmentIsDeep) {
    Buffer a(2, 5.0);
    Buffer b(5, 0.0);
    b = a;                         // копирующее присваивание (разные размеры!)
    ASSERT_EQ(b.size(), 2u);
    ASSERT_NE(b.data(), a.data());
    b[0] = -1.0;
    EXPECT_DOUBLE_EQ(a[0], 5.0);
}

TEST(Buffer, SelfAssignmentSafe) {
    Buffer a(3, 4.0);
    Buffer& ref = a;
    a = ref;                       // самоприсваивание не должно ломать/чистить данные
    ASSERT_EQ(a.size(), 3u);
    EXPECT_DOUBLE_EQ(a[2], 4.0);
}

TEST(Buffer, MoveConstructorStealsBuffer) {
    Buffer a(4, 8.0);
    const double* old_ptr = a.data();
    Buffer b = std::move(a);       // перемещающий конструктор
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b.data(), old_ptr);  // тот же буфер «переехал» без копирования
    EXPECT_DOUBLE_EQ(b[0], 8.0);
    EXPECT_EQ(a.size(), 0u);       // источник опустошён
    EXPECT_EQ(a.data(), nullptr);
}

TEST(Buffer, MoveAssignmentStealsBuffer) {
    Buffer a(4, 2.0);
    const double* old_ptr = a.data();
    Buffer b(1, 0.0);
    b = std::move(a);
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b.data(), old_ptr);
    EXPECT_EQ(a.size(), 0u);
    EXPECT_EQ(a.data(), nullptr);
}

TEST(Buffer, SwapExchangesContents) {
    Buffer a(2, 1.0);
    Buffer b(3, 9.0);
    a.swap(b);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(b.size(), 2u);
    EXPECT_DOUBLE_EQ(a[0], 9.0);
    EXPECT_DOUBLE_EQ(b[0], 1.0);
}

TEST(Buffer, MoveIsNoexcept) {
    // правильно написанный move обязан быть noexcept — это требуют контейнеры
    EXPECT_TRUE(std::is_nothrow_move_constructible_v<Buffer>);
    EXPECT_TRUE(std::is_nothrow_move_assignable_v<Buffer>);
}

// ───────────────────────────────────────────────────────────────────────────
// Задание 3: UniqueHandle
// ───────────────────────────────────────────────────────────────────────────

// move-only тип на этапе КОМПИЛЯЦИИ: копировать нельзя, перемещать можно
static_assert(!std::is_copy_constructible_v<UniqueHandle>, "UniqueHandle must NOT be copyable");
static_assert(!std::is_copy_assignable_v<UniqueHandle>,    "UniqueHandle must NOT be copy-assignable");
static_assert(std::is_move_constructible_v<UniqueHandle>,  "UniqueHandle must be movable");
static_assert(std::is_move_assignable_v<UniqueHandle>,     "UniqueHandle must be move-assignable");

TEST(UniqueHandle, DefaultIsEmpty) {
    resource_reset_registry();
    UniqueHandle h;
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.get(), kInvalidHandle);
    EXPECT_FALSE(static_cast<bool>(h));
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, MakeHandleAcquires) {
    resource_reset_registry();
    {
        UniqueHandle h = make_handle();
        EXPECT_TRUE(h.valid());
        EXPECT_NE(h.get(), kInvalidHandle);
        EXPECT_EQ(resource_live_count(), 1);   // ресурс захвачен
    }
    EXPECT_EQ(resource_live_count(), 0);        // деструктор освободил его
}

TEST(UniqueHandle, DestructorReleases) {
    resource_reset_registry();
    {
        UniqueHandle h(resource_acquire());
        EXPECT_EQ(resource_live_count(), 1);
    }
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, MoveTransfersOwnership) {
    resource_reset_registry();
    UniqueHandle a = make_handle();
    int id = a.get();
    UniqueHandle b = std::move(a);
    EXPECT_EQ(b.get(), id);
    EXPECT_TRUE(b.valid());
    EXPECT_FALSE(a.valid());                    // источник опустошён
    EXPECT_EQ(a.get(), kInvalidHandle);
    EXPECT_EQ(resource_live_count(), 1);        // владелец один — ресурс не задвоился
}

TEST(UniqueHandle, MoveAssignReleasesOldThenSteals) {
    resource_reset_registry();
    UniqueHandle a = make_handle();   // id A
    UniqueHandle b = make_handle();   // id B
    EXPECT_EQ(resource_live_count(), 2);
    int idA = a.get();
    b = std::move(a);                 // b должен освободить B, забрать A
    EXPECT_EQ(b.get(), idA);
    EXPECT_FALSE(a.valid());
    EXPECT_EQ(resource_live_count(), 1);  // B освобождён, A жив => ровно 1
}

TEST(UniqueHandle, ReleaseHandsOutOwnership) {
    resource_reset_registry();
    UniqueHandle h = make_handle();
    int id = h.release();             // отдать наружу, НЕ освобождая
    EXPECT_NE(id, kInvalidHandle);
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(resource_live_count(), 1);  // ресурс ещё жив — теперь за него отвечаем мы
    resource_release(id);                  // освобождаем вручную
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, ResetReplacesResource) {
    resource_reset_registry();
    UniqueHandle h = make_handle();
    int first = h.get();
    h.reset(resource_acquire());     // освободить первый, взять второй
    EXPECT_NE(h.get(), first);
    EXPECT_TRUE(h.valid());
    EXPECT_EQ(resource_live_count(), 1);  // первый освобождён, второй жив

    h.reset();                       // сбросить в пустоту
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(resource_live_count(), 0);
}

TEST(UniqueHandle, SwapExchangesIds) {
    resource_reset_registry();
    UniqueHandle a = make_handle();
    UniqueHandle b;
    int id = a.get();
    a.swap(b);
    EXPECT_FALSE(a.valid());
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(b.get(), id);
    EXPECT_EQ(resource_live_count(), 1);
}

TEST(UniqueHandle, NoLeakAcrossManyMoves) {
    resource_reset_registry();
    {
        UniqueHandle h = make_handle();
        for (int i = 0; i < 5; ++i) {
            UniqueHandle tmp = std::move(h);
            h = std::move(tmp);       // гоняем владение туда-сюда
        }
        EXPECT_TRUE(h.valid());
        EXPECT_EQ(resource_live_count(), 1);
    }
    EXPECT_EQ(resource_live_count(), 0);
}
