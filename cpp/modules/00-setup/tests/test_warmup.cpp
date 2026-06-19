#include <gtest/gtest.h>
#include "warmup.hpp"
#include <random>
#include <climits>
#include <cstdlib>

// Эти тесты трогать не нужно — это эталон поведения, к которому ты приводишь код.

TEST(Warmup, AddSumsNumbers) {
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(-4, 4), 0);
    EXPECT_EQ(add(10, 25), 35);
}

TEST(Warmup, SecondsInHours) {
    EXPECT_EQ(seconds_in(1), 3600);
    EXPECT_EQ(seconds_in(2), 7200);
    EXPECT_EQ(seconds_in(0), 0);
}

// Задание 2: abs_diff объявлена в .hpp, определена в .cpp.
// Если определение не написать — будет ошибка ЛИНКОВКИ, и тест не соберётся.
TEST(Warmup, AbsDiffIsAlwaysNonNegative) {
    EXPECT_EQ(abs_diff(7, 3), 4);
    EXPECT_EQ(abs_diff(3, 7), 4);   // порядок не важен — это модуль
    EXPECT_EQ(abs_diff(5, 5), 0);
    EXPECT_EQ(abs_diff(-2, 3), 5);
    EXPECT_EQ(abs_diff(-10, -4), 6);
}

// Задание 3: is_even — inline-функция целиком в заголовке.
TEST(Warmup, IsEvenSplitsParity) {
    EXPECT_TRUE(is_even(0));
    EXPECT_TRUE(is_even(2));
    EXPECT_TRUE(is_even(-4));
    EXPECT_TRUE(is_even(100));
    EXPECT_FALSE(is_even(1));
    EXPECT_FALSE(is_even(3));
    EXPECT_FALSE(is_even(-7));
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Все генераторы используют ФИКСИРОВАННЫЙ сид std::mt19937 — никакого time()/
// random_device, поэтому прогон в CI всегда воспроизводим и не «флакает».
// Диапазоны входов выбраны так, чтобы эталонная реализация (которая считает в
// long) не переполнялась: для int-арифметики берём половину диапазона int,
// чтобы сумма/разность гарантированно влезали.

// --- add: коммутативность, ассоциативность сдвига, нейтральный 0, оракул long ---
TEST(WarmupProps, AddMatchesLongOracleAndIsCommutative) {
    std::mt19937 rng(0xC0FFEEu);
    // Половина диапазона int: сумма двух таких чисел гарантированно влезает в int.
    std::uniform_int_distribution<int> dist(-1'000'000'000, 1'000'000'000);
    for (int i = 0; i < 400; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        long oracle = static_cast<long>(a) + static_cast<long>(b);
        EXPECT_EQ(add(a, b), static_cast<int>(oracle));  // совпадает с честной long-суммой
        EXPECT_EQ(add(a, b), add(b, a));                 // коммутативность
        EXPECT_EQ(add(a, 0), a);                         // 0 — нейтральный элемент
        EXPECT_EQ(add(0, b), b);
        // add(a+1, b) == add(a, b) + 1 (сдвиг одного аргумента на 1)
        if (a < 1'000'000'000) {
            EXPECT_EQ(add(a + 1, b), add(a, b) + 1);
        }
    }
}

// --- add: явные крайние случаи (границы int без переполнения суммы) ---
TEST(WarmupProps, AddEdgeCases) {
    EXPECT_EQ(add(0, 0), 0);
    EXPECT_EQ(add(INT_MAX, 0), INT_MAX);
    EXPECT_EQ(add(INT_MIN, 0), INT_MIN);
    EXPECT_EQ(add(INT_MAX, INT_MIN), -1);   // MAX + MIN == -1, переполнения нет
    EXPECT_EQ(add(INT_MAX - 1, 1), INT_MAX);
    EXPECT_EQ(add(INT_MIN + 1, -1), INT_MIN);
}

// --- seconds_in: линейность и точное равенство 3600*h на безопасном диапазоне ---
TEST(WarmupProps, SecondsInIsLinear) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(-500'000, 500'000);
    for (int i = 0; i < 400; ++i) {
        int h = dist(rng);
        long oracle = static_cast<long>(h) * 3600L;
        EXPECT_EQ(seconds_in(h), oracle);              // ровно h*3600
        EXPECT_EQ(seconds_in(h), static_cast<long>(h) * seconds_in(1));
        // Линейность по сложению часов: seconds_in(h)+seconds_in(1) == seconds_in(h+1)
        EXPECT_EQ(seconds_in(h) + seconds_in(1), seconds_in(h + 1));
        // Знак результата следует за знаком аргумента.
        if (h > 0) EXPECT_GT(seconds_in(h), 0);
        if (h < 0) EXPECT_LT(seconds_in(h), 0);
        if (h == 0) EXPECT_EQ(seconds_in(h), 0L);
    }
}

// --- seconds_in: крайние случаи ---
TEST(WarmupProps, SecondsInEdgeCases) {
    EXPECT_EQ(seconds_in(0), 0L);
    EXPECT_EQ(seconds_in(1), 3600L);
    EXPECT_EQ(seconds_in(-1), -3600L);
    // 24 часа в сутках, проверим что считается без сюрпризов.
    EXPECT_EQ(seconds_in(24), 86400L);
    // Большой час: умножение должно идти в long, не переполняя int (3600*1e6 > INT_MAX).
    EXPECT_EQ(seconds_in(1'000'000), 3'600'000'000L);
}

// --- abs_diff: симметрия, неотрицательность, оракул, связь с add ---
TEST(WarmupProps, AbsDiffIsSymmetricNonNegativeOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(-2'000'000'000, 2'000'000'000);
    for (int i = 0; i < 500; ++i) {
        int a = dist(rng);
        int b = dist(rng);
        long d = abs_diff(a, b);
        // Оракул: |a-b| честно в long.
        long oracle = std::labs(static_cast<long>(a) - static_cast<long>(b));
        EXPECT_EQ(d, oracle);
        EXPECT_GE(d, 0);                       // всегда неотрицательно
        EXPECT_EQ(d, abs_diff(b, a));          // симметрия |a-b| == |b-a|
        EXPECT_EQ(abs_diff(a, a), 0L);         // расстояние до себя — 0
        // Треугольное равенство для точки между: |a-b| == |a-c|+|c-b|, если c между a и b.
        int lo = a < b ? a : b;
        int hi = a < b ? b : a;
        long c = lo + (static_cast<long>(hi) - lo) / 2;  // середина в long, c в [lo,hi]
        EXPECT_EQ(abs_diff(a, static_cast<int>(c)) + abs_diff(static_cast<int>(c), b), d);
    }
}

// --- abs_diff: крайние случаи, включая границы int (где int-вычитание переполнилось бы) ---
TEST(WarmupProps, AbsDiffEdgeCases) {
    EXPECT_EQ(abs_diff(0, 0), 0L);
    EXPECT_EQ(abs_diff(5, 5), 0L);
    EXPECT_EQ(abs_diff(INT_MAX, INT_MIN),
              static_cast<long>(INT_MAX) - static_cast<long>(INT_MIN));  // ~2^32, нужен long
    EXPECT_EQ(abs_diff(INT_MIN, INT_MAX),
              static_cast<long>(INT_MAX) - static_cast<long>(INT_MIN));  // симметрия на границе
    EXPECT_EQ(abs_diff(INT_MAX, 0), static_cast<long>(INT_MAX));
    EXPECT_EQ(abs_diff(INT_MIN, 0), -static_cast<long>(INT_MIN));        // |INT_MIN| влезает в long
    EXPECT_EQ(abs_diff(-7, 7), 14L);
}

// --- is_even: оракул %2, дополнение нечётности, инвариант соседей ---
TEST(WarmupProps, IsEvenMatchesModuloOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
    for (int i = 0; i < 500; ++i) {
        int x = dist(rng);
        bool oracle = (x % 2 == 0);
        EXPECT_EQ(is_even(x), oracle);             // совпадает с прямым %2
        // Ровно один из двух соседей чётный: чётность чередуется.
        if (x != INT_MAX) EXPECT_NE(is_even(x), is_even(x + 1));
        if (x != INT_MIN) EXPECT_NE(is_even(x), is_even(x - 1));
        // 2*k всегда чётно (на безопасном множителе).
    }
    // Удвоение всегда даёт чётное.
    std::uniform_int_distribution<int> half(-1'000'000'000, 1'000'000'000);
    for (int i = 0; i < 200; ++i) {
        int k = half(rng);
        EXPECT_TRUE(is_even(k * 2));
        EXPECT_FALSE(is_even(k * 2 + 1));
        EXPECT_FALSE(is_even(k * 2 - 1));
    }
}

// --- is_even: крайние случаи на границах int ---
TEST(WarmupProps, IsEvenEdgeCases) {
    EXPECT_TRUE(is_even(0));
    EXPECT_FALSE(is_even(INT_MAX));   // INT_MAX == 2^31-1 — нечётное
    EXPECT_TRUE(is_even(INT_MIN));    // INT_MIN == -2^31     — чётное
    EXPECT_TRUE(is_even(INT_MAX - 1));
    EXPECT_FALSE(is_even(INT_MIN + 1));
    EXPECT_TRUE(is_even(-2));
    EXPECT_FALSE(is_even(-1));
}
