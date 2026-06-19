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

// --- Дополнительные задания ---

TEST(Stl, WordFrequencyBasic) {
    auto f = word_frequency("a b a c a b");
    EXPECT_EQ(f.size(), 3u);
    EXPECT_EQ(f["a"], 3);
    EXPECT_EQ(f["b"], 2);
    EXPECT_EQ(f["c"], 1);
}

TEST(Stl, WordFrequencyCaseInsensitive) {
    // регистр приводится к нижнему: Hello/HELLO/hello — одно слово
    auto f = word_frequency("Hello hello HELLO world WORLD");
    EXPECT_EQ(f.size(), 2u);
    EXPECT_EQ(f["hello"], 3);
    EXPECT_EQ(f["world"], 2);
    // ключей в верхнем регистре быть не должно
    EXPECT_EQ(f.count("Hello"), 0u);
    EXPECT_EQ(f.count("WORLD"), 0u);
}

TEST(Stl, WordFrequencyWhitespaceAndEmpty) {
    // несколько пробелов, табы и переводы строк — всё это разделители
    auto f = word_frequency("  one\t two \n  one  ");
    EXPECT_EQ(f.size(), 2u);
    EXPECT_EQ(f["one"], 2);
    EXPECT_EQ(f["two"], 1);
    EXPECT_TRUE(word_frequency("").empty());
    EXPECT_TRUE(word_frequency("   \t\n  ").empty());
}

TEST(Stl, TopKBasic) {
    EXPECT_EQ(top_k({3, 1, 4, 1, 5, 9, 2, 6}, 3), (std::vector<int>{9, 6, 5}));
    EXPECT_EQ(top_k({10, 20, 30}, 1), (std::vector<int>{30}));
}

TEST(Stl, TopKWithDuplicates) {
    // дубликаты сохраняются: топ-4 из этого набора — 5,5,4,4
    EXPECT_EQ(top_k({4, 5, 4, 5, 1, 2}, 4), (std::vector<int>{5, 5, 4, 4}));
}

TEST(Stl, TopKEdgeCases) {
    EXPECT_TRUE(top_k({1, 2, 3}, 0).empty());
    EXPECT_TRUE(top_k({1, 2, 3}, -2).empty());
    EXPECT_TRUE(top_k({}, 3).empty());
    // k больше размера — весь вектор по убыванию
    EXPECT_EQ(top_k({2, 1, 3}, 10), (std::vector<int>{3, 2, 1}));
}

TEST(Stl, DropBelowReturnsRemovedCount) {
    std::vector<int> v{5, 1, 8, 2, 9, 3, 7};
    std::size_t removed = drop_below(v, 5);
    // удаляются 1, 2, 3 -> остаются 5, 8, 9, 7 в исходном порядке
    EXPECT_EQ(removed, 3u);
    EXPECT_EQ(v, (std::vector<int>{5, 8, 9, 7}));
}

TEST(Stl, DropBelowKeepsOrderAndBoundary) {
    std::vector<int> v{10, 10, 9, 11, 10};
    // порог 10: строго меньшие (9) удаляются, ровно 10 остаются
    std::size_t removed = drop_below(v, 10);
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(v, (std::vector<int>{10, 10, 11, 10}));
}

TEST(Stl, DropBelowAllAndNone) {
    std::vector<int> all_gone{1, 2, 3};
    EXPECT_EQ(drop_below(all_gone, 100), 3u);
    EXPECT_TRUE(all_gone.empty());

    std::vector<int> none{5, 6, 7};
    EXPECT_EQ(drop_below(none, 0), 0u);
    EXPECT_EQ(none, (std::vector<int>{5, 6, 7}));
}
