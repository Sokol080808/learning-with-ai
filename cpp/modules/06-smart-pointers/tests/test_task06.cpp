#include <gtest/gtest.h>
#include "task06.hpp"

TEST(SmartPointers, MakeUniqueInt) {
    auto p = make_unique_int(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
}

TEST(IntList, PushFrontOrder) {
    IntList list;
    list.push_front(1);
    list.push_front(2);
    list.push_front(3);
    // последний добавленный — первым: 3, 2, 1
    std::vector<int> expected = {3, 2, 1};
    EXPECT_EQ(list.to_vector(), expected);
}

TEST(IntList, Size) {
    IntList list;
    EXPECT_EQ(list.size(), 0u);
    list.push_front(10);
    list.push_front(20);
    EXPECT_EQ(list.size(), 2u);
}

TEST(IntList, Contains) {
    IntList list;
    list.push_front(5);
    list.push_front(7);
    EXPECT_TRUE(list.contains(5));
    EXPECT_TRUE(list.contains(7));
    EXPECT_FALSE(list.contains(99));
}

TEST(IntList, EmptyToVector) {
    IntList list;
    EXPECT_TRUE(list.to_vector().empty());
}
