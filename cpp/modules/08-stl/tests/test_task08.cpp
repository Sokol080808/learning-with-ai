#include <gtest/gtest.h>
#include "task08.hpp"

TEST(Stl, Evens) {
    EXPECT_EQ(evens({1, 2, 3, 4, 5, 6}), (std::vector<int>{2, 4, 6}));
    EXPECT_TRUE(evens({1, 3, 5}).empty());
}

TEST(Stl, CountGreater) {
    EXPECT_EQ(count_greater({1, 5, 10, 3, 8}, 4), 3);
    EXPECT_EQ(count_greater({1, 2, 3}, 10), 0);
}

TEST(Stl, Squared) {
    EXPECT_EQ(squared({1, 2, 3, 4}), (std::vector<int>{1, 4, 9, 16}));
}

TEST(Stl, SortedDesc) {
    EXPECT_EQ(sorted_desc({3, 1, 4, 1, 5}), (std::vector<int>{5, 4, 3, 1, 1}));
}

TEST(Stl, CharFrequency) {
    auto f = char_frequency("hello");
    EXPECT_EQ(f['l'], 2);
    EXPECT_EQ(f['h'], 1);
    EXPECT_EQ(f['o'], 1);
    EXPECT_EQ(f.count('z'), 0u);
}
