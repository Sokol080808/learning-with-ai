#include <gtest/gtest.h>
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
