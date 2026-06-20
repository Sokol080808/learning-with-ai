// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, как ДОЛЖНЫ работать функции из src/buggy.c ПОСЛЕ того, как ты
// почИнишь баги. Пока баги на месте — тесты красные. Почини код в .c — станут зелёными.
// Сам этот файл менять НЕ надо: если «исправить» тест, ты обманешь только себя.
//
// Запуск:  ./caos/run.sh 14

#include <gtest/gtest.h>
#include <cstddef>
#include <random>
#include <vector>
#include <string>
#include <algorithm>
#include <climits>
#include <cstring>
#include <map>
#include <set>

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

// ============================================================
// RANDOMIZED / PROPERTY TESTS  (seed 0xC0FFEE, ~500–1000 iters)
// ============================================================

// ---------- sum_first_n — randomized oracle ----------

// Oracle: naive sequential sum over the same array.
// Bites the original off-by-one (loop stopped at n-1 instead of n).
TEST(SumFirstNRand, AgainstNaiveOracle) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> val_dist(-10000, 10000);
    std::uniform_int_distribution<int> len_dist(0, 64);

    for (int iter = 0; iter < 1000; ++iter) {
        int n = len_dist(rng);
        std::vector<int> a(n == 0 ? 1 : n);
        for (auto& x : a) x = val_dist(rng);

        // Naive oracle
        int expected = 0;
        for (int i = 0; i < n; ++i) expected += a[i];

        EXPECT_EQ(sum_first_n(a.data(), n), expected)
            << "iter=" << iter << " n=" << n;
    }
}

// n == 0 always returns 0 regardless of array content.
TEST(SumFirstNRand, ZeroNAlwaysZero) {
    std::mt19937 rng(0xC0FFEE + 1);
    std::uniform_int_distribution<int> val_dist(INT_MIN / 2, INT_MAX / 2);
    int a[4];
    for (int iter = 0; iter < 500; ++iter) {
        for (auto& x : a) x = val_dist(rng);
        EXPECT_EQ(sum_first_n(a, 0), 0) << "iter=" << iter;
    }
}

// Partial prefix sums: sum_first_n(a, k) must equal sum of a[0..k-1].
// Validates that the function uses exactly the first n elements.
TEST(SumFirstNRand, PrefixConsistency) {
    std::mt19937 rng(0xC0FFEE + 2);
    std::uniform_int_distribution<int> val_dist(-500, 500);

    for (int iter = 0; iter < 500; ++iter) {
        const int N = 32;
        std::vector<int> a(N);
        for (auto& x : a) x = val_dist(rng);

        int running = 0;
        for (int k = 0; k <= N; ++k) {
            EXPECT_EQ(sum_first_n(a.data(), k), running)
                << "iter=" << iter << " k=" << k;
            running += a[k < N ? k : 0];  // advance only when k < N
            if (k == N) break;
        }
    }
}

// Additive property: sum_first_n(a, n) == sum_first_n(a, n-1) + a[n-1]
TEST(SumFirstNRand, AdditiveStep) {
    std::mt19937 rng(0xC0FFEE + 3);
    std::uniform_int_distribution<int> val_dist(-1000, 1000);
    std::uniform_int_distribution<int> len_dist(1, 64);

    for (int iter = 0; iter < 500; ++iter) {
        int n = len_dist(rng);
        std::vector<int> a(n);
        for (auto& x : a) x = val_dist(rng);

        int full = sum_first_n(a.data(), n);
        int minus_one = sum_first_n(a.data(), n - 1);
        EXPECT_EQ(full, minus_one + a[n - 1])
            << "iter=" << iter << " n=" << n;
    }
}

// ---------- max_in — randomized oracle ----------

// Oracle: std::max_element — bites the "init best=0" bug for all-negative arrays.
TEST(MaxInRand, AgainstStdMaxElement) {
    std::mt19937 rng(0xC0FFEE + 10);
    std::uniform_int_distribution<int> val_dist(-100000, 100000);
    std::uniform_int_distribution<int> len_dist(1, 64);

    for (int iter = 0; iter < 1000; ++iter) {
        int n = len_dist(rng);
        std::vector<int> a(n);
        for (auto& x : a) x = val_dist(rng);

        int expected = *std::max_element(a.begin(), a.end());
        EXPECT_EQ(max_in(a.data(), n), expected)
            << "iter=" << iter << " n=" << n;
    }
}

// All-negative arrays: result must be the least-negative element (still < 0).
// This test directly targets the "init best=0" bug.
TEST(MaxInRand, AllNegativeRandom) {
    std::mt19937 rng(0xC0FFEE + 11);
    std::uniform_int_distribution<int> val_dist(-100000, -1);
    std::uniform_int_distribution<int> len_dist(1, 32);

    for (int iter = 0; iter < 500; ++iter) {
        int n = len_dist(rng);
        std::vector<int> a(n);
        for (auto& x : a) x = val_dist(rng);

        int expected = *std::max_element(a.begin(), a.end());
        EXPECT_LT(expected, 0) << "sanity";
        EXPECT_EQ(max_in(a.data(), n), expected)
            << "iter=" << iter << " n=" << n;
    }
}

// Single-element: max_in must return exactly that element, even INT_MIN.
TEST(MaxInRand, SingleElementEdgeCases) {
    std::mt19937 rng(0xC0FFEE + 12);
    std::uniform_int_distribution<int> val_dist(INT_MIN, INT_MAX);

    for (int iter = 0; iter < 500; ++iter) {
        int v = val_dist(rng);
        EXPECT_EQ(max_in(&v, 1), v) << "iter=" << iter << " v=" << v;
    }
}

// Max is always an element actually present in the array (not 0 or some sentinel).
TEST(MaxInRand, ResultIsInArray) {
    std::mt19937 rng(0xC0FFEE + 13);
    std::uniform_int_distribution<int> val_dist(-50, 50);
    std::uniform_int_distribution<int> len_dist(1, 32);

    for (int iter = 0; iter < 500; ++iter) {
        int n = len_dist(rng);
        std::vector<int> a(n);
        for (auto& x : a) x = val_dist(rng);

        int result = max_in(a.data(), n);
        bool found = std::find(a.begin(), a.end(), result) != a.end();
        EXPECT_TRUE(found) << "iter=" << iter << " result=" << result
                           << " not in array";
    }
}

// ---------- reverse_array — randomized oracle ----------

// Round-trip: reverse twice restores original.
TEST(ReverseArrayRand, DoubleReverseIsIdentity) {
    std::mt19937 rng(0xC0FFEE + 20);
    std::uniform_int_distribution<int> val_dist(-10000, 10000);
    std::uniform_int_distribution<int> len_dist(0, 64);

    for (int iter = 0; iter < 1000; ++iter) {
        int n = len_dist(rng);
        std::vector<int> orig(n == 0 ? 1 : n);
        for (auto& x : orig) x = val_dist(rng);
        std::vector<int> a = orig;

        reverse_array(a.data(), n);
        reverse_array(a.data(), n);

        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(a[i], orig[i])
                << "iter=" << iter << " n=" << n << " i=" << i;
        }
    }
}

// Oracle: compare element-by-element against std::reverse on a copy.
TEST(ReverseArrayRand, AgainstStdReverse) {
    std::mt19937 rng(0xC0FFEE + 21);
    std::uniform_int_distribution<int> val_dist(-10000, 10000);
    std::uniform_int_distribution<int> len_dist(1, 64);

    for (int iter = 0; iter < 1000; ++iter) {
        int n = len_dist(rng);
        std::vector<int> a(n), expected(n);
        for (int i = 0; i < n; ++i) a[i] = expected[i] = val_dist(rng);

        std::reverse(expected.begin(), expected.end());
        reverse_array(a.data(), n);

        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(a[i], expected[i])
                << "iter=" << iter << " n=" << n << " i=" << i;
        }
    }
}

// Guard bytes around the buffer: reverse_array must not write outside [0, n).
// Bites the original j = n - i  (off-by-one past the end) and the i <= n/2
// loop condition (one extra swap that hits index n).
TEST(ReverseArrayRand, DoesNotTouchGuardBytes) {
    std::mt19937 rng(0xC0FFEE + 22);
    std::uniform_int_distribution<int> val_dist(-9999, 9999);
    std::uniform_int_distribution<int> len_dist(1, 60);

    const int GUARD_L = 0xDEAD1234;
    const int GUARD_R = 0xBEEF5678;

    for (int iter = 0; iter < 500; ++iter) {
        int n = len_dist(rng);
        // Layout: [GUARD_L] [n elements] [GUARD_R]
        std::vector<int> buf(n + 2);
        buf[0] = GUARD_L;
        buf[n + 1] = GUARD_R;
        for (int i = 1; i <= n; ++i) buf[i] = val_dist(rng);

        reverse_array(buf.data() + 1, n);

        EXPECT_EQ(buf[0], GUARD_L)
            << "iter=" << iter << " n=" << n << ": left guard corrupted";
        EXPECT_EQ(buf[n + 1], GUARD_R)
            << "iter=" << iter << " n=" << n << ": right guard corrupted";
    }
}

// Sorted-then-reversed property: reverse of {0,1,...,n-1} == {n-1,...,1,0}.
TEST(ReverseArrayRand, SortedArrayReverses) {
    std::mt19937 rng(0xC0FFEE + 23);
    std::uniform_int_distribution<int> len_dist(1, 128);

    for (int iter = 0; iter < 500; ++iter) {
        int n = len_dist(rng);
        std::vector<int> a(n);
        for (int i = 0; i < n; ++i) a[i] = i;

        reverse_array(a.data(), n);

        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(a[i], n - 1 - i)
                << "iter=" << iter << " n=" << n << " i=" << i;
        }
    }
}

// ---------- count_char — randomized oracle ----------

// Oracle: naive loop using std::count — bites the "count=1" instead of "count++" bug.
TEST(CountCharRand, AgainstStdCount) {
    std::mt19937 rng(0xC0FFEE + 30);
    // printable ASCII 32..126
    std::uniform_int_distribution<int> char_dist(32, 126);
    std::uniform_int_distribution<int> len_dist(0, 128);

    for (int iter = 0; iter < 1000; ++iter) {
        int n = len_dist(rng);
        std::string s(n, ' ');
        for (auto& ch : s) ch = static_cast<char>(char_dist(rng));

        char target = static_cast<char>(char_dist(rng));

        size_t expected = static_cast<size_t>(
            std::count(s.begin(), s.end(), target));
        EXPECT_EQ(count_char(s.c_str(), target), expected)
            << "iter=" << iter << " s=\"" << s << "\" target='" << target << "'";
    }
}

// High-repetition strings: all same character — result must equal string length.
// Bites "count=1" directly when length > 1.
TEST(CountCharRand, AllSameCharacter) {
    std::mt19937 rng(0xC0FFEE + 31);
    std::uniform_int_distribution<int> char_dist(32, 126);
    std::uniform_int_distribution<int> len_dist(2, 200);

    for (int iter = 0; iter < 500; ++iter) {
        char c = static_cast<char>(char_dist(rng));
        int n = len_dist(rng);
        std::string s(n, c);

        EXPECT_EQ(count_char(s.c_str(), c), static_cast<size_t>(n))
            << "iter=" << iter << " c='" << c << "' n=" << n;
    }
}

// Absent character: result must be 0 for any string.
TEST(CountCharRand, AbsentCharacterAlwaysZero) {
    std::mt19937 rng(0xC0FFEE + 32);
    std::uniform_int_distribution<int> len_dist(0, 64);

    for (int iter = 0; iter < 500; ++iter) {
        int n = len_dist(rng);
        // String only of 'a'; search for 'b'.
        std::string s(n, 'a');
        EXPECT_EQ(count_char(s.c_str(), 'b'), 0u)
            << "iter=" << iter << " n=" << n;
    }
}

// count_char never returns a value > strlen(s).
TEST(CountCharRand, CountNeverExceedsLength) {
    std::mt19937 rng(0xC0FFEE + 33);
    std::uniform_int_distribution<int> char_dist(32, 126);
    std::uniform_int_distribution<int> len_dist(0, 128);

    for (int iter = 0; iter < 500; ++iter) {
        int n = len_dist(rng);
        std::string s(n, ' ');
        for (auto& ch : s) ch = static_cast<char>(char_dist(rng));
        char target = static_cast<char>(char_dist(rng));

        size_t result = count_char(s.c_str(), target);
        EXPECT_LE(result, s.size())
            << "iter=" << iter << " result=" << result
            << " > len=" << s.size();
    }
}

// Additive split: count_char over a+b == count_char(a) + count_char(b).
TEST(CountCharRand, AdditiveSplit) {
    std::mt19937 rng(0xC0FFEE + 34);
    std::uniform_int_distribution<int> char_dist(32, 126);
    std::uniform_int_distribution<int> len_dist(0, 64);

    for (int iter = 0; iter < 500; ++iter) {
        int n1 = len_dist(rng), n2 = len_dist(rng);
        std::string s1(n1, ' '), s2(n2, ' ');
        for (auto& ch : s1) ch = static_cast<char>(char_dist(rng));
        for (auto& ch : s2) ch = static_cast<char>(char_dist(rng));
        char target = static_cast<char>(char_dist(rng));

        std::string combined = s1 + s2;
        size_t split_sum = count_char(s1.c_str(), target)
                         + count_char(s2.c_str(), target);
        size_t whole = count_char(combined.c_str(), target);

        EXPECT_EQ(whole, split_sum)
            << "iter=" << iter;
    }
}

// ============================================================
// ЗАДАНИЕ 5: FreqTable — хеш-таблица счётчиков частот слов
// Тесты проверяют корректное поведение ПОСЛЕ починки всех трёх
// багов.  На студенческой ветке (stub) они красные.
// ============================================================

// ---------- базовые случаи ----------

// Пустая таблица: любой lookup возвращает 0, unique == 0.
TEST(FreqTable, EmptyLookupIsZero) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(freq_lookup(t, "hello"), 0u);
    EXPECT_EQ(freq_lookup(t, ""),      0u);
    EXPECT_EQ(freq_unique(t),          0u);
    freq_destroy(t);
}

// Одно слово добавлено один раз → count == 1, unique == 1.
TEST(FreqTable, SingleWordOnce) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(freq_add(t, "cat"), 0);
    EXPECT_EQ(freq_lookup(t, "cat"), 1u);
    EXPECT_EQ(freq_unique(t),        1u);
    freq_destroy(t);
}

// Одно слово добавлено многократно → count растёт, unique остаётся 1.
// Ловит баг 3 (логическая ошибка: счётчик не увеличивался при повторном слове).
TEST(FreqTable, SingleWordRepeated) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(freq_add(t, "dog"), 0) << "i=" << i;
    }
    EXPECT_EQ(freq_lookup(t, "dog"), 10u);  // баг 3: без фикса вернёт 1
    EXPECT_EQ(freq_unique(t),         1u);
    freq_destroy(t);
}

// Два разных слова: счётчики независимы.
TEST(FreqTable, TwoDistinctWords) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    freq_add(t, "apple");
    freq_add(t, "banana");
    freq_add(t, "apple");
    EXPECT_EQ(freq_lookup(t, "apple"),  2u);
    EXPECT_EQ(freq_lookup(t, "banana"), 1u);
    EXPECT_EQ(freq_unique(t),           2u);
    freq_destroy(t);
}

// Слово не добавленное → 0.
TEST(FreqTable, AbsentWordIsZero) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    freq_add(t, "present");
    EXPECT_EQ(freq_lookup(t, "absent"), 0u);
    freq_destroy(t);
}

// freq_destroy(NULL) не должен падать.
TEST(FreqTable, DestroyNullIsSafe) {
    freq_destroy(nullptr);  // UB / segfault без правильной проверки
}

// ---------- баг 3: логическая ошибка в freq_add ----------

// Ключевой тест на баг 3: слово повторяется ровно N раз,
// lookup должен вернуть N, а unique == 1.
TEST(FreqTable, Bug3RepeatCount) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    const int N = 7;
    for (int i = 0; i < N; i++) freq_add(t, "repeat");
    // Без фикса (strcmp(word, word) всегда 0) freq_add считает, что слово
    // уже есть, и инкрементирует — но это случайно работает правильно.
    // Настоящий баг: strcmp(word, word)==0 всегда, поэтому первый же
    // вызов попадает в if и увеличивает несуществующий count.
    // На практике: freq_add при первом вызове находит «совпадение»
    // с неинициализированным мусором → UB или segfault.
    // После починки: ровно N.
    EXPECT_EQ(freq_lookup(t, "repeat"), static_cast<size_t>(N));
    EXPECT_EQ(freq_unique(t), 1u);
    freq_destroy(t);
}

// ---------- баг 2: signed overflow в хеш-функции ----------

// Длинные слова вызывают переполнение, если хеш считается в signed int.
// После фикса (unsigned int) слова с длинными ключами хешируются корректно
// и попадают в правильные корзины → lookup находит их.
TEST(FreqTable, Bug2LongWordHash) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    // Строка из 80 символов 'z' (код 122): при signed int h*33+122
    // переполняется уже на ~6-м символе → UBSan/UBSAN кричит.
    std::string longword(80, 'z');
    freq_add(t, longword.c_str());
    freq_add(t, longword.c_str());
    freq_add(t, longword.c_str());
    EXPECT_EQ(freq_lookup(t, longword.c_str()), 3u);
    freq_destroy(t);
}

// ---------- баг 1: use-after-free в freq_lookup ----------

// После вызова freq_destroy память освобождена.  Этот тест проверяет,
// что lookup до destroy работает корректно (ASan проверит сам процесс
// на use-after-free при выполнении тестов с -fsanitize=address).
TEST(FreqTable, Bug1UseAfterFreeGuard) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    freq_add(t, "alpha");
    freq_add(t, "beta");
    freq_add(t, "alpha");
    // lookup ПЕРЕД destroy — должно вернуть правильное значение
    EXPECT_EQ(freq_lookup(t, "alpha"), 2u);
    EXPECT_EQ(freq_lookup(t, "beta"),  1u);
    freq_destroy(t);
    // После destroy не вызываем lookup — это и есть use-after-free.
    // ASan поймает его в студенческой версии, где был баг.
}

// ---------- randomized / property тесты ----------

// Оракул: std::map<string,int> считает то же самое.
// Ловит оба бага логики (3) и хеша (2) на случайных данных.
TEST(FreqTableRand, AgainstMapOracle) {
    std::mt19937 rng(0xFEED42);
    // Словарь из 8 коротких слов, повторяемых случайно.
    const std::vector<std::string> vocab = {
        "the", "cat", "sat", "on", "mat", "a", "dog", "ran"
    };
    std::uniform_int_distribution<int> word_dist(0, static_cast<int>(vocab.size()) - 1);
    std::uniform_int_distribution<int> cnt_dist(1, 200);

    for (int iter = 0; iter < 100; ++iter) {
        FreqTable* t = freq_create();
        ASSERT_NE(t, nullptr);
        std::map<std::string, int> oracle;

        int ops = cnt_dist(rng);
        for (int i = 0; i < ops; i++) {
            const std::string& w = vocab[word_dist(rng)];
            freq_add(t, w.c_str());
            oracle[w]++;
        }

        for (const auto& kv : oracle) {
            EXPECT_EQ(freq_lookup(t, kv.first.c_str()),
                      static_cast<size_t>(kv.second))
                << "iter=" << iter << " word=" << kv.first;
        }
        EXPECT_EQ(freq_unique(t), oracle.size())
            << "iter=" << iter;

        freq_destroy(t);
    }
}

// unique() растёт ровно на 1 при каждом новом слове и не растёт при повторе.
TEST(FreqTableRand, UniqueCountMonotone) {
    std::mt19937 rng(0xCAFE00);
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    std::set<std::string> seen;

    const std::vector<std::string> words = {
        "one", "two", "three", "four", "five",
        "one", "two", "one",   "five", "six"
    };
    for (const auto& w : words) {
        bool was_new = (seen.find(w) == seen.end());
        seen.insert(w);
        freq_add(t, w.c_str());
        EXPECT_EQ(freq_unique(t), seen.size())
            << "after adding '" << w << "' (new=" << was_new << ")";
    }
    freq_destroy(t);
}

// Длинные слова (> 60 символов) хешируются корректно — ловит баг 2.
TEST(FreqTableRand, LongWordsDontCollide) {
    FreqTable* t = freq_create();
    ASSERT_NE(t, nullptr);
    // Два разных длинных слова, оба > 60 символов
    std::string w1(64, 'a');
    std::string w2(64, 'b');
    freq_add(t, w1.c_str());
    freq_add(t, w1.c_str());
    freq_add(t, w2.c_str());
    EXPECT_EQ(freq_lookup(t, w1.c_str()), 2u);
    EXPECT_EQ(freq_lookup(t, w2.c_str()), 1u);
    EXPECT_EQ(freq_unique(t), 2u);
    freq_destroy(t);
}
