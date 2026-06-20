#include <gtest/gtest.h>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <random>
#include <vector>
#include <array>
#include <numeric>
#include <algorithm>
#include <limits>
#include "task07.hpp"

TEST(Templates, MyMax) {
    EXPECT_EQ(my_max(3, 5), 5);
    EXPECT_EQ(my_max(5, 3), 5);
    EXPECT_DOUBLE_EQ(my_max(2.5, 1.5), 2.5);
    EXPECT_EQ(my_max(std::string("a"), std::string("b")), "b");
}

TEST(Templates, ClampValue) {
    EXPECT_EQ(clamp_value(5, 0, 10), 5);
    EXPECT_EQ(clamp_value(-3, 0, 10), 0);
    EXPECT_EQ(clamp_value(42, 0, 10), 10);
    EXPECT_DOUBLE_EQ(clamp_value(1.5, 0.0, 1.0), 1.0);
}

TEST(Templates, SumVector) {
    EXPECT_EQ(sum_vector(std::vector<int>{1, 2, 3, 4}), 10);
    EXPECT_DOUBLE_EQ(sum_vector(std::vector<double>{1.5, 2.5}), 4.0);
    EXPECT_EQ(sum_vector(std::vector<int>{}), 0);
}

TEST(Templates, PairSwapped) {
    Pair<int, std::string> p(1, "one");
    auto q = p.swapped();
    EXPECT_EQ(q.first, "one");
    EXPECT_EQ(q.second, 1);
}

TEST(Templates, IsPowerOfTwo) {
    EXPECT_TRUE(is_power_of_two(1));
    EXPECT_TRUE(is_power_of_two(2));
    EXPECT_TRUE(is_power_of_two(1024));
    EXPECT_FALSE(is_power_of_two(0));
    EXPECT_FALSE(is_power_of_two(3));
    EXPECT_FALSE(is_power_of_two(-4));
}

// ---- 6. Matrix<T, Rows, Cols> (нетиповые параметры) ------------------------

TEST(Matrix, Dimensions) {
    EXPECT_EQ((Matrix<int, 2, 3>::rows()), 2u);
    EXPECT_EQ((Matrix<int, 2, 3>::cols()), 3u);
    // Размер зашит в тип, доступен на этапе компиляции:
    static_assert(Matrix<double, 4, 5>::rows() == 4);
    static_assert(Matrix<double, 4, 5>::cols() == 5);
}

TEST(Matrix, AtReadWrite) {
    Matrix<int, 2, 2> m;
    m.at(0, 0) = 1;
    m.at(0, 1) = 2;
    m.at(1, 0) = 3;
    m.at(1, 1) = 4;
    EXPECT_EQ(m.at(0, 0), 1);
    EXPECT_EQ(m.at(0, 1), 2);
    EXPECT_EQ(m.at(1, 0), 3);
    EXPECT_EQ(m.at(1, 1), 4);
    // row-major: (1,0) лежит на data[2]
    EXPECT_EQ(m.data[2], 3);
}

TEST(Matrix, AtConst) {
    Matrix<int, 2, 2> m;
    m.at(1, 1) = 9;
    const Matrix<int, 2, 2>& cm = m;
    EXPECT_EQ(cm.at(1, 1), 9);
}

TEST(Matrix, AtOutOfRangeThrows) {
    Matrix<int, 2, 3> m;
    EXPECT_THROW(m.at(2, 0), std::out_of_range);  // строка вне границ
    EXPECT_THROW(m.at(0, 3), std::out_of_range);  // столбец вне границ
    EXPECT_NO_THROW(m.at(1, 2));                   // последний валидный элемент
}

TEST(Matrix, ElementwiseAdd) {
    Matrix<int, 2, 2> a;
    a.at(0, 0) = 1; a.at(0, 1) = 2; a.at(1, 0) = 3; a.at(1, 1) = 4;
    Matrix<int, 2, 2> b;
    b.at(0, 0) = 10; b.at(0, 1) = 20; b.at(1, 0) = 30; b.at(1, 1) = 40;

    Matrix<int, 2, 2> c = a + b;
    EXPECT_EQ(c.at(0, 0), 11);
    EXPECT_EQ(c.at(0, 1), 22);
    EXPECT_EQ(c.at(1, 0), 33);
    EXPECT_EQ(c.at(1, 1), 44);
    // исходные матрицы не должны измениться
    EXPECT_EQ(a.at(0, 0), 1);
    EXPECT_EQ(b.at(0, 0), 10);
}

// ---- 7. Специализация type_name (bool / указатели) ------------------------

TEST(TypeName, PrimaryTemplate) {
    EXPECT_STREQ(type_name(42), "other");
    EXPECT_STREQ(type_name(3.14), "other");
    EXPECT_STREQ(type_name(std::string("hi")), "other");
}

TEST(TypeName, BoolSpecialization) {
    EXPECT_STREQ(type_name(true), "bool");
    EXPECT_STREQ(type_name(false), "bool");
    // важно: bool не должен срабатывать как "other"
    bool b = true;
    EXPECT_STREQ(type_name(b), "bool");
}

TEST(TypeName, PointerOverload) {
    int x = 5;
    int* p = &x;
    EXPECT_STREQ(type_name(p), "pointer");
    const char* s = "abc";
    EXPECT_STREQ(type_name(s), "pointer");
    double* dp = nullptr;
    EXPECT_STREQ(type_name(dp), "pointer");
}

// ---- 8. compile-time Pow<Base, Exp> ---------------------------------------

TEST(Pow, BasicValues) {
    EXPECT_EQ((Pow<2, 0>::value), 1);
    EXPECT_EQ((Pow<2, 1>::value), 2);
    EXPECT_EQ((Pow<2, 10>::value), 1024);
    EXPECT_EQ((Pow<3, 4>::value), 81);
    EXPECT_EQ((Pow<5, 3>::value), 125);
    EXPECT_EQ((Pow<7, 0>::value), 1);
}

// Используем результат как РАЗМЕР массива: это работает только если значение —
// настоящая compile-time константа. Когда задание решено правильно, размер == 16.
template <unsigned N>
constexpr std::size_t pow_as_array_size() {
    std::array<int, Pow<2, N>::value> arr{};  // размер берётся из шаблона на этапе компиляции
    return arr.size();
}

TEST(Pow, IsCompileTime) {
    // pow_v — variable template поверх Pow.
    EXPECT_EQ((pow_v<3, 3>), 27);
    // Значение реально использовано как размер массива => оно compile-time.
    EXPECT_EQ(pow_as_array_size<4>(), 16u);  // Pow<2,4> == 16
    EXPECT_EQ(pow_as_array_size<5>(), 32u);  // Pow<2,5> == 32
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Эти тесты прогоняют сотни сгенерированных входов и проверяют ИНВАРИАНТЫ
// (сравнение с оракулом из <algorithm>/<numeric>, round-trip, перестановка,
// идемпотентность, границы) плюс явные крайние случаи. Генератор детерминирован
// (std::mt19937 с фиксированным сидом) — CI не флакает.

// ---- my_max: оракул std::max, выбор одного из аргументов, коммутативность ----

TEST(MyMaxProps, MatchesStdMaxInts) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 400; ++i) {
        int a = dist(rng), b = dist(rng);
        int m = my_max(a, b);
        // оракул: совпадает со std::max
        EXPECT_EQ(m, std::max(a, b));
        // результат — это один из аргументов и не меньше обоих
        EXPECT_TRUE(m == a || m == b);
        EXPECT_GE(m, a);
        EXPECT_GE(m, b);
        // коммутативность по значению результата
        EXPECT_EQ(my_max(a, b), my_max(b, a));
    }
}

TEST(MyMaxProps, MatchesStdMaxDoublesAndEdges) {
    std::mt19937 rng(0x1234567u);
    std::uniform_real_distribution<double> dist(-1e6, 1e6);
    for (int i = 0; i < 300; ++i) {
        double a = dist(rng), b = dist(rng);
        EXPECT_DOUBLE_EQ(my_max(a, b), std::max(a, b));
    }
    // крайние случаи: равные аргументы и предельные значения int
    EXPECT_EQ(my_max(7, 7), 7);
    constexpr int hi = std::numeric_limits<int>::max();
    constexpr int lo = std::numeric_limits<int>::min();
    EXPECT_EQ(my_max(hi, lo), hi);
    EXPECT_EQ(my_max(lo, hi), hi);
    EXPECT_EQ(my_max(hi, hi), hi);
}

// ---- clamp_value: оракул std::clamp, попадание в [lo,hi], идемпотентность ----

TEST(ClampValueProps, MatchesStdClampAndInRange) {
    std::mt19937 rng(0xABCDEFu);
    std::uniform_int_distribution<int> dist(-500, 500);
    for (int i = 0; i < 400; ++i) {
        int lo = dist(rng), hi = dist(rng);
        if (lo > hi) std::swap(lo, hi);  // контракт clamp требует lo <= hi
        int v = dist(rng);
        int r = clamp_value(v, lo, hi);
        // оракул
        EXPECT_EQ(r, std::clamp(v, lo, hi));
        // результат всегда внутри диапазона
        EXPECT_GE(r, lo);
        EXPECT_LE(r, hi);
        // идемпотентность: повторное зажатие ничего не меняет
        EXPECT_EQ(clamp_value(r, lo, hi), r);
        // если v уже в диапазоне — это тождество
        if (v >= lo && v <= hi) EXPECT_EQ(r, v);
    }
}

TEST(ClampValueProps, BoundaryAndDegenerateRange) {
    // вырожденный диапазон lo == hi: всё схлопывается в одну точку
    std::mt19937 rng(0x55AA55u);
    std::uniform_int_distribution<int> dist(-1000, 1000);
    for (int i = 0; i < 200; ++i) {
        int p = dist(rng);
        int v = dist(rng);
        EXPECT_EQ(clamp_value(v, p, p), p);
    }
    // явные границы
    EXPECT_EQ(clamp_value(0, 0, 10), 0);    // нижняя граница
    EXPECT_EQ(clamp_value(10, 0, 10), 10);  // верхняя граница
    EXPECT_EQ(clamp_value(-1, 0, 10), 0);
    EXPECT_EQ(clamp_value(11, 0, 10), 10);
}

// ---- sum_vector: оракул std::accumulate, перестановка, аддитивность ----

TEST(SumVectorProps, MatchesAccumulateAndPermutationInvariant) {
    std::mt19937 rng(0xDEADBEEFu);
    std::uniform_int_distribution<int> lenDist(0, 64);
    std::uniform_int_distribution<int> valDist(-1000, 1000);
    for (int it = 0; it < 300; ++it) {
        std::size_t n = static_cast<std::size_t>(lenDist(rng));
        std::vector<int> v(n);
        for (auto& x : v) x = valDist(rng);

        long long oracle = std::accumulate(v.begin(), v.end(), 0LL);
        long long got = sum_vector(v);
        // оракул: совпадает с std::accumulate (на маленьких длинах overflow нет)
        EXPECT_EQ(got, oracle);

        // перестановка элементов не меняет сумму
        std::vector<int> shuffled = v;
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        EXPECT_EQ(sum_vector(shuffled), got);
    }
}

TEST(SumVectorProps, AdditiveOverConcatAndEmpty) {
    std::mt19937 rng(0x0BADF00Du);
    std::uniform_int_distribution<int> lenDist(0, 32);
    std::uniform_int_distribution<int> valDist(-500, 500);
    for (int it = 0; it < 200; ++it) {
        std::vector<int> a(static_cast<std::size_t>(lenDist(rng)));
        std::vector<int> b(static_cast<std::size_t>(lenDist(rng)));
        for (auto& x : a) x = valDist(rng);
        for (auto& x : b) x = valDist(rng);
        std::vector<int> ab = a;
        ab.insert(ab.end(), b.begin(), b.end());
        // сумма конкатенации == сумма частей
        EXPECT_EQ(sum_vector(ab), sum_vector(a) + sum_vector(b));
    }
    // явные крайние случаи
    EXPECT_EQ(sum_vector(std::vector<int>{}), 0);          // пустой -> T{}
    EXPECT_EQ(sum_vector(std::vector<int>{42}), 42);       // один элемент
    EXPECT_DOUBLE_EQ(sum_vector(std::vector<double>{}), 0.0);
}

// ---- Pair::swapped: round-trip и обмен полей ----

TEST(PairProps, SwappedRoundTripAndFields) {
    std::mt19937 rng(0xFEEDC0DEu);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int i = 0; i < 300; ++i) {
        int a = dist(rng), b = dist(rng);
        Pair<int, int> p(a, b);
        auto s = p.swapped();
        // поля переставлены
        EXPECT_EQ(s.first, b);
        EXPECT_EQ(s.second, a);
        // swapped(swapped(x)) == x (двойной обмен = тождество)
        auto back = s.swapped();
        EXPECT_EQ(back.first, a);
        EXPECT_EQ(back.second, b);
    }
}

TEST(PairProps, SwappedHeterogeneousTypes) {
    std::mt19937 rng(0x99AA77u);
    std::uniform_int_distribution<int> dist(0, 1000000);
    for (int i = 0; i < 200; ++i) {
        int k = dist(rng);
        std::string v = "v" + std::to_string(dist(rng));
        Pair<int, std::string> p(k, v);
        auto s = p.swapped();        // Pair<std::string, int>
        EXPECT_EQ(s.first, v);
        EXPECT_EQ(s.second, k);
        // тип после двойного swap снова Pair<int,std::string>
        static_assert(std::is_same_v<decltype(s.swapped()), Pair<int, std::string>>);
        auto back = s.swapped();
        EXPECT_EQ(back.first, k);
        EXPECT_EQ(back.second, v);
    }
}

// ---- is_power_of_two: независимый оракул через подсчёт битов ----

namespace {
// Оракул: x — положительная степень двойки, если у него ровно один установленный бит.
bool oracle_pow2(long long x) {
    if (x <= 0) return false;
    int bits = 0;
    unsigned long long u = static_cast<unsigned long long>(x);
    while (u) { bits += static_cast<int>(u & 1ULL); u >>= 1; }
    return bits == 1;
}
}  // namespace

TEST(IsPowerOfTwoProps, MatchesBitCountOracle) {
    std::mt19937 rng(0x2718281u);
    std::uniform_int_distribution<long long> dist(-1000000, 1000000);
    for (int i = 0; i < 400; ++i) {
        long long x = dist(rng);
        EXPECT_EQ(is_power_of_two(x), oracle_pow2(x));
    }
}

TEST(IsPowerOfTwoProps, AllPowersTrueNeighborsFalse) {
    // Все степени двойки, влезающие в long long, должны давать true.
    for (int e = 0; e < 62; ++e) {
        long long p = 1LL << e;
        EXPECT_TRUE(is_power_of_two(p)) << "2^" << e;
        // p+1 (>=2) и p-1 (для e>=2) — не степени двойки
        if (p >= 2) {  // 2 -> 3 не степень
            EXPECT_FALSE(is_power_of_two(p + 1)) << "2^" << e << "+1";
        }
        if (e >= 2) {  // 4-1=3, 8-1=7 ... не степени двойки
            EXPECT_FALSE(is_power_of_two(p - 1)) << "2^" << e << "-1";
        }
    }
    // явные крайние случаи
    EXPECT_FALSE(is_power_of_two(0));
    EXPECT_FALSE(is_power_of_two(-1));
    EXPECT_FALSE(is_power_of_two(-1024));
    EXPECT_TRUE(is_power_of_two(1));
}

// ---- Matrix: round-trip at()/row-major, сложение (оракул, коммутативность) ----

TEST(MatrixProps, AtRoundTripRowMajor) {
    constexpr std::size_t R = 4, C = 5;
    std::mt19937 rng(0x13572468u);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int it = 0; it < 200; ++it) {
        Matrix<int, R, C> m;
        int ref[R * C];
        // записываем случайные значения
        for (std::size_t r = 0; r < R; ++r)
            for (std::size_t c = 0; c < C; ++c) {
                int val = dist(rng);
                m.at(r, c) = val;
                ref[r * C + c] = val;
            }
        // читаем обратно: то, что записали (round-trip)
        for (std::size_t r = 0; r < R; ++r)
            for (std::size_t c = 0; c < C; ++c) {
                EXPECT_EQ(m.at(r, c), ref[r * C + c]);
                // row-major layout: at(r,c) == data[r*C + c]
                EXPECT_EQ(m.data[r * C + c], ref[r * C + c]);
            }
    }
}

TEST(MatrixProps, AddIsElementwiseCommutativeAndPure) {
    constexpr std::size_t R = 3, C = 3;
    std::mt19937 rng(0x0F0F0F0Fu);
    std::uniform_int_distribution<int> dist(-50000, 50000);
    for (int it = 0; it < 200; ++it) {
        Matrix<int, R, C> a, b;
        for (std::size_t i = 0; i < R * C; ++i) {
            a.data[i] = dist(rng);
            b.data[i] = dist(rng);
        }
        // сохраняем копии, чтобы проверить, что операнды не мутируют
        auto a0 = a.data, b0 = b.data;

        Matrix<int, R, C> c = a + b;
        for (std::size_t r = 0; r < R; ++r)
            for (std::size_t cc = 0; cc < C; ++cc)
                EXPECT_EQ(c.at(r, cc), a.at(r, cc) + b.at(r, cc));  // поэлементно

        // коммутативность сложения
        Matrix<int, R, C> d = b + a;
        EXPECT_TRUE(c == d);

        // чистота: a и b не изменились
        EXPECT_EQ(a.data, a0);
        EXPECT_EQ(b.data, b0);
    }
}

TEST(MatrixProps, AtThrowsOutOfRange) {
    constexpr std::size_t R = 3, C = 4;
    Matrix<int, R, C> m;
    std::mt19937 rng(0xCAFED00Du);
    std::uniform_int_distribution<std::size_t> rowOut(R, R + 50);
    std::uniform_int_distribution<std::size_t> colOut(C, C + 50);
    std::uniform_int_distribution<std::size_t> rowIn(0, R - 1);
    std::uniform_int_distribution<std::size_t> colIn(0, C - 1);
    for (int i = 0; i < 200; ++i) {
        // выход за строки -> std::out_of_range
        EXPECT_THROW(m.at(rowOut(rng), colIn(rng)), std::out_of_range);
        // выход за столбцы -> std::out_of_range
        EXPECT_THROW(m.at(rowIn(rng), colOut(rng)), std::out_of_range);
        // валидный индекс не бросает
        EXPECT_NO_THROW(m.at(rowIn(rng), colIn(rng)));
    }
    // явная угловая граница: последний валидный и первый невалидный
    EXPECT_NO_THROW(m.at(R - 1, C - 1));
    EXPECT_THROW(m.at(R, C - 1), std::out_of_range);
    EXPECT_THROW(m.at(R - 1, C), std::out_of_range);
}

// ---- Pow: оракул целочисленной степени, согласованность pow_v и рекурсии ----

TEST(PowProps, MatchesIterativeOracle) {
    // Pow<Base,Exp>::value считается на этапе компиляции, поэтому Base/Exp —
    // нетиповые параметры. Проверяем согласованность с итеративным оракулом
    // для фиксированного набора пар (значения compile-time, перечисляем явно).
    auto ipow = [](long long base, unsigned e) {
        long long r = 1;
        for (unsigned i = 0; i < e; ++i) r *= base;
        return r;
    };
    // основания и показатели, безопасные по переполнению long long
    EXPECT_EQ((Pow<2, 0>::value),  ipow(2, 0));
    EXPECT_EQ((Pow<2, 30>::value), ipow(2, 30));
    EXPECT_EQ((Pow<3, 20>::value), ipow(3, 20));
    EXPECT_EQ((Pow<5, 10>::value), ipow(5, 10));
    EXPECT_EQ((Pow<7, 5>::value),  ipow(7, 5));
    EXPECT_EQ((Pow<10, 9>::value), ipow(10, 9));
    EXPECT_EQ((Pow<1, 50>::value), ipow(1, 50));   // 1^n == 1
    EXPECT_EQ((Pow<-2, 7>::value), ipow(-2, 7));    // отрицательное основание, нечётный
    EXPECT_EQ((Pow<-2, 8>::value), ipow(-2, 8));    // отрицательное основание, чётный
    // согласованность variable template и рекурсивного шаблона
    EXPECT_EQ((pow_v<3, 20>), (Pow<3, 20>::value));
    EXPECT_EQ((pow_v<6, 11>), (Pow<6, 11>::value));
    // База рекурсии: Base^0 == 1
    EXPECT_EQ((Pow<123, 0>::value), 1);
}

TEST(PowProps, MultiplicativeStep) {
    // Инвариант рекурсии: Base^(e+1) == Base * Base^e (проверяем по цепочке).
    static_assert(Pow<2, 11>::value == 2 * Pow<2, 10>::value);
    static_assert(Pow<3, 8>::value  == 3 * Pow<3, 7>::value);
    static_assert(Pow<5, 6>::value  == 5 * Pow<5, 5>::value);
    EXPECT_EQ((Pow<2, 11>::value), (2 * Pow<2, 10>::value));
    EXPECT_EQ((Pow<3, 8>::value),  (3 * Pow<3, 7>::value));
    EXPECT_EQ((Pow<5, 6>::value),  (5 * Pow<5, 5>::value));
    SUCCEED();
}

// ============================================================================
//  Задание 9: концепт Addable, sum_all, variadic sum_args
// ============================================================================

// ---- 9a. Статические проверки концепта Addable -----------------------------

// Addable<int> и Addable<double> должны быть истинны.
static_assert(Addable<int>,    "int должен быть Addable");
static_assert(Addable<double>, "double должен быть Addable");
static_assert(Addable<std::string>, "std::string должен быть Addable (конкатенация)");

// Тип без operator+ не удовлетворяет концепту.
struct NoAdd { int x; };
static_assert(!Addable<NoAdd>, "NoAdd не должен быть Addable");

// ---- 9b. Фиксированные тесты sum_all ----------------------------------------

TEST(Addable, SumAllInt) {
    EXPECT_EQ(sum_all(std::vector<int>{1, 2, 3, 4}), 10);
    EXPECT_EQ(sum_all(std::vector<int>{}), 0);            // пустой → T{} = 0
    EXPECT_EQ(sum_all(std::vector<int>{42}), 42);         // один элемент
}

TEST(Addable, SumAllDouble) {
    EXPECT_DOUBLE_EQ(sum_all(std::vector<double>{1.5, 2.5, 0.5}), 4.5);
    EXPECT_DOUBLE_EQ(sum_all(std::vector<double>{}), 0.0);
}

TEST(Addable, SumAllString) {
    // std::string — Addable через конкатенацию
    std::vector<std::string> words{"foo", "bar", "baz"};
    EXPECT_EQ(sum_all(words), "foobarbaz");
    EXPECT_EQ(sum_all(std::vector<std::string>{}), "");
}

// ---- 9c. Фиксированные тесты sum_args ----------------------------------------

TEST(Addable, SumArgsInts) {
    EXPECT_EQ(sum_args(1, 2, 3), 6);
    EXPECT_EQ(sum_args(10), 10);          // один аргумент
    EXPECT_EQ(sum_args(0, 0, 0), 0);
}

TEST(Addable, SumArgsDoubles) {
    EXPECT_DOUBLE_EQ(sum_args(1.5, 2.5), 4.0);
    EXPECT_DOUBLE_EQ(sum_args(1.0, 2.0, 3.0, 4.0), 10.0);
}

TEST(Addable, SumArgsMixedPromotion) {
    // int + double → double через стандартные arithmetic promotions
    auto result = sum_args(1, 2.5, 3);
    static_assert(std::is_floating_point_v<decltype(result)>);
    EXPECT_DOUBLE_EQ(result, 6.5);
}

// ---- 9d. Seeded property-тест: sum_all(random vector) == std::accumulate ----

TEST(AddableProps, SumAllMatchesAccumulate) {
    std::mt19937 rng(0xB00BFACEu);
    std::uniform_int_distribution<int> lenDist(0, 60);
    std::uniform_int_distribution<int> valDist(-1000, 1000);
    for (int it = 0; it < 400; ++it) {
        std::size_t n = static_cast<std::size_t>(lenDist(rng));
        std::vector<int> v(n);
        for (auto& x : v) x = valDist(rng);
        long long oracle = std::accumulate(v.begin(), v.end(), 0LL);
        EXPECT_EQ(sum_all(v), static_cast<int>(oracle));
    }
}

TEST(AddableProps, SumAllDoubleMatchesAccumulate) {
    std::mt19937 rng(0xC001D00Du);
    std::uniform_int_distribution<int> lenDist(1, 50);
    std::uniform_real_distribution<double> valDist(-100.0, 100.0);
    for (int it = 0; it < 300; ++it) {
        std::size_t n = static_cast<std::size_t>(lenDist(rng));
        std::vector<double> v(n);
        for (auto& x : v) x = valDist(rng);
        double oracle = std::accumulate(v.begin(), v.end(), 0.0);
        // floating-point: допускаем небольшую разницу из-за порядка суммирования
        EXPECT_NEAR(sum_all(v), oracle, std::abs(oracle) * 1e-9 + 1e-9);
    }
}
