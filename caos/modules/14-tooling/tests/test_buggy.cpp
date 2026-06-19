// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, как ДОЛЖНЫ работать функции из src/buggy.c ПОСЛЕ того, как ты
// почИнишь баги. Пока баги на месте — тесты красные. Почини код в .c — станут зелёными.
// Сам этот файл менять НЕ надо: если «исправить» тест, ты обманешь только себя.
//
// Запуск:  ./caos/run.sh 14

#include <gtest/gtest.h>
#include <cstddef>

extern "C" {
#include "buggy.h"
}

// ---------- sum_first_n ----------

TEST(SumFirstN, EmptyIsZero) {
    int a[1] = {42};            // n == 0: ни один элемент не читаем, сумма 0
    EXPECT_EQ(sum_first_n(a, 0), 0);
}

TEST(SumFirstN, SingleElement) {
    int a[1] = {7};
    EXPECT_EQ(sum_first_n(a, 1), 7);   // ловит off-by-one: последний элемент НЕ должен теряться
}

TEST(SumFirstN, SeveralElements) {
    int a[5] = {1, 2, 3, 4, 5};
    EXPECT_EQ(sum_first_n(a, 5), 15);
}

TEST(SumFirstN, WithNegatives) {
    int a[4] = {10, -3, -2, 5};
    EXPECT_EQ(sum_first_n(a, 4), 10);
}

// ---------- max_in ----------

TEST(MaxIn, SinglePositive) {
    int a[1] = {9};
    EXPECT_EQ(max_in(a, 1), 9);
}

TEST(MaxIn, MixedValues) {
    int a[5] = {3, 9, 2, 7, 4};
    EXPECT_EQ(max_in(a, 5), 9);
}

TEST(MaxIn, AllNegative) {
    // Ключевой случай: если максимум инициализировать нулём — здесь ответ будет неверным.
    int a[4] = {-7, -2, -9, -5};
    EXPECT_EQ(max_in(a, 4), -2);
}

TEST(MaxIn, MaxIsLast) {
    int a[3] = {-10, -20, -1};
    EXPECT_EQ(max_in(a, 3), -1);
}

// ---------- reverse_array ----------

TEST(ReverseArray, EvenLength) {
    int a[4] = {1, 2, 3, 4};
    reverse_array(a, 4);
    EXPECT_EQ(a[0], 4);
    EXPECT_EQ(a[1], 3);
    EXPECT_EQ(a[2], 2);
    EXPECT_EQ(a[3], 1);
}

TEST(ReverseArray, OddLength) {
    int a[5] = {1, 2, 3, 4, 5};
    reverse_array(a, 5);
    EXPECT_EQ(a[0], 5);
    EXPECT_EQ(a[1], 4);
    EXPECT_EQ(a[2], 3);   // центральный элемент остаётся на месте
    EXPECT_EQ(a[3], 2);
    EXPECT_EQ(a[4], 1);
}

TEST(ReverseArray, SingleElement) {
    int a[1] = {42};
    reverse_array(a, 1);
    EXPECT_EQ(a[0], 42);
}

TEST(ReverseArray, DoesNotTouchNeighbours) {
    // «Часовые» (guard) по краям: если reverse лезет за границу [1..4],
    // он испортит guard'ы — а они должны остаться нетронутыми.
    int buf[6] = {111, 1, 2, 3, 4, 222};
    reverse_array(buf + 1, 4);     // разворачиваем только середину
    EXPECT_EQ(buf[0], 111);        // левый часовой цел
    EXPECT_EQ(buf[5], 222);        // правый часовой цел  <-- падает при выходе за границу
    EXPECT_EQ(buf[1], 4);
    EXPECT_EQ(buf[2], 3);
    EXPECT_EQ(buf[3], 2);
    EXPECT_EQ(buf[4], 1);
}

// ---------- count_char ----------

TEST(CountChar, NotPresent) {
    EXPECT_EQ(count_char("hello", 'z'), 0u);
}

TEST(CountChar, OnceOnly) {
    EXPECT_EQ(count_char("hello", 'e'), 1u);
}

TEST(CountChar, ManyTimes) {
    // Ключевой случай: 'l' встречается дважды, 'o' в "loop" — дважды.
    EXPECT_EQ(count_char("hello", 'l'), 2u);
    EXPECT_EQ(count_char("loop", 'o'), 2u);
}

TEST(CountChar, AllSame) {
    EXPECT_EQ(count_char("aaaa", 'a'), 4u);
}

TEST(CountChar, EmptyString) {
    EXPECT_EQ(count_char("", 'a'), 0u);
}
