#include <gtest/gtest.h>
#include <stdexcept>
#include "task03.hpp"

TEST(Pointers, SumArray) {
    int a[] = {1, 2, 3, 4, 5};
    EXPECT_EQ(sum_array(a, 5), 15);
    EXPECT_EQ(sum_array(a, 0), 0);
    int b[] = {-3, 3};
    EXPECT_EQ(sum_array(b, 2), 0);
}

TEST(Pointers, SwapInts) {
    int x = 1, y = 2;
    swap_ints(&x, &y);
    EXPECT_EQ(x, 2);
    EXPECT_EQ(y, 1);
}

TEST(Pointers, CountValue) {
    int a[] = {1, 2, 2, 3, 2};
    EXPECT_EQ(count_value(a, 5, 2), 3);
    EXPECT_EQ(count_value(a, 5, 9), 0);
}

TEST(Pointers, MaxElementPtr) {
    int a[] = {3, 7, 1, 9, 4};
    const int* m = max_element_ptr(a, 5);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(*m, 9);
    EXPECT_EQ(m, &a[3]);  // именно указатель на нужный элемент
    EXPECT_EQ(max_element_ptr(a, 0), nullptr);
}

TEST(Pointers, ReverseInPlace) {
    int a[] = {1, 2, 3, 4, 5};
    reverse_in_place(a, 5);
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[1], 4);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[3], 2);
    EXPECT_EQ(a[4], 1);

    int b[] = {10, 20};
    reverse_in_place(b, 2);
    EXPECT_EQ(b[0], 20);
    EXPECT_EQ(b[1], 10);
}

// ── Задание 6: reverse через два указателя ───────────────────────────────────

TEST(Pointers, ReverseWithPointersOdd) {
    int a[] = {1, 2, 3, 4, 5};
    reverse_with_pointers(a, 5);
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[1], 4);
    EXPECT_EQ(a[2], 3);
    EXPECT_EQ(a[3], 2);
    EXPECT_EQ(a[4], 1);
}

TEST(Pointers, ReverseWithPointersEven) {
    int a[] = {7, 8, 9, 10};
    reverse_with_pointers(a, 4);
    EXPECT_EQ(a[0], 10);
    EXPECT_EQ(a[1], 9);
    EXPECT_EQ(a[2], 8);
    EXPECT_EQ(a[3], 7);
}

TEST(Pointers, ReverseWithPointersEdgeCases) {
    // n == 1 — ничего не меняется.
    int one[] = {42};
    reverse_with_pointers(one, 1);
    EXPECT_EQ(one[0], 42);

    // n == 0 — не должно тронуть память (просто не падаем).
    int* empty = nullptr;
    reverse_with_pointers(empty, 0);
    SUCCEED();
}

// ── Задание 7: DynArray ──────────────────────────────────────────────────────

TEST(DynArrayTest, FillsOnConstruction) {
    DynArray a(4, 7);
    EXPECT_EQ(a.size(), 4);
    EXPECT_EQ(a.at(0), 7);
    EXPECT_EQ(a.at(1), 7);
    EXPECT_EQ(a.at(2), 7);
    EXPECT_EQ(a.at(3), 7);
}

TEST(DynArrayTest, DefaultFillIsZero) {
    DynArray a(3);
    EXPECT_EQ(a.size(), 3);
    EXPECT_EQ(a.sum(), 0);
}

TEST(DynArrayTest, EmptyArray) {
    DynArray a(0, 99);
    EXPECT_EQ(a.size(), 0);
    EXPECT_EQ(a.sum(), 0);
}

TEST(DynArrayTest, WriteThroughAtAndSum) {
    DynArray a(3, 0);
    a.at(0) = 10;   // неконстантная at возвращает int&
    a.at(1) = 20;
    a.at(2) = 30;
    EXPECT_EQ(a.at(1), 20);
    EXPECT_EQ(a.sum(), 60);
}

TEST(DynArrayTest, FillOverwritesAll) {
    DynArray a(5, 1);
    a.fill(3);
    EXPECT_EQ(a.sum(), 15);
    for (int i = 0; i < a.size(); ++i) {
        EXPECT_EQ(a.at(i), 3);
    }
}

TEST(DynArrayTest, AtThrowsOutOfRange) {
    DynArray a(2, 0);
    EXPECT_THROW(a.at(-1), std::out_of_range);
    EXPECT_THROW(a.at(2), std::out_of_range);
    EXPECT_THROW(a.at(100), std::out_of_range);
    // А корректный индекс не бросает.
    EXPECT_NO_THROW(a.at(0));
}

TEST(DynArrayTest, ConstAtReads) {
    const DynArray a(3, 5);
    EXPECT_EQ(a.at(2), 5);
    EXPECT_THROW(a.at(3), std::out_of_range);
}

// ── Задание 8: глубокое vs поверхностное копирование ─────────────────────────

TEST(OwnedIntTest, CopyConstructorIsDeepValue) {
    OwnedInt a(42);
    OwnedInt b(a);
    EXPECT_EQ(b.get(), 42);          // значение скопировано
    EXPECT_NE(a.raw(), b.raw());     // но буфер ОТДЕЛЬНЫЙ (глубокое копирование)
}

TEST(OwnedIntTest, CopyIsIndependent) {
    OwnedInt a(10);
    OwnedInt b(a);
    EXPECT_NE(a.raw(), b.raw());      // буферы разные — иначе set(99) задел бы оба
    b.set(99);
    EXPECT_EQ(a.get(), 10);          // изменение копии не задело оригинал
    EXPECT_EQ(b.get(), 99);
}

TEST(OwnedIntTest, AssignmentIsDeepValue) {
    OwnedInt a(5);
    OwnedInt b(0);
    b = a;
    EXPECT_EQ(b.get(), 5);
    EXPECT_NE(a.raw(), b.raw());     // присваивание тоже даёт свой буфер
}

TEST(OwnedIntTest, AssignmentIsIndependent) {
    OwnedInt a(7);
    OwnedInt b(0);
    b = a;
    a.set(123);                      // меняем оригинал после присваивания
    EXPECT_EQ(b.get(), 7);           // копия не должна измениться
}

TEST(OwnedIntTest, SelfAssignmentSafe) {
    OwnedInt a(3);
    a = a;                           // не должно ломать объект или течь
    EXPECT_EQ(a.get(), 3);
}
