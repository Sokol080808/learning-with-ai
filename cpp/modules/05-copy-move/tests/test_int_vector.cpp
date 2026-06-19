#include <gtest/gtest.h>
#include <utility>  // std::move
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
