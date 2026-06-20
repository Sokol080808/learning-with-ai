#include <gtest/gtest.h>
#include "task10.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <map>
#include <set>
#include <string_view>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <memory>

TEST(Modern, MinMax) {
    auto [lo, hi] = minmax_of({3, 1, 4, 1, 5, 9, 2});
    EXPECT_EQ(lo, 1);
    EXPECT_EQ(hi, 9);
}

TEST(Modern, ColorName) {
    EXPECT_EQ(color_name(Color::Red), "Red");
    EXPECT_EQ(color_name(Color::Green), "Green");
    EXPECT_EQ(color_name(Color::Blue), "Blue");
}

TEST(Modern, SquareConstexpr) {
    // Использование в constexpr-контексте: компилируется, только если square_ce —
    // корректная constexpr-функция. Значение проверяем в рантайме, чтобы недописанный
    // стаб не ломал СБОРКУ всего проекта (он просто даст красный тест).
    constexpr int v5 = square_ce(5);
    constexpr int v0 = square_ce(0);
    EXPECT_EQ(v5, 25);
    EXPECT_EQ(v0, 0);
    EXPECT_EQ(square_ce(7), 49);  // и в рантайме тоже
}

TEST(Modern, EvensSquared) {
    EXPECT_EQ(evens_squared({1, 2, 3, 4, 5, 6}), (std::vector<int>{4, 16, 36}));
    EXPECT_TRUE(evens_squared({1, 3, 5}).empty());
}

TEST(Modern, SumValues) {
    std::map<std::string, int> m{{"a", 1}, {"b", 2}, {"c", 3}};
    EXPECT_EQ(sum_values(m), 6);
    EXPECT_EQ(sum_values({}), 0);
}

// --- Задание 6: split_view ---

TEST(Modern, SplitViewBasic) {
    auto parts = split_view("a,b,c", ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(Modern, SplitViewEmptyPieces) {
    auto parts = split_view(",a,", ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "a");
    EXPECT_EQ(parts[2], "");
}

TEST(Modern, SplitViewSingleAndEmpty) {
    auto one = split_view("hello", ',');
    ASSERT_EQ(one.size(), 1u);
    EXPECT_EQ(one[0], "hello");

    auto empty = split_view("", ',');
    ASSERT_EQ(empty.size(), 1u);
    EXPECT_EQ(empty[0], "");
}

TEST(Modern, SplitViewNoCopy) {
    // Куски должны СМОТРЕТЬ внутрь исходной строки, а не быть копиями.
    std::string src = "alpha:beta:gamma";
    auto parts = split_view(src, ':');
    ASSERT_EQ(parts.size(), 3u);
    // Адрес первого символа куска совпадает с адресом внутри src — значит копии нет.
    EXPECT_EQ(parts[0].data(), src.data());
    EXPECT_EQ(parts[1].data(), src.data() + 6);
}

// --- Задание 7: ScopeGuard ---

TEST(Modern, ScopeGuardRunsOnExit) {
    int calls = 0;
    {
        ScopeGuard g([&] { ++calls; });
        EXPECT_EQ(calls, 0);  // ещё не сработал — мы внутри блока
    }
    EXPECT_EQ(calls, 1);  // сработал ровно один раз при выходе
}

TEST(Modern, ScopeGuardRunsOnThrow) {
    int calls = 0;
    try {
        ScopeGuard g([&] { ++calls; });
        throw std::runtime_error("boom");
    } catch (const std::runtime_error&) {
        // исключение поймали — но guard уже должен был отработать
    }
    EXPECT_EQ(calls, 1);
}

TEST(Modern, ScopeGuardDismiss) {
    int dismissed = 0;
    int kept = 0;
    {
        ScopeGuard g_dismissed([&] { ++dismissed; });
        ScopeGuard g_kept([&] { ++kept; });
        g_dismissed.dismiss();  // отменяем только первый
    }
    EXPECT_EQ(dismissed, 0);  // отменён — не сработал
    EXPECT_EQ(kept, 1);       // не отменён — сработал ровно один раз
}

TEST(Modern, ScopeGuardFactory) {
    int calls = 0;
    {
        auto g = make_scope_guard([&] { ++calls; });
    }
    EXPECT_EQ(calls, 1);
}

// --- Задание 8: divmod + [[nodiscard]] + structured bindings ---

TEST(Modern, DivmodBasic) {
    auto [q, r] = divmod(17, 5);
    EXPECT_EQ(q, 3);
    EXPECT_EQ(r, 2);
}

TEST(Modern, DivmodExact) {
    auto [q, r] = divmod(20, 4);
    EXPECT_EQ(q, 5);
    EXPECT_EQ(r, 0);
}

TEST(Modern, DivmodNegative) {
    auto [q, r] = divmod(-7, 2);
    // C++ truncation toward zero: -7 / 2 == -3, -7 % 2 == -1
    EXPECT_EQ(q, -3);
    EXPECT_EQ(r, -1);
}

TEST(Modern, DivmodThrowsOnZero) {
    EXPECT_THROW(divmod(1, 0), std::invalid_argument);
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо новых фиксированных примеров мы прогоняем сотни случайных (но
// воспроизводимых — RNG с жёстким сидом) входов и проверяем ИНВАРИАНТЫ:
// сверка с std::-оракулом, round-trip, перестановка, границы, идемпотентность.
// Плюс явные крайние случаи. На эталонном решении всё должно проходить.

// --- minmax_of: сверка с оракулом std::minmax_element ---

TEST(MinMaxProps, MatchesStdOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(1, 60);
    std::uniform_int_distribution<int> val(-1000, 1000);
    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> v;
        int n = len(rng);
        v.reserve(n);
        for (int i = 0; i < n; ++i) v.push_back(val(rng));

        auto [lo, hi] = minmax_of(v);
        int oracle_lo = *std::min_element(v.begin(), v.end());
        int oracle_hi = *std::max_element(v.begin(), v.end());

        EXPECT_EQ(lo, oracle_lo) << "iter=" << iter;
        EXPECT_EQ(hi, oracle_hi) << "iter=" << iter;
        // Инварианты порядка и принадлежности.
        EXPECT_LE(lo, hi);
        EXPECT_NE(std::find(v.begin(), v.end(), lo), v.end());
        EXPECT_NE(std::find(v.begin(), v.end(), hi), v.end());
        for (int x : v) {
            EXPECT_LE(lo, x);
            EXPECT_GE(hi, x);
        }
    }
}

TEST(MinMaxProps, EdgeSingleAndExtremes) {
    auto [lo1, hi1] = minmax_of({42});
    EXPECT_EQ(lo1, 42);
    EXPECT_EQ(hi1, 42);  // на одноэлементном векторе min == max

    auto [lo2, hi2] = minmax_of(
        {std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max()});
    EXPECT_EQ(lo2, std::numeric_limits<int>::min());
    EXPECT_EQ(hi2, std::numeric_limits<int>::max());

    auto [lo3, hi3] = minmax_of({-5, -5, -5});  // все равны
    EXPECT_EQ(lo3, -5);
    EXPECT_EQ(hi3, -5);
}

// --- color_name: enum -> строка, инъективность и полнота ---

TEST(ColorProps, AllNamesDistinctAndNonEmpty) {
    const Color all[] = {Color::Red, Color::Green, Color::Blue};
    std::set<std::string> names;
    for (Color c : all) {
        std::string nm = color_name(c);
        EXPECT_FALSE(nm.empty());
        names.insert(nm);
    }
    // Имена различны для разных цветов (инъективность).
    EXPECT_EQ(names.size(), 3u);
    EXPECT_TRUE(names.count("Red"));
    EXPECT_TRUE(names.count("Green"));
    EXPECT_TRUE(names.count("Blue"));
}

TEST(ColorProps, DeterministicRepeated) {
    // Чистая функция: один и тот же вход всегда даёт один и тот же выход.
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> pick(0, 2);
    const Color all[] = {Color::Red, Color::Green, Color::Blue};
    for (int iter = 0; iter < 200; ++iter) {
        Color c = all[pick(rng)];
        EXPECT_EQ(color_name(c), color_name(c));
    }
}

// --- square_ce: оракул n*n, чётность, неотрицательность ---

TEST(SquareProps, MatchesProductOracle) {
    std::mt19937 rng(0xC0FFEEu);
    // Узкий диапазон, чтобы n*n не переполняло int.
    std::uniform_int_distribution<int> val(-46340, 46340);
    for (int iter = 0; iter < 400; ++iter) {
        int n = val(rng);
        long long sq = static_cast<long long>(square_ce(n));
        EXPECT_EQ(sq, static_cast<long long>(n) * n) << "n=" << n;
        EXPECT_GE(sq, 0);                      // квадрат неотрицателен
        EXPECT_EQ(square_ce(n), square_ce(-n)); // чётная функция
    }
}

TEST(SquareProps, EdgeZeroOneAndConstexpr) {
    // Значения проверяем В РАНТАЙМЕ: на незаполненном стабе тест обязан компилироваться,
    // а падать во время выполнения. static_assert на значении сломал бы сборку стаба.
    EXPECT_EQ(square_ce(0), 0);
    EXPECT_EQ(square_ce(1), 1);
    EXPECT_EQ(square_ce(-1), 1);
}

// --- evens_squared: сверка с ручным оракулом + инварианты ---

TEST(EvensSquaredProps, MatchesManualOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 50);
    std::uniform_int_distribution<int> val(-200, 200);
    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> v;
        int n = len(rng);
        for (int i = 0; i < n; ++i) v.push_back(val(rng));

        std::vector<int> oracle;
        for (int x : v) if (x % 2 == 0) oracle.push_back(x * x);

        auto got = evens_squared(v);
        EXPECT_EQ(got, oracle) << "iter=" << iter;
        // Каждый элемент результата — полный квадрат и неотрицателен.
        for (int y : got) {
            EXPECT_GE(y, 0);
            int root = static_cast<int>(std::lround(std::sqrt(static_cast<double>(y))));
            EXPECT_TRUE(root * root == y || (root + 1) * (root + 1) == y);
        }
        // Размер результата = число чётных во входе.
        std::size_t evens = std::count_if(v.begin(), v.end(),
                                          [](int x) { return x % 2 == 0; });
        EXPECT_EQ(got.size(), evens);
    }
}

TEST(EvensSquaredProps, EdgeAllOddAllEvenEmpty) {
    EXPECT_TRUE(evens_squared({}).empty());          // пусто
    EXPECT_TRUE(evens_squared({1, 3, 5, 7}).empty()); // одни нечётные
    EXPECT_EQ(evens_squared({0}), (std::vector<int>{0}));  // ноль — чётный
    EXPECT_EQ(evens_squared({-2, -4}), (std::vector<int>{4, 16}));  // отрицательные чётные
}

// --- sum_values: оракул std::accumulate + аддитивность ---

TEST(SumValuesProps, MatchesAccumulateOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 40);
    std::uniform_int_distribution<int> val(-500, 500);
    std::uniform_int_distribution<int> klen(1, 6);
    std::uniform_int_distribution<int> ch('a', 'z');
    for (int iter = 0; iter < 300; ++iter) {
        std::map<std::string, int> m;
        int n = len(rng);
        for (int i = 0; i < n; ++i) {
            std::string key;
            int kl = klen(rng);
            for (int j = 0; j < kl; ++j) key.push_back(static_cast<char>(ch(rng)));
            m[key] = val(rng);  // дубликаты ключей схлопываются — это нормально
        }
        long long oracle = 0;
        for (const auto& [k, v] : m) { (void)k; oracle += v; }

        EXPECT_EQ(static_cast<long long>(sum_values(m)), oracle) << "iter=" << iter;
    }
}

TEST(SumValuesProps, EdgeEmptyAndSingle) {
    EXPECT_EQ(sum_values({}), 0);                       // пустой словарь -> 0
    EXPECT_EQ(sum_values({{"only", -123}}), -123);      // один элемент
    EXPECT_EQ(sum_values({{"a", 5}, {"b", -5}}), 0);    // взаимно гасятся
}

// --- split_view: round-trip (join обратно), счётчик кусков, no-copy ---

namespace {
// Склеить куски обратно через разделитель: должно дать исходную строку.
std::string join_views(const std::vector<std::string_view>& parts, char sep) {
    std::string out;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i) out.push_back(sep);
        out.append(parts[i]);
    }
    return out;
}
}  // namespace

TEST(SplitViewProps, RoundTripAndCount) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 40);
    // Алфавит включает сам разделитель ',', чтобы получать пустые куски.
    const std::string alphabet = "ab,";
    std::uniform_int_distribution<int> pick(0, static_cast<int>(alphabet.size()) - 1);
    const char sep = ',';

    for (int iter = 0; iter < 500; ++iter) {
        std::string s;
        int n = len(rng);
        for (int i = 0; i < n; ++i) s.push_back(alphabet[pick(rng)]);

        auto parts = split_view(s, sep);

        // Инвариант 1: число кусков = число разделителей + 1.
        std::size_t seps = std::count(s.begin(), s.end(), sep);
        EXPECT_EQ(parts.size(), seps + 1) << "s=\"" << s << "\"";

        // Инвариант 2: round-trip — склейка через sep восстанавливает строку.
        EXPECT_EQ(join_views(parts, sep), s) << "s=\"" << s << "\"";

        // Инвариант 3: ни один кусок не содержит разделителя.
        for (auto p : parts) {
            EXPECT_EQ(p.find(sep), std::string_view::npos);
        }

        // Инвариант 4 (no-copy): каждый кусок смотрит внутрь исходной строки s.
        const char* base = s.data();
        for (auto p : parts) {
            EXPECT_GE(p.data(), base);
            EXPECT_LE(p.data() + p.size(), base + s.size());
        }
    }
}

TEST(SplitViewProps, EdgeEmptySepsOnlyNoSep) {
    auto e = split_view("", ',');
    ASSERT_EQ(e.size(), 1u);
    EXPECT_EQ(e[0], "");  // пустая строка -> один пустой кусок

    auto only = split_view(",,,", ',');   // три разделителя -> четыре пустых куска
    ASSERT_EQ(only.size(), 4u);
    for (auto p : only) EXPECT_EQ(p, "");

    auto nosep = split_view("hello", ',');  // нет разделителя -> один кусок = вся строка
    ASSERT_EQ(nosep.size(), 1u);
    EXPECT_EQ(nosep[0], "hello");
}

// --- ScopeGuard: ровно один вызов, dismiss отменяет, область видимости ---

TEST(ScopeGuardProps, FiresExactlyOnceRandomNesting) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dn(1, 8);
    for (int iter = 0; iter < 300; ++iter) {
        int guards = dn(rng);
        int calls = 0;
        {
            // Вектор из guard'ов: каждый при выходе из блока обязан сработать ровно раз.
            std::vector<std::unique_ptr<ScopeGuard>> gs;
            for (int i = 0; i < guards; ++i)
                gs.push_back(std::make_unique<ScopeGuard>([&] { ++calls; }));
            EXPECT_EQ(calls, 0);  // пока внутри блока — ни один не сработал
        }
        EXPECT_EQ(calls, guards) << "iter=" << iter;  // ровно по разу на каждый
    }
}

TEST(ScopeGuardProps, DismissSuppressesExactly) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dn(0, 10);
    std::bernoulli_distribution coin(0.5);
    for (int iter = 0; iter < 300; ++iter) {
        int total = dn(rng);
        int fired = 0;
        int expected = 0;
        {
            std::vector<std::unique_ptr<ScopeGuard>> gs;
            for (int i = 0; i < total; ++i) {
                gs.push_back(std::make_unique<ScopeGuard>([&] { ++fired; }));
                if (coin(rng)) {
                    gs.back()->dismiss();  // отменяем половину (в среднем)
                } else {
                    ++expected;
                }
            }
        }
        // Сработали только НЕотменённые, ровно по одному разу.
        EXPECT_EQ(fired, expected) << "iter=" << iter;
    }
}

TEST(ScopeGuardProps, EdgeThrowAndDismissAll) {
    // Срабатывает даже при выходе через исключение.
    int onThrow = 0;
    try {
        ScopeGuard g([&] { ++onThrow; });
        throw std::runtime_error("boom");
    } catch (const std::runtime_error&) {}
    EXPECT_EQ(onThrow, 1);

    // Все отменены -> ноль вызовов.
    int none = 0;
    {
        ScopeGuard a([&] { ++none; });
        ScopeGuard b([&] { ++none; });
        a.dismiss();
        b.dismiss();
    }
    EXPECT_EQ(none, 0);
}

// --- Задание 9: safe_divide (optional) ---

TEST(Modern, SafeDivideBasic) {
    auto r = safe_divide(6, 3);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 2);
}

TEST(Modern, SafeDivideZeroDivisor) {
    EXPECT_FALSE(safe_divide(7, 0).has_value());
    EXPECT_EQ(safe_divide(7, 0), std::nullopt);
}

TEST(Modern, SafeDivideNegative) {
    auto r1 = safe_divide(-7, 2);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(*r1, -3);   // truncation toward zero

    auto r2 = safe_divide(7, -2);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(*r2, -3);

    auto r3 = safe_divide(0, 5);
    ASSERT_TRUE(r3.has_value());
    EXPECT_EQ(*r3, 0);
}

TEST(Modern, SafeDivideEdge) {
    // Точное деление без остатка
    auto exact = safe_divide(100, 4);
    ASSERT_TRUE(exact.has_value());
    EXPECT_EQ(*exact, 25);

    // b==0 независимо от a
    EXPECT_EQ(safe_divide(0, 0), std::nullopt);
    EXPECT_EQ(safe_divide(-100, 0), std::nullopt);
    EXPECT_EQ(safe_divide(std::numeric_limits<int>::max(), 0), std::nullopt);
}

// Seeded property: для b!=0 — has_value() и *value == a/b; для b==0 — nullopt.
TEST(SafeDivideProps, PropertySeeded) {
    std::mt19937 rng(0xDECAFu);
    std::uniform_int_distribution<int> aval(-100000, 100000);
    std::uniform_int_distribution<int> bval(-1000, 1000);

    for (int iter = 0; iter < 600; ++iter) {
        int a = aval(rng);
        int b = bval(rng);

        auto result = safe_divide(a, b);

        if (b == 0) {
            // Инвариант: при нулевом делителе — nullopt
            EXPECT_FALSE(result.has_value()) << "a=" << a << " b=" << b;
            EXPECT_EQ(result, std::nullopt);
        } else {
            // Инвариант: при b!=0 — has_value() и совпадает с оракулом a/b
            ASSERT_TRUE(result.has_value()) << "a=" << a << " b=" << b;
            EXPECT_EQ(*result, a / b) << "a=" << a << " b=" << b;
            // Дополнительно: тождество деления (безостаточная часть)
            EXPECT_EQ(*result * b + (a % b), a) << "a=" << a << " b=" << b;
        }
    }
}

// --- divmod: тождество q*b + r == a, знак остатка, исключение ---

TEST(DivmodProps, EuclidIdentityAndSign) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> aval(-100000, 100000);
    std::uniform_int_distribution<int> bval(-1000, 1000);
    for (int iter = 0; iter < 500; ++iter) {
        int a = aval(rng);
        int b = bval(rng);
        if (b == 0) b = 1;  // ноль обрабатываем отдельным edge-тестом

        auto [q, r] = divmod(a, b);

        // Главное тождество деления с остатком.
        EXPECT_EQ(static_cast<long long>(q) * b + r, a) << "a=" << a << " b=" << b;
        // Сверка с оракулом языка.
        EXPECT_EQ(q, a / b);
        EXPECT_EQ(r, a % b);
        // |r| < |b| и знак остатка совпадает со знаком делимого (truncation toward zero).
        EXPECT_LT(std::abs(static_cast<long long>(r)), std::abs(static_cast<long long>(b)));
        if (r != 0) {
            EXPECT_EQ(r > 0, a > 0);
        }
    }
}

TEST(DivmodProps, EdgeThrowsAndBoundaries) {
    EXPECT_THROW(divmod(0, 0), std::invalid_argument);
    EXPECT_THROW(divmod(-5, 0), std::invalid_argument);

    auto [q0, r0] = divmod(0, 7);   // ноль делимое
    EXPECT_EQ(q0, 0);
    EXPECT_EQ(r0, 0);

    auto [q1, r1] = divmod(5, 1);   // делитель 1
    EXPECT_EQ(q1, 5);
    EXPECT_EQ(r1, 0);

    auto [q2, r2] = divmod(7, -3);  // отрицательный делитель
    EXPECT_EQ(static_cast<long long>(q2) * -3 + r2, 7);
}
