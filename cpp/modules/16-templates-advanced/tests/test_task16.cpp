#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <list>
#include <type_traits>
#include <random>
#include <numeric>
#include <iterator>
#include <cstddef>
#include <climits>
#include <sstream>
#include "task16.hpp"

// ---------------------------------------------------------------------
// Задание 1. variadic + fold
// ---------------------------------------------------------------------
TEST(Variadic, SumInts) {
    EXPECT_EQ(adv::sum(1, 2, 3, 4), 10);
    EXPECT_EQ(adv::sum(42), 42);
    EXPECT_EQ(adv::sum(-5, 5), 0);
}

TEST(Variadic, SumDoubles) {
    EXPECT_DOUBLE_EQ(adv::sum(1.5, 2.5, 1.0), 5.0);
}

TEST(Variadic, ToStringAll) {
    EXPECT_EQ(adv::to_string_all(1, 2, 3), "1, 2, 3");
    EXPECT_EQ(adv::to_string_all(42), "42");
    EXPECT_EQ(adv::to_string_all(7, 8), "7, 8");
}

TEST(Variadic, ToStringAllMixedNumeric) {
    // int + double + int -> std::to_string каждого, склейка через ", "
    EXPECT_EQ(adv::to_string_all(1, 2, 3, 4, 5), "1, 2, 3, 4, 5");
}

// ---------------------------------------------------------------------
// Задание 2. свои type traits
// ---------------------------------------------------------------------
TEST(Traits, RemoveConst) {
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<const int>, int>));
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<int>, int>));
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<const double>, double>));
    // const должен сниматься только с верхнего уровня
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<const char*>, const char*>));
}

TEST(Traits, IsPointer) {
    EXPECT_TRUE(adv::my_is_pointer_v<int*>);
    EXPECT_TRUE(adv::my_is_pointer_v<const char*>);
    EXPECT_TRUE(adv::my_is_pointer_v<double*>);
    EXPECT_FALSE(adv::my_is_pointer_v<int>);
    EXPECT_FALSE(adv::my_is_pointer_v<double>);
    EXPECT_FALSE(adv::my_is_pointer_v<std::string>);
}

TEST(Traits, IsSame) {
    EXPECT_TRUE((adv::my_is_same_v<int, int>));
    EXPECT_TRUE((adv::my_is_same_v<double, double>));
    EXPECT_FALSE((adv::my_is_same_v<int, double>));
    EXPECT_FALSE((adv::my_is_same_v<int, long>));
    EXPECT_FALSE((adv::my_is_same_v<const int, int>));
}

// ---------------------------------------------------------------------
// Задание 3. enable_if-перегрузка describe
// ---------------------------------------------------------------------
TEST(EnableIf, DescribeIntegral) {
    EXPECT_EQ(adv::describe(5), "integer");
    EXPECT_EQ(adv::describe(0), "integer");
    EXPECT_EQ(adv::describe(static_cast<long>(123)), "integer");
}

TEST(EnableIf, DescribeFloating) {
    EXPECT_EQ(adv::describe(3.14), "floating");
    EXPECT_EQ(adv::describe(2.0f), "floating");
}

// ---------------------------------------------------------------------
// Задание 4. CRTP Comparable
// ---------------------------------------------------------------------
namespace {
struct Money : adv::Comparable<Money> {
    int cents;
    explicit Money(int c) : cents(c) {}
    bool operator<(const Money& o) const { return cents < o.cents; }
    bool operator==(const Money& o) const { return cents == o.cents; }
};
}  // namespace

TEST(CRTP, DerivedOperators) {
    Money a{100}, b{200}, c{100};

    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != c);

    EXPECT_TRUE(b > a);
    EXPECT_FALSE(a > b);

    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(b <= a);

    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(a >= c);
    EXPECT_FALSE(a >= b);
}

// ---------------------------------------------------------------------
// Задание 5. tag dispatch
// ---------------------------------------------------------------------
TEST(TagDispatch, RandomAccessUsesFastPath) {
    adv::fast_path_calls() = 0;
    std::vector<int> v{10, 20, 30, 40, 50};
    EXPECT_EQ(adv::distance_dispatch(v.begin(), v.end()), 5);
    EXPECT_EQ(adv::fast_path_calls(), 1);  // быстрый путь сработал
}

TEST(TagDispatch, ListUsesSlowPath) {
    adv::fast_path_calls() = 0;
    std::list<int> l{1, 2, 3, 4};
    EXPECT_EQ(adv::distance_dispatch(l.begin(), l.end()), 4);
    EXPECT_EQ(adv::fast_path_calls(), 0);  // быстрый путь НЕ должен был сработать
}

TEST(TagDispatch, EmptyRange) {
    adv::fast_path_calls() = 0;
    std::vector<int> v;
    EXPECT_EQ(adv::distance_dispatch(v.begin(), v.end()), 0);
    std::list<int> l;
    EXPECT_EQ(adv::distance_dispatch(l.begin(), l.end()), 0);
}

// =====================================================================
// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо фиксированных примеров прогоняем сотни случайных входов и
// проверяем ИНВАРИАНТЫ (оракул против std::, round-trip, антисимметрия,
// согласованность операторов сравнения) плюс явные крайние случаи.
// Один и тот же сид std::mt19937 => CI не флакает.
// =====================================================================

namespace {

// Хелпер: посчитать, сколько раз подстрока встречается в строке.
std::size_t count_substr(const std::string& s, const std::string& sub) {
    if (sub.empty()) return 0;
    std::size_t n = 0, pos = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) {
        ++n;
        pos += sub.size();
    }
    return n;
}

// CRTP-обёртка над int для проверки оператора сравнения через миксин.
struct Num : adv::Comparable<Num> {
    int v;
    explicit Num(int x) : v(x) {}
    bool operator<(const Num& o) const { return v < o.v; }
    bool operator==(const Num& o) const { return v == o.v; }
};

}  // namespace

// ---------------------------------------------------------------------
// Задание 1. sum — оракул против std::accumulate + коммутативность.
// ---------------------------------------------------------------------
TEST(VariadicProps, SumFixedAritiesMatchAccumulate) {
    std::mt19937 rng(0xC0FFEEu);
    // sum принимает РОВНО фиксированное число аргументов в каждом вызове;
    // прогоняем много случайных пятёрок и сверяем со std::accumulate.
    std::uniform_int_distribution<int> dist(-1000, 1000);
    for (int iter = 0; iter < 400; ++iter) {
        int a = dist(rng), b = dist(rng), c = dist(rng), d = dist(rng), e = dist(rng);
        long long oracle = static_cast<long long>(a) + b + c + d + e;
        EXPECT_EQ(adv::sum(a, b, c, d, e), static_cast<int>(oracle));
        // Коммутативность сложения: переставленный порядок даёт тот же результат.
        EXPECT_EQ(adv::sum(a, b, c, d, e), adv::sum(e, d, c, b, a));
        // Один аргумент => тождество.
        EXPECT_EQ(adv::sum(a), a);
        // Пара => обычная сумма.
        EXPECT_EQ(adv::sum(a, b), a + b);
    }
}

TEST(VariadicProps, SumDoublesMatchAccumulate) {
    std::mt19937 rng(0x5EEDu);
    std::uniform_real_distribution<double> dist(-1e3, 1e3);
    for (int iter = 0; iter < 300; ++iter) {
        double a = dist(rng), b = dist(rng), c = dist(rng);
        double oracle = a + b + c;
        EXPECT_DOUBLE_EQ(adv::sum(a, b, c), oracle);
        EXPECT_DOUBLE_EQ(adv::sum(a), a);
    }
}

TEST(VariadicProps, SumEdgeCases) {
    // Нули и крайние значения (без переполнения за счёт long long-оракула).
    EXPECT_EQ(adv::sum(0, 0, 0), 0);
    EXPECT_EQ(adv::sum(0), 0);
    // INT_MAX/2 + INT_MAX/2 не переполняет int.
    int half = INT_MAX / 2;
    EXPECT_EQ(adv::sum(half, half), half + half);
    EXPECT_EQ(adv::sum(INT_MIN, 0), INT_MIN);
}

// ---------------------------------------------------------------------
// Задание 1. to_string_all — round-trip: разбираем строку обратно в числа.
// ---------------------------------------------------------------------
TEST(VariadicProps, ToStringAllRoundTripFiveInts) {
    std::mt19937 rng(0xABCDEFu);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int iter = 0; iter < 400; ++iter) {
        int a = dist(rng), b = dist(rng), c = dist(rng), d = dist(rng), e = dist(rng);
        std::vector<int> in{a, b, c, d, e};
        std::string s = adv::to_string_all(a, b, c, d, e);

        // Инвариант: ровно (n-1) разделителей ", " при n аргументах.
        EXPECT_EQ(count_substr(s, ", "), in.size() - 1);

        // Round-trip: распарсим обратно и сравним с исходными числами.
        std::vector<int> out;
        std::stringstream ss(s);
        std::string tok;
        while (std::getline(ss, tok, ',')) {
            // У токенов после запятой есть ведущий пробел из разделителя ", ".
            std::size_t p = tok.find_first_not_of(' ');
            if (p != std::string::npos) tok = tok.substr(p);
            out.push_back(std::stoi(tok));
        }
        EXPECT_EQ(out, in);
    }
}

TEST(VariadicProps, ToStringAllSingleHasNoSeparator) {
    std::mt19937 rng(0x1234u);
    std::uniform_int_distribution<int> dist(-100000, 100000);
    for (int iter = 0; iter < 200; ++iter) {
        int a = dist(rng);
        std::string s = adv::to_string_all(a);
        EXPECT_EQ(s, std::to_string(a));        // ровно одно число, без ", "
        EXPECT_EQ(count_substr(s, ", "), 0u);
    }
}

// ---------------------------------------------------------------------
// Задание 2. type traits — оракул против <type_traits> на многих типах.
// (Типы compile-time, поэтому "много входов" — это широкий набор пар.)
// ---------------------------------------------------------------------
TEST(TraitsProps, RemoveConstMatchesStd) {
#define CHK_RC(T) \
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<T>, std::remove_const_t<T>>))
    CHK_RC(int);
    CHK_RC(const int);
    CHK_RC(double);
    CHK_RC(const double);
    CHK_RC(char);
    CHK_RC(const char);
    CHK_RC(long);
    CHK_RC(const long);
    CHK_RC(const char*);        // снимается только верхний const => указатель не меняется
    CHK_RC(char* const);        // const на самом указателе снимается
    CHK_RC(const volatile int); // верхний const снят, volatile остаётся
    CHK_RC(std::string);
    CHK_RC(const std::string);
#undef CHK_RC
}

TEST(TraitsProps, IsPointerMatchesStd) {
#define CHK_PTR(T) EXPECT_EQ(adv::my_is_pointer_v<T>, std::is_pointer_v<T>)
    CHK_PTR(int);
    CHK_PTR(int*);
    CHK_PTR(const int*);
    CHK_PTR(int**);
    CHK_PTR(double);
    CHK_PTR(double*);
    CHK_PTR(const char*);
    CHK_PTR(char);
    CHK_PTR(std::string);
    CHK_PTR(std::string*);
    CHK_PTR(void*);
    CHK_PTR(long);
    CHK_PTR(long*);
#undef CHK_PTR
}

TEST(TraitsProps, IsSameMatchesStdAndReflexive) {
#define CHK_SAME(A, B) EXPECT_EQ((adv::my_is_same_v<A, B>), (std::is_same_v<A, B>))
    // Рефлексивность: тип сам с собой всегда true.
    CHK_SAME(int, int);
    CHK_SAME(double, double);
    CHK_SAME(std::string, std::string);
    CHK_SAME(int*, int*);
    // Различающиеся типы => false (и совпадает со std).
    CHK_SAME(int, double);
    CHK_SAME(int, long);
    CHK_SAME(const int, int);
    CHK_SAME(int, int*);
    CHK_SAME(int*, double*);
    CHK_SAME(char, signed char);
    CHK_SAME(float, double);
    // Симметрия: my_is_same<A,B> == my_is_same<B,A>.
    EXPECT_EQ((adv::my_is_same_v<int, double>), (adv::my_is_same_v<double, int>));
    EXPECT_EQ((adv::my_is_same_v<int, int>), (adv::my_is_same_v<int, int>));
#undef CHK_SAME
}

// ---------------------------------------------------------------------
// Задание 3. describe — для случайных целых "integer", для дробных "floating".
// ---------------------------------------------------------------------
TEST(EnableIfProps, DescribeIntegralRandom) {
    std::mt19937 rng(0xDEADBEEFu);
    std::uniform_int_distribution<int> idist(INT_MIN, INT_MAX);
    std::uniform_int_distribution<long long> ldist(LLONG_MIN, LLONG_MAX);
    for (int iter = 0; iter < 300; ++iter) {
        int i = idist(rng);
        long long l = ldist(rng);
        EXPECT_EQ(adv::describe(i), "integer");
        EXPECT_EQ(adv::describe(l), "integer");
        EXPECT_EQ(adv::describe(static_cast<short>(i)), "integer");
        EXPECT_EQ(adv::describe(static_cast<unsigned>(i)), "integer");
    }
}

TEST(EnableIfProps, DescribeFloatingRandom) {
    std::mt19937 rng(0xFEEDu);
    std::uniform_real_distribution<double> ddist(-1e9, 1e9);
    for (int iter = 0; iter < 300; ++iter) {
        double d = ddist(rng);
        float f = static_cast<float>(d);
        EXPECT_EQ(adv::describe(d), "floating");
        EXPECT_EQ(adv::describe(f), "floating");
    }
    // Крайние значения.
    EXPECT_EQ(adv::describe(0.0), "floating");
    EXPECT_EQ(adv::describe(0), "integer");
    EXPECT_EQ(adv::describe('a'), "integer");   // char — целочисленный
    EXPECT_EQ(adv::describe(true), "integer");  // bool — целочисленный
}

// ---------------------------------------------------------------------
// Задание 4. CRTP Comparable — операторы согласованы с оракулом (int).
// ---------------------------------------------------------------------
TEST(CRTPProps, OperatorsMatchIntOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(-50, 50);  // узкий диапазон => много равных пар
    for (int iter = 0; iter < 500; ++iter) {
        int x = dist(rng), y = dist(rng);
        Num a{x}, b{y};

        // Каждый производный оператор обязан совпасть со встроенным сравнением int.
        EXPECT_EQ(a != b, x != y);
        EXPECT_EQ(a > b,  x > y);
        EXPECT_EQ(a <= b, x <= y);
        EXPECT_EQ(a >= b, x >= y);
        // Базовые (от пользователя) для полноты картины.
        EXPECT_EQ(a < b,  x < y);
        EXPECT_EQ(a == b, x == y);

        // Внутренняя согласованность: > это отрицание <=, >= это отрицание <.
        EXPECT_EQ(a > b, !(a <= b));
        EXPECT_EQ(a >= b, !(a < b));
        // != это отрицание ==.
        EXPECT_EQ(a != b, !(a == b));
        // Антисимметрия: если a<b, то b>a и не a>b.
        if (a < b) {
            EXPECT_TRUE(b > a);
            EXPECT_FALSE(a > b);
        }
    }
}

TEST(CRTPProps, ReflexiveEqualAndBounds) {
    // Рефлексивность для крайних значений: x всегда <= и >= самому себе, не !=, не >.
    for (int x : {INT_MIN, -1, 0, 1, INT_MAX}) {
        Num a{x}, b{x};
        EXPECT_TRUE(a <= b);
        EXPECT_TRUE(a >= b);
        EXPECT_FALSE(a != b);
        EXPECT_FALSE(a > b);
    }
}

// ---------------------------------------------------------------------
// Задание 5. distance_dispatch — оракул против std::distance + счётчик.
// ---------------------------------------------------------------------
TEST(TagDispatchProps, VectorMatchesStdDistanceAndFastPath) {
    std::mt19937 rng(0xBADF00Du);
    std::uniform_int_distribution<int> ndist(0, 300);
    for (int iter = 0; iter < 300; ++iter) {
        int n = ndist(rng);
        std::vector<int> v(static_cast<std::size_t>(n));
        std::iota(v.begin(), v.end(), 0);

        adv::fast_path_calls() = 0;
        auto got = adv::distance_dispatch(v.begin(), v.end());
        EXPECT_EQ(got, std::distance(v.begin(), v.end()));  // оракул
        EXPECT_EQ(got, static_cast<std::ptrdiff_t>(n));
        // vector — random-access => быстрый путь сработал ровно один раз.
        EXPECT_EQ(adv::fast_path_calls(), 1);

        // Round-trip на подотрезке: dist(begin, begin+k) == k.
        if (n > 0) {
            std::uniform_int_distribution<int> kdist(0, n);
            int k = kdist(rng);
            adv::fast_path_calls() = 0;
            EXPECT_EQ(adv::distance_dispatch(v.begin(), v.begin() + k),
                      static_cast<std::ptrdiff_t>(k));
            EXPECT_EQ(adv::fast_path_calls(), 1);
        }
    }
}

TEST(TagDispatchProps, ListMatchesStdDistanceAndSlowPath) {
    std::mt19937 rng(0x0FF1CEu);
    std::uniform_int_distribution<int> ndist(0, 200);
    for (int iter = 0; iter < 300; ++iter) {
        int n = ndist(rng);
        std::list<int> l;
        for (int i = 0; i < n; ++i) l.push_back(i);

        adv::fast_path_calls() = 0;
        auto got = adv::distance_dispatch(l.begin(), l.end());
        EXPECT_EQ(got, std::distance(l.begin(), l.end()));  // оракул
        EXPECT_EQ(got, static_cast<std::ptrdiff_t>(n));
        // list — bidirectional, НЕ random-access => быстрый путь не вызывается.
        EXPECT_EQ(adv::fast_path_calls(), 0);
    }
}

TEST(TagDispatchProps, EmptyAndSingleEdge) {
    // Пустые и одноэлементные диапазоны — крайние случаи.
    std::vector<int> ev;
    std::list<int> el;
    adv::fast_path_calls() = 0;
    EXPECT_EQ(adv::distance_dispatch(ev.begin(), ev.end()), 0);
    EXPECT_EQ(adv::distance_dispatch(el.begin(), el.end()), 0);
    EXPECT_EQ(adv::fast_path_calls(), 1);  // только vector прошёл быстрый путь

    std::vector<int> sv{7};
    std::list<int> sl{7};
    adv::fast_path_calls() = 0;
    EXPECT_EQ(adv::distance_dispatch(sv.begin(), sv.end()), 1);
    EXPECT_EQ(adv::distance_dispatch(sl.begin(), sl.end()), 1);
    EXPECT_EQ(adv::fast_path_calls(), 1);
}

// =====================================================================
// Задание 6. all_of_pred — fold по &&
// =====================================================================

namespace {
auto is_even = [](int x) { return x % 2 == 0; };
auto is_positive = [](int x) { return x > 0; };
}  // namespace

// --- Фиксированные краевые случаи ---

TEST(AllOfFold, AllEvenTrue) {
    EXPECT_TRUE(adv::all_of_pred(is_even, 2, 4, 6));
}

TEST(AllOfFold, OneOddFalse) {
    EXPECT_FALSE(adv::all_of_pred(is_even, 2, 4, 5));
    EXPECT_FALSE(adv::all_of_pred(is_even, 1, 2, 4));
    EXPECT_FALSE(adv::all_of_pred(is_even, 1));
}

TEST(AllOfFold, EmptyPackTrue) {
    // Пустой пакет — vacuous truth: предикат не нарушен ни разу.
    EXPECT_TRUE(adv::all_of_pred(is_even));
}

TEST(AllOfFold, SingleElementBothWays) {
    EXPECT_TRUE(adv::all_of_pred(is_even, 0));
    EXPECT_TRUE(adv::all_of_pred(is_positive, 42));
    EXPECT_FALSE(adv::all_of_pred(is_positive, -1));
}

TEST(AllOfFold, MixedTypes) {
    // Предикат работает с любым типом, совместимым с вызовом.
    auto is_nonzero = [](auto x) { return x != 0; };
    EXPECT_TRUE(adv::all_of_pred(is_nonzero, 1, 2, 3, 4));
    EXPECT_FALSE(adv::all_of_pred(is_nonzero, 1, 0, 3));
}

// --- Seeded property-тест: оракул — ручная проверка по вектору ---

TEST(AllOfFoldProps, MatchesManualCheckSeeded) {
    // Инвариант: all_of_pred(p, a0..a4) == (p(a0) && p(a1) && p(a2) && p(a3) && p(a4)).
    // Используем фиксированный сид — результат детерминирован, CI не флакает.
    std::mt19937 rng(0xA110F01Du);
    std::uniform_int_distribution<int> dist(-20, 20);

    for (int iter = 0; iter < 500; ++iter) {
        int a = dist(rng), b = dist(rng), c = dist(rng), d = dist(rng), e = dist(rng);

        // Оракул: вручную И-им результаты предиката для всех пяти значений.
        bool oracle_even     = is_even(a) && is_even(b) && is_even(c) && is_even(d) && is_even(e);
        bool oracle_positive = is_positive(a) && is_positive(b) && is_positive(c)
                               && is_positive(d) && is_positive(e);

        EXPECT_EQ(adv::all_of_pred(is_even,     a, b, c, d, e), oracle_even);
        EXPECT_EQ(adv::all_of_pred(is_positive, a, b, c, d, e), oracle_positive);

        // Инвариант согласованности: all_of_pred из одного элемента == p(элемент).
        EXPECT_EQ(adv::all_of_pred(is_even, a),     is_even(a));
        EXPECT_EQ(adv::all_of_pred(is_positive, a), is_positive(a));
    }
}

TEST(AllOfFoldProps, ShortCircuitSemantics) {
    // Если первый аргумент уже ложен — результат false независимо от остальных.
    // Проверяем через разные позиции «плохого» элемента.
    std::mt19937 rng(0xBAD1dea0u);
    std::uniform_int_distribution<int> even_dist(-50, 50);

    for (int iter = 0; iter < 200; ++iter) {
        // Генерируем чётные числа — все подходят под is_even.
        int e0 = even_dist(rng) * 2;
        int e1 = even_dist(rng) * 2;
        int e2 = even_dist(rng) * 2;
        // Нечётный элемент в разных позициях должен давать false.
        EXPECT_FALSE(adv::all_of_pred(is_even, 1, e0, e1));
        EXPECT_FALSE(adv::all_of_pred(is_even, e0, 1, e1));
        EXPECT_FALSE(adv::all_of_pred(is_even, e0, e1, 1));
        // Все чётные — true.
        EXPECT_TRUE(adv::all_of_pred(is_even, e0, e1, e2));
    }
}
