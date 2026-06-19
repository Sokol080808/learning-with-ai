#include <gtest/gtest.h>
#include <string>
#include "task13.hpp"

TEST(Stack, PushTopPopSize) {
    Stack<int> s;
    EXPECT_TRUE(s.empty());
    s.push(1);
    s.push(2);
    s.push(3);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.top(), 3);
    s.pop();
    EXPECT_EQ(s.top(), 2);
    EXPECT_EQ(s.size(), 2u);
    EXPECT_FALSE(s.empty());
}

TEST(Stack, WorksForStrings) {
    Stack<std::string> s;
    s.push("a");
    s.push("b");
    EXPECT_EQ(s.top(), "b");
}

TEST(Algo, BinarySearch) {
    std::vector<int> v{1, 3, 5, 7, 9, 11};
    EXPECT_EQ(binary_search_idx(v, 1), 0);
    EXPECT_EQ(binary_search_idx(v, 7), 3);
    EXPECT_EQ(binary_search_idx(v, 11), 5);
    EXPECT_EQ(binary_search_idx(v, 4), -1);
    EXPECT_EQ(binary_search_idx({}, 1), -1);
}

TEST(Algo, InsertionSort) {
    EXPECT_EQ(insertion_sort({3, 1, 4, 1, 5, 9, 2, 6}),
              (std::vector<int>{1, 1, 2, 3, 4, 5, 6, 9}));
    EXPECT_EQ(insertion_sort({}), (std::vector<int>{}));
    EXPECT_EQ(insertion_sort({42}), (std::vector<int>{42}));
    EXPECT_EQ(insertion_sort({5, 4, 3, 2, 1}), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(Algo, TwoSum) {
    auto r = two_sum({2, 7, 11, 15}, 9);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->first, 0);
    EXPECT_EQ(r->second, 1);

    auto r2 = two_sum({3, 2, 4}, 6);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->first, 1);
    EXPECT_EQ(r2->second, 2);

    EXPECT_FALSE(two_sum({1, 2, 3}, 100).has_value());
}
