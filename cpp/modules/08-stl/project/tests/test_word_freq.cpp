#include <gtest/gtest.h>
#include "word_freq.hpp"
#include <random>
#include <algorithm>
#include <string>
#include <vector>

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

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
// Сотни случайных текстов с фиксированным seed (CI не флакует). Проверяем инварианты
// word_count и top_n, сверяясь с независимым оракулом.

// --- word_count: регистрозависимый подсчёт; сумма частот == числу токенов ---
TEST(WordFreqProps, WordCountInvariants) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> nwords_dist(0, 25);
    std::uniform_int_distribution<int> wlen_dist(1, 5);
    // Смешанный регистр -> проверяем регистрозависимость.
    const std::string letters = "abAB";
    std::uniform_int_distribution<int> ch_dist(0, static_cast<int>(letters.size()) - 1);
    const char* seps[] = {" ", "   ", "\t", "\n"};
    std::uniform_int_distribution<int> sep_dist(0, 3);

    for (int iter = 0; iter < 400; ++iter) {
        const int nwords = nwords_dist(rng);
        std::vector<std::string> tokens;
        std::string text;
        for (int w = 0; w < nwords; ++w) {
            const int wl = wlen_dist(rng);
            std::string word;
            for (int i = 0; i < wl; ++i) word.push_back(letters[ch_dist(rng)]);
            tokens.push_back(word);
            if (w) text += seps[sep_dist(rng)];
            text += word;
        }

        auto f = word_count(text);

        // Оракул: подсчёт точных (регистрозависимых) токенов.
        std::map<std::string, int> oracle;
        for (const auto& t : tokens) ++oracle[t];
        ASSERT_EQ(f, oracle);

        // Сумма частот == число токенов; все частоты положительны.
        long long total = 0;
        for (const auto& [word, cnt] : f) {
            total += cnt;
            ASSERT_GT(cnt, 0);
        }
        ASSERT_EQ(total, static_cast<long long>(tokens.size()));
    }

    EXPECT_TRUE(word_count("").empty());
    EXPECT_TRUE(word_count("  \t\n ").empty());
    // Регистрозависимость: три разных ключа.
    EXPECT_EQ(word_count("Go go GO").size(), 3u);
}

// --- top_n: отсортирован по (частота убыв., слово возр.); размер min(n, |freq|); подмножество ---
TEST(WordFreqProps, TopNInvariants) {
    std::mt19937 rng(0x70Eu);
    std::uniform_int_distribution<int> nkeys_dist(0, 20);
    std::uniform_int_distribution<int> wlen_dist(1, 4);
    std::uniform_int_distribution<int> freq_dist(1, 6);
    const std::string letters = "abcde";
    std::uniform_int_distribution<int> ch_dist(0, static_cast<int>(letters.size()) - 1);

    for (int iter = 0; iter < 400; ++iter) {
        // Строим случайный частотный словарь напрямую.
        std::map<std::string, int> freq;
        const int nkeys = nkeys_dist(rng);
        for (int k = 0; k < nkeys; ++k) {
            const int wl = wlen_dist(rng);
            std::string word;
            for (int i = 0; i < wl; ++i) word.push_back(letters[ch_dist(rng)]);
            // map подавляет дубликаты ключей — это нормально.
            freq[word] = freq_dist(rng);
        }

        std::uniform_int_distribution<int> n_dist(-2, static_cast<int>(freq.size()) + 3);
        const int n = n_dist(rng);

        auto top = top_n(freq, n);

        // 1. Размер == clamp(n) ограниченный размером словаря.
        const std::size_t want = (n < 0) ? 0u
            : std::min<std::size_t>(static_cast<std::size_t>(n), freq.size());
        ASSERT_EQ(top.size(), want);

        // 2. Каждая пара существует в исходном словаре с той же частотой.
        for (const auto& [word, cnt] : top) {
            auto it = freq.find(word);
            ASSERT_NE(it, freq.end());
            ASSERT_EQ(it->second, cnt);
        }

        // 3. Слова уникальны.
        for (std::size_t i = 1; i < top.size(); ++i) {
            ASSERT_NE(top[i - 1].first, top[i].first);
        }

        // 4. Порядок: частота по убыванию, при равенстве — слово по возрастанию.
        for (std::size_t i = 1; i < top.size(); ++i) {
            const auto& a = top[i - 1];
            const auto& b = top[i];
            const bool ok = (a.second > b.second) ||
                            (a.second == b.second && a.first < b.first);
            ASSERT_TRUE(ok) << "нарушен порядок top_n на позиции " << i;
        }

        // 5. Оракул: полная сортировка по тому же компаратору, затем первые want.
        std::vector<std::pair<std::string, int>> all(freq.begin(), freq.end());
        std::sort(all.begin(), all.end(),
                  [](const std::pair<std::string, int>& a,
                     const std::pair<std::string, int>& b) {
                      if (a.second != b.second) return a.second > b.second;
                      return a.first < b.first;
                  });
        all.resize(want);
        ASSERT_EQ(top, all);
    }

    // Краевые случаи.
    {
        std::map<std::string, int> empty_freq;
        EXPECT_TRUE(top_n(empty_freq, 5).empty());
    }
    {
        std::map<std::string, int> f{{"a", 1}, {"b", 2}};
        EXPECT_TRUE(top_n(f, 0).empty());
        EXPECT_TRUE(top_n(f, -3).empty());
        EXPECT_EQ(top_n(f, 100).size(), 2u);  // n больше размера -> весь словарь
    }
}
