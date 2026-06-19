#include <gtest/gtest.h>
#include "word_freq.hpp"

TEST(WordFreq, Counts) {
    auto f = word_count("a b a c a b");
    EXPECT_EQ(f["a"], 3);
    EXPECT_EQ(f["b"], 2);
    EXPECT_EQ(f["c"], 1);
}

TEST(WordFreq, HandlesNewlinesAndSpaces) {
    auto f = word_count("  one\ntwo   one\tthree ");
    EXPECT_EQ(f["one"], 2);
    EXPECT_EQ(f["two"], 1);
    EXPECT_EQ(f["three"], 1);
    EXPECT_EQ(f.size(), 3u);
}

TEST(WordFreq, CaseSensitive) {
    auto f = word_count("Go go GO");
    EXPECT_EQ(f["Go"], 1);
    EXPECT_EQ(f["go"], 1);
    EXPECT_EQ(f["GO"], 1);
}

TEST(WordFreq, TopNByFrequencyThenAlpha) {
    auto f = word_count("apple banana apple cherry banana apple");
    auto top = top_n(f, 2);
    ASSERT_EQ(top.size(), 2u);
    EXPECT_EQ(top[0].first, "apple");   // 3
    EXPECT_EQ(top[0].second, 3);
    EXPECT_EQ(top[1].first, "banana");  // 2
    EXPECT_EQ(top[1].second, 2);
}

TEST(WordFreq, TopNTieBrokenAlphabetically) {
    // все по 1 разу -> алфавитный порядок
    auto f = word_count("delta charlie bravo alpha");
    auto top = top_n(f, 3);
    ASSERT_EQ(top.size(), 3u);
    EXPECT_EQ(top[0].first, "alpha");
    EXPECT_EQ(top[1].first, "bravo");
    EXPECT_EQ(top[2].first, "charlie");
}
