#include <gtest/gtest.h>
#include "task08.hpp"
#include <random>
#include <algorithm>
#include <numeric>
#include <climits>
#include <set>

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

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
// Эти тесты прогоняют сотни случайных входов (фиксированный seed -> CI не флакует)
// и проверяют ИНВАРИАНТЫ функций, а не конкретные примеры. Сверяются с эталонами
// из std:: там, где они есть.

namespace {

// Проверка, что b — перестановка a (мультимножества совпадают).
bool is_permutation_of(std::vector<int> a, std::vector<int> b) {
    if (a.size() != b.size()) return false;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    return a == b;
}

}  // namespace

// --- evens: подмножество в исходном порядке, ровно чётные, сохраняет порядок и кратности ---
TEST(StlProps, EvensInvariants) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len_dist(0, 40);
    std::uniform_int_distribution<int> val_dist(-50, 50);

    for (int iter = 0; iter < 400; ++iter) {
        const int n = len_dist(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val_dist(rng);

        std::vector<int> out = evens(v);

        // Оракул: std::copy_if по тому же предикату.
        std::vector<int> oracle;
        std::copy_if(v.begin(), v.end(), std::back_inserter(oracle),
                     [](int x) { return x % 2 == 0; });
        ASSERT_EQ(out, oracle) << "evens должна совпадать со std::copy_if(чётность)";

        // Все элементы результата — чётные.
        for (int x : out) ASSERT_EQ(x % 2, 0);

        // Результат — подпоследовательность исходного (порядок сохранён):
        // продвигаемся по v, матча элементы out по порядку.
        std::size_t j = 0;
        for (int x : v) {
            if (j < out.size() && out[j] == x) ++j;
        }
        ASSERT_EQ(j, out.size()) << "evens должна сохранять исходный порядок";

        // Размер == количеству чётных в исходнике.
        const auto even_cnt = std::count_if(v.begin(), v.end(),
                                            [](int x) { return x % 2 == 0; });
        ASSERT_EQ(out.size(), static_cast<std::size_t>(even_cnt));
    }

    EXPECT_TRUE(evens({}).empty());        // пусто
    EXPECT_TRUE(evens({1}).empty());       // один нечётный
    EXPECT_EQ(evens({0}), (std::vector<int>{0}));  // ноль чётный
    EXPECT_EQ(evens({-4, -3, -2}), (std::vector<int>{-4, -2}));  // отрицательные чётные
}

// --- count_greater: согласуется со std::count_if; границы порога ---
TEST(StlProps, CountGreaterMatchesOracle) {
    std::mt19937 rng(0x1234567u);
    std::uniform_int_distribution<int> len_dist(0, 50);
    std::uniform_int_distribution<int> val_dist(-100, 100);
    std::uniform_int_distribution<int> thr_dist(-120, 120);

    for (int iter = 0; iter < 400; ++iter) {
        const int n = len_dist(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val_dist(rng);
        const int thr = thr_dist(rng);

        const int got = count_greater(v, thr);
        const int oracle = static_cast<int>(std::count_if(
            v.begin(), v.end(), [thr](int x) { return x > thr; }));
        ASSERT_EQ(got, oracle);

        // В диапазоне [0, n].
        ASSERT_GE(got, 0);
        ASSERT_LE(got, n);

        // Дополнение: (> thr) + (<= thr) == n.
        const int le = static_cast<int>(std::count_if(
            v.begin(), v.end(), [thr](int x) { return x <= thr; }));
        ASSERT_EQ(got + le, n);
    }

    // Строгое неравенство: равные порогу НЕ считаются.
    EXPECT_EQ(count_greater({5, 5, 5}, 5), 0);
    EXPECT_EQ(count_greater({}, 0), 0);
    EXPECT_EQ(count_greater({INT_MAX}, INT_MAX - 1), 1);
    EXPECT_EQ(count_greater({INT_MIN}, INT_MIN), 0);
}

// --- squared: поэлементный квадрат, всегда >= 0, длина сохранена, |out|==in^2 ---
TEST(StlProps, SquaredInvariants) {
    std::mt19937 rng(0xABCDEFu);
    std::uniform_int_distribution<int> len_dist(0, 40);
    // Ограничиваем значения, чтобы квадрат не переполнял int.
    std::uniform_int_distribution<int> val_dist(-46340, 46340);

    for (int iter = 0; iter < 400; ++iter) {
        const int n = len_dist(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val_dist(rng);

        std::vector<int> out = squared(v);
        ASSERT_EQ(out.size(), v.size());
        for (std::size_t i = 0; i < v.size(); ++i) {
            ASSERT_EQ(out[i], v[i] * v[i]);
            ASSERT_GE(out[i], 0);  // квадрат неотрицателен
        }
    }

    EXPECT_TRUE(squared({}).empty());
    EXPECT_EQ(squared({0}), (std::vector<int>{0}));
    EXPECT_EQ(squared({-7}), (std::vector<int>{49}));  // знак исчезает
}

// --- sorted_desc: отсортировано по убыванию, перестановка исходника, не меняет аргумент ---
TEST(StlProps, SortedDescInvariants) {
    std::mt19937 rng(0xDEAD42u);
    std::uniform_int_distribution<int> len_dist(0, 60);
    std::uniform_int_distribution<int> val_dist(-200, 200);

    for (int iter = 0; iter < 400; ++iter) {
        const int n = len_dist(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val_dist(rng);

        std::vector<int> orig = v;
        std::vector<int> out = sorted_desc(v);  // приём по значению

        // 1. Невозрастающий порядок.
        ASSERT_TRUE(std::is_sorted(out.begin(), out.end(), std::greater<int>()));
        // 2. Перестановка исходного мультимножества.
        ASSERT_TRUE(is_permutation_of(orig, out));
        // 3. Оракул: совпадает с reverse(std::sort по возрастанию).
        std::vector<int> oracle = orig;
        std::sort(oracle.begin(), oracle.end());
        std::reverse(oracle.begin(), oracle.end());
        ASSERT_EQ(out, oracle);
        // 4. Аргумент (по значению) не должен затрагивать локальную копию вызывающего.
        ASSERT_EQ(v, orig);
    }

    EXPECT_TRUE(sorted_desc({}).empty());
    EXPECT_EQ(sorted_desc({42}), (std::vector<int>{42}));
    EXPECT_EQ(sorted_desc({3, 3, 3}), (std::vector<int>{3, 3, 3}));  // дубликаты
}

// --- char_frequency: сумма частот == длине строки; каждая частота == std::count ---
TEST(StlProps, CharFrequencySumsToLength) {
    std::mt19937 rng(0xFEEDu);
    std::uniform_int_distribution<int> len_dist(0, 60);
    // Маленький алфавит -> гарантированы повторы.
    std::uniform_int_distribution<int> ch_dist(0, 5);
    const std::string alphabet = "abcXY ";

    for (int iter = 0; iter < 400; ++iter) {
        const int n = len_dist(rng);
        std::string s;
        s.reserve(n);
        for (int i = 0; i < n; ++i) s.push_back(alphabet[ch_dist(rng)]);

        std::map<char, int> f = char_frequency(s);

        // Сумма всех частот == длина строки.
        long long total = 0;
        for (const auto& [c, cnt] : f) {
            total += cnt;
            ASSERT_GT(cnt, 0);  // в map попадают только встретившиеся символы
            // Оракул для конкретного символа.
            ASSERT_EQ(cnt, static_cast<int>(std::count(s.begin(), s.end(), c)));
        }
        ASSERT_EQ(total, static_cast<long long>(s.size()));

        // Число ключей == числу различных символов.
        std::set<char> distinct(s.begin(), s.end());
        ASSERT_EQ(f.size(), distinct.size());
        // Отсутствующий символ имеет count()==0.
        ASSERT_EQ(f.count('Z'), 0u);
    }

    EXPECT_TRUE(char_frequency("").empty());
}

// --- top_k: первые k по убыванию; подмультимножество; согласован с полной сортировкой ---
TEST(StlProps, TopKInvariants) {
    std::mt19937 rng(0x70Bu);
    std::uniform_int_distribution<int> len_dist(0, 50);
    std::uniform_int_distribution<int> val_dist(-100, 100);

    for (int iter = 0; iter < 500; ++iter) {
        const int n = len_dist(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val_dist(rng);
        // k иногда отрицательный, иногда больше размера.
        std::uniform_int_distribution<int> k_dist(-3, n + 5);
        const int k = k_dist(rng);

        std::vector<int> out = top_k(v, k);

        // Ожидаемый размер.
        std::size_t expected_size;
        if (k <= 0) expected_size = 0;
        else expected_size = std::min<std::size_t>(static_cast<std::size_t>(k), v.size());
        ASSERT_EQ(out.size(), expected_size);

        // Результат отсортирован по убыванию.
        ASSERT_TRUE(std::is_sorted(out.begin(), out.end(), std::greater<int>()));

        // Оракул: полная сортировка по убыванию, затем первые expected_size.
        std::vector<int> full = v;
        std::sort(full.begin(), full.end(), std::greater<int>());
        full.resize(expected_size);
        ASSERT_EQ(out, full)
            << "top_k должна давать те же значения, что усечённая сортировка по убыванию";

        // Результат — подмультимножество исходника.
        std::vector<int> a = v, b = out;
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        ASSERT_TRUE(std::includes(a.begin(), a.end(), b.begin(), b.end()));
    }

    EXPECT_TRUE(top_k({}, 5).empty());
    EXPECT_TRUE(top_k({1, 2, 3}, 0).empty());
    EXPECT_TRUE(top_k({1, 2, 3}, -1).empty());
    EXPECT_EQ(top_k({INT_MIN, INT_MAX, 0}, 2), (std::vector<int>{INT_MAX, 0}));
}

// --- drop_below: возвращает число удалённых; оставшиеся все >= threshold; порядок сохранён ---
TEST(StlProps, DropBelowInvariants) {
    std::mt19937 rng(0xD0Bu);
    std::uniform_int_distribution<int> len_dist(0, 50);
    std::uniform_int_distribution<int> val_dist(-100, 100);
    std::uniform_int_distribution<int> thr_dist(-110, 110);

    for (int iter = 0; iter < 500; ++iter) {
        const int n = len_dist(rng);
        std::vector<int> v(n);
        for (int& x : v) x = val_dist(rng);
        const int thr = thr_dist(rng);

        std::vector<int> orig = v;
        const std::size_t orig_size = v.size();
        const std::size_t removed = drop_below(v, thr);  // на месте

        // 1. removed + остаток == исходный размер.
        ASSERT_EQ(removed + v.size(), orig_size);

        // 2. removed == число элементов строго меньше thr.
        const std::size_t below = static_cast<std::size_t>(std::count_if(
            orig.begin(), orig.end(), [thr](int x) { return x < thr; }));
        ASSERT_EQ(removed, below);

        // 3. Все оставшиеся >= thr.
        for (int x : v) ASSERT_GE(x, thr);

        // 4. Оставшиеся сохраняют относительный порядок исходника (подпоследовательность).
        std::size_t j = 0;
        for (int x : orig) {
            if (j < v.size() && v[j] == x) ++j;
        }
        ASSERT_EQ(j, v.size());

        // 5. Оракул: оставшиеся == исходник, профильтрованный (x >= thr).
        std::vector<int> kept;
        std::copy_if(orig.begin(), orig.end(), std::back_inserter(kept),
                     [thr](int x) { return x >= thr; });
        ASSERT_EQ(v, kept);
    }

    {
        std::vector<int> empty_v;
        EXPECT_EQ(drop_below(empty_v, 0), 0u);
        EXPECT_TRUE(empty_v.empty());
    }
    {
        // Граница: ровно равные порогу остаются.
        std::vector<int> v{7, 7, 7};
        EXPECT_EQ(drop_below(v, 7), 0u);
        EXPECT_EQ(v, (std::vector<int>{7, 7, 7}));
    }
    {
        std::vector<int> v{INT_MIN, 0, INT_MAX};
        EXPECT_EQ(drop_below(v, 0), 1u);  // только INT_MIN < 0
        EXPECT_EQ(v, (std::vector<int>{0, INT_MAX}));
    }
}

// --- word_frequency: сумма частот == числу токенов; ключи в нижнем регистре; согласован с char_frequency-стилем ---
TEST(StlProps, WordFrequencyInvariants) {
    std::mt19937 rng(0x701Du);
    std::uniform_int_distribution<int> nwords_dist(0, 20);
    std::uniform_int_distribution<int> wlen_dist(1, 6);
    // Берём смешанный регистр латиницы.
    const std::string letters = "abABcC";
    std::uniform_int_distribution<int> ch_dist(0, static_cast<int>(letters.size()) - 1);
    std::uniform_int_distribution<int> ws_dist(0, 2);
    const char* seps[] = {" ", "  ", "\t"};

    for (int iter = 0; iter < 400; ++iter) {
        const int nwords = nwords_dist(rng);
        std::vector<std::string> tokens;
        std::string text;
        for (int w = 0; w < nwords; ++w) {
            const int wl = wlen_dist(rng);
            std::string word;
            for (int i = 0; i < wl; ++i) word.push_back(letters[ch_dist(rng)]);
            tokens.push_back(word);
            if (w) text += seps[ws_dist(rng)];
            text += word;
        }
        // Иногда обрамляем пробелами с краёв.
        if (ws_dist(rng) == 0) { text = "  " + text + " \n"; }

        std::map<std::string, int> f = word_frequency(text);

        // Оракул: разбиваем по whitespace и приводим к нижнему регистру вручную.
        std::map<std::string, int> oracle;
        long long oracle_total = 0;
        for (std::string t : tokens) {
            std::transform(t.begin(), t.end(), t.begin(), [](char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            });
            ++oracle[t];
            ++oracle_total;
        }
        ASSERT_EQ(f, oracle);

        // Сумма частот == число токенов.
        long long total = 0;
        for (const auto& [word, cnt] : f) {
            total += cnt;
            ASSERT_GT(cnt, 0);
            // Все ключи — в нижнем регистре (никаких A-Z).
            for (char c : word) {
                ASSERT_FALSE(c >= 'A' && c <= 'Z') << "ключи должны быть в нижнем регистре";
            }
        }
        ASSERT_EQ(total, oracle_total);
    }

    EXPECT_TRUE(word_frequency("").empty());
    EXPECT_TRUE(word_frequency("   \t\n ").empty());
    // Идемпотентность приведения регистра: один и тот же ключ.
    EXPECT_EQ(word_frequency("Aa aA AA aa")["aa"], 4);
}

// ===== evens_squared (Идея 8 — C++20 Ranges) =====

TEST(Stl, EvensSquaredFixed) {
    // Базовый пример из условия задачи.
    EXPECT_EQ(evens_squared({1, 2, 3, 4, 5, 6}), (std::vector<int>{4, 16, 36}));
}

TEST(Stl, EvensSquaredEdgeCases) {
    // Пустой вход — пустой результат.
    EXPECT_TRUE(evens_squared({}).empty());
    // Все нечётные — результат пустой.
    EXPECT_TRUE(evens_squared({1, 3, 5, 7}).empty());
    // Все чётные.
    EXPECT_EQ(evens_squared({2, 4, 6}), (std::vector<int>{4, 16, 36}));
    // Один элемент — чётный.
    EXPECT_EQ(evens_squared({-4}), (std::vector<int>{16}));
    // Ноль — чётный, 0^2 == 0.
    EXPECT_EQ(evens_squared({0}), (std::vector<int>{0}));
    // Отрицательные чётные: квадрат положительный.
    EXPECT_EQ(evens_squared({-2, -3, -4}), (std::vector<int>{4, 16}));
}

// Seeded property-тест: результат должен точно совпадать с ручным filter+square.
TEST(StlProps, EvensSquaredMatchesOracle) {
    std::mt19937 rng(0x08E5u);
    std::uniform_int_distribution<int> len_dist(0, 50);
    // Ограничиваем значения: квадрат не должен выходить за int.
    std::uniform_int_distribution<int> val_dist(-46340, 46340);

    for (int iter = 0; iter < 500; ++iter) {
        const int n = len_dist(rng);
        std::vector<int> xs(n);
        for (int& x : xs) x = val_dist(rng);

        std::vector<int> got = evens_squared(xs);

        // Оракул: ручной цикл filter+square без ranges.
        std::vector<int> oracle;
        for (int x : xs) {
            if (x % 2 == 0) oracle.push_back(x * x);
        }

        ASSERT_EQ(got, oracle)
            << "evens_squared должна совпадать с ручным filter+square";

        // Дополнительные инварианты результата:
        // 1. Все элементы результата — неотрицательны (квадраты).
        for (int x : got) ASSERT_GE(x, 0);

        // 2. Порядок сохранён — got является подпоследовательностью квадратов xs.
        std::size_t j = 0;
        for (int x : xs) {
            if (x % 2 == 0) {
                if (j < got.size()) {
                    ASSERT_EQ(got[j], x * x);
                    ++j;
                }
            }
        }
        ASSERT_EQ(j, got.size());
    }
}
