#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include "task01.hpp"

TEST(Basics, ToFahrenheit) {
    EXPECT_DOUBLE_EQ(to_fahrenheit(0.0), 32.0);
    EXPECT_DOUBLE_EQ(to_fahrenheit(100.0), 212.0);
    EXPECT_DOUBLE_EQ(to_fahrenheit(37.0), 98.6);
    EXPECT_DOUBLE_EQ(to_fahrenheit(-40.0), -40.0);
}

TEST(Basics, IsEven) {
    EXPECT_TRUE(is_even(0));
    EXPECT_TRUE(is_even(4));
    EXPECT_TRUE(is_even(-2));
    EXPECT_FALSE(is_even(1));
    EXPECT_FALSE(is_even(-3));
}

TEST(Basics, MinOfThree) {
    EXPECT_EQ(min_of_three(1, 2, 3), 1);
    EXPECT_EQ(min_of_three(3, 2, 1), 1);
    EXPECT_EQ(min_of_three(5, 5, 5), 5);
    EXPECT_EQ(min_of_three(-1, 0, 1), -1);
}

TEST(Basics, TripleByReference) {
    int x = 7;
    triple(x);
    EXPECT_EQ(x, 21);
    int y = -3;
    triple(y);
    EXPECT_EQ(y, -9);
}

TEST(Basics, Average3) {
    EXPECT_DOUBLE_EQ(average3(2, 4, 6), 4.0);
    EXPECT_DOUBLE_EQ(average3(1, 2, 2), 5.0 / 3.0);  // НЕ должно быть 1.0
    EXPECT_DOUBLE_EQ(average3(0, 0, 1), 1.0 / 3.0);
}

// --- Задание 6: safe_add ---

TEST(Basics, SafeAddOrdinary) {
    auto r = safe_add(2, 3);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 5);

    auto neg = safe_add(-10, 4);
    ASSERT_TRUE(neg.has_value());
    EXPECT_EQ(*neg, -6);

    auto zero = safe_add(0, 0);
    ASSERT_TRUE(zero.has_value());
    EXPECT_EQ(*zero, 0);
}

TEST(Basics, SafeAddBoundariesOk) {
    const int kMax = std::numeric_limits<int>::max();
    const int kMin = std::numeric_limits<int>::min();

    // Ровно по краю — переполнения ещё нет.
    auto hi = safe_add(kMax - 1, 1);
    ASSERT_TRUE(hi.has_value());
    EXPECT_EQ(*hi, kMax);

    auto lo = safe_add(kMin + 1, -1);
    ASSERT_TRUE(lo.has_value());
    EXPECT_EQ(*lo, kMin);

    auto mix = safe_add(kMax, kMin);  // = -1, переполнения нет
    ASSERT_TRUE(mix.has_value());
    EXPECT_EQ(*mix, -1);
}

TEST(Basics, SafeAddOverflowReturnsNullopt) {
    const int kMax = std::numeric_limits<int>::max();
    const int kMin = std::numeric_limits<int>::min();

    EXPECT_FALSE(safe_add(kMax, 1).has_value());       // переполнение вверх
    EXPECT_FALSE(safe_add(kMax, kMax).has_value());
    EXPECT_FALSE(safe_add(kMin, -1).has_value());      // переполнение вниз
    EXPECT_FALSE(safe_add(kMin, kMin).has_value());
}

// --- Задание 7: битовые операции ---

TEST(Basics, SetBit) {
    EXPECT_EQ(set_bit(0u, 0), 1u);
    EXPECT_EQ(set_bit(0u, 3), 8u);
    EXPECT_EQ(set_bit(0u, 31), 0x80000000u);
    // Установка уже установленного бита ничего не меняет.
    EXPECT_EQ(set_bit(0b1010u, 1), 0b1010u);
    // Остальные биты не трогаем.
    EXPECT_EQ(set_bit(0b0001u, 2), 0b0101u);
}

TEST(Basics, ClearBit) {
    EXPECT_EQ(clear_bit(0b1111u, 0), 0b1110u);
    EXPECT_EQ(clear_bit(0b1111u, 3), 0b0111u);
    EXPECT_EQ(clear_bit(0xFFFFFFFFu, 31), 0x7FFFFFFFu);
    // Сброс уже сброшенного бита ничего не меняет.
    EXPECT_EQ(clear_bit(0b0010u, 0), 0b0010u);
}

TEST(Basics, ToggleBit) {
    EXPECT_EQ(toggle_bit(0u, 0), 1u);
    EXPECT_EQ(toggle_bit(1u, 0), 0u);
    EXPECT_EQ(toggle_bit(0b1010u, 0), 0b1011u);
    EXPECT_EQ(toggle_bit(0b1010u, 1), 0b1000u);
    // Два переключения подряд возвращают исходное.
    EXPECT_EQ(toggle_bit(toggle_bit(0xABCDu, 5), 5), 0xABCDu);
}

TEST(Basics, GetBit) {
    EXPECT_TRUE(get_bit(0b1000u, 3));
    EXPECT_FALSE(get_bit(0b1000u, 2));
    EXPECT_TRUE(get_bit(1u, 0));
    EXPECT_FALSE(get_bit(0u, 0));
    EXPECT_TRUE(get_bit(0x80000000u, 31));
    EXPECT_FALSE(get_bit(0x7FFFFFFFu, 31));
}

// --- Задание 8: swap_ints ---

TEST(Basics, SwapInts) {
    int a = 1, b = 2;
    swap_ints(a, b);
    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 1);

    int x = -5, y = 100;
    swap_ints(x, y);
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, -5);
}

TEST(Basics, SwapIntsSameValue) {
    int a = 42, b = 42;
    swap_ints(a, b);
    EXPECT_EQ(a, 42);
    EXPECT_EQ(b, 42);
}

TEST(Basics, SwapIntsSameVariableStaysIntact) {
    // Самоприсваивание-через-swap не должно обнулять значение
    // (ловит наивную реализацию через XOR при a и b — одна переменная).
    int v = 7;
    swap_ints(v, v);
    EXPECT_EQ(v, 7);
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Эти тесты прогоняют сотни случайных входов с ФИКСИРОВАННЫМ сидом mt19937
// (CI никогда не «мигает») и проверяют ИНВАРИАНТЫ функций, а не конкретные примеры:
// сверка с оракулом (std::min / арифметика в long long / битовые маски),
// обратимость (round-trip), идемпотентность, границы. Плюс явные крайние случаи.

// --- to_fahrenheit: линейность, обратное преобразование, неподвижная точка ---

TEST(BasicsProps, ToFahrenheitMatchesLinearOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
    for (int i = 0; i < 400; ++i) {
        const double c = dist(rng);
        const double expected = c * 9.0 / 5.0 + 32.0;
        EXPECT_DOUBLE_EQ(to_fahrenheit(c), expected) << "c=" << c;
    }
}

TEST(BasicsProps, ToFahrenheitInverseRoundTrip) {
    // Обратное преобразование F -> C должно вернуть исходный C.
    std::mt19937 rng(0x1234ABCDu);
    std::uniform_real_distribution<double> dist(-500.0, 500.0);
    for (int i = 0; i < 300; ++i) {
        const double c = dist(rng);
        const double f = to_fahrenheit(c);
        const double back = (f - 32.0) * 5.0 / 9.0;
        EXPECT_NEAR(back, c, 1e-9) << "c=" << c << " f=" << f;
    }
}

TEST(BasicsProps, ToFahrenheitFixedPointAndMonotonic) {
    // -40 — единственная неподвижная точка шкал C и F.
    EXPECT_DOUBLE_EQ(to_fahrenheit(-40.0), -40.0);
    // Монотонность: больший C даёт строго больший F.
    std::mt19937 rng(0x5EEDu);
    std::uniform_real_distribution<double> dist(-300.0, 300.0);
    for (int i = 0; i < 300; ++i) {
        const double a = dist(rng);
        const double delta = std::uniform_real_distribution<double>(0.5, 50.0)(rng);
        EXPECT_LT(to_fahrenheit(a), to_fahrenheit(a + delta)) << "a=" << a;
    }
}

// --- is_even: оракул %, сосед нечётный, инвариантность к знаку ---

TEST(BasicsProps, IsEvenMatchesModOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(-1'000'000, 1'000'000);
    for (int i = 0; i < 500; ++i) {
        const int n = dist(rng);
        EXPECT_EQ(is_even(n), (n % 2 == 0)) << "n=" << n;
        // Сосед (n+1) обязан иметь противоположную чётность.
        EXPECT_NE(is_even(n), is_even(n + 1)) << "n=" << n;
        // 2*n всегда чётно, 2*n+1 всегда нечётно.
        const int half = dist(rng) / 2;  // делим, чтобы 2*half не переполнял int
        EXPECT_TRUE(is_even(2 * half));
        EXPECT_FALSE(is_even(2 * half + 1));
    }
    EXPECT_TRUE(is_even(0));  // явная граница
}

// --- min_of_three: равно std::min, один из аргументов, <= всех, перестановки ---

TEST(BasicsProps, MinOfThreeMatchesStdAndIsMember) {
    std::mt19937 rng(0xBEEF1234u);
    std::uniform_int_distribution<int> dist(-100'000, 100'000);
    for (int i = 0; i < 500; ++i) {
        const int a = dist(rng), b = dist(rng), c = dist(rng);
        const int m = min_of_three(a, b, c);
        const int oracle = std::min({a, b, c});
        EXPECT_EQ(m, oracle) << a << "," << b << "," << c;
        // Результат не больше каждого аргумента...
        EXPECT_LE(m, a);
        EXPECT_LE(m, b);
        EXPECT_LE(m, c);
        // ...и совпадает хотя бы с одним из них.
        EXPECT_TRUE(m == a || m == b || m == c);
    }
}

TEST(BasicsProps, MinOfThreePermutationInvariant) {
    // Минимум не зависит от порядка аргументов.
    std::mt19937 rng(0xABCDEF01u);
    std::uniform_int_distribution<int> dist(-50'000, 50'000);
    for (int i = 0; i < 400; ++i) {
        const int a = dist(rng), b = dist(rng), c = dist(rng);
        const int ref = min_of_three(a, b, c);
        EXPECT_EQ(min_of_three(a, c, b), ref);
        EXPECT_EQ(min_of_three(b, a, c), ref);
        EXPECT_EQ(min_of_three(b, c, a), ref);
        EXPECT_EQ(min_of_three(c, a, b), ref);
        EXPECT_EQ(min_of_three(c, b, a), ref);
    }
    // Граница: все равны.
    EXPECT_EQ(min_of_three(7, 7, 7), 7);
}

// --- triple: ровно утроение, идемпотентность через ссылку ---

TEST(BasicsProps, TripleEqualsTimesThree) {
    std::mt19937 rng(0x33333u);
    // Диапазон такой, что 3*x не переполняет int.
    std::uniform_int_distribution<int> dist(-700'000'000, 700'000'000);
    for (int i = 0; i < 500; ++i) {
        const int orig = dist(rng);
        int x = orig;
        triple(x);
        EXPECT_EQ(x, orig * 3) << "orig=" << orig;
    }
    int z = 0;
    triple(z);
    EXPECT_EQ(z, 0);  // граница: ноль
}

// --- average3: совпадает с double-оракулом и лежит между min и max ---

TEST(BasicsProps, Average3MatchesOracleAndBounded) {
    std::mt19937 rng(0xA1B2C3D4u);
    std::uniform_int_distribution<int> dist(-100'000, 100'000);
    for (int i = 0; i < 500; ++i) {
        const int a = dist(rng), b = dist(rng), c = dist(rng);
        const double got = average3(a, b, c);
        const double oracle = static_cast<double>(a + b + c) / 3.0;
        EXPECT_DOUBLE_EQ(got, oracle) << a << "," << b << "," << c;
        // Среднее не выходит за пределы [min, max] аргументов (с допуском на округление).
        const double lo = std::min({a, b, c});
        const double hi = std::max({a, b, c});
        EXPECT_GE(got, lo - 1e-9);
        EXPECT_LE(got, hi + 1e-9);
    }
    EXPECT_DOUBLE_EQ(average3(5, 5, 5), 5.0);  // граница: равные -> само значение
}

// --- safe_add: сверка с точным оракулом long long, коммутативность, границы ---

TEST(BasicsProps, SafeAddMatchesWideOracle) {
    std::mt19937 rng(0xDEAD10CCu);
    std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());
    constexpr long long kMax = std::numeric_limits<int>::max();
    constexpr long long kMin = std::numeric_limits<int>::min();
    for (int i = 0; i < 500; ++i) {
        const int a = dist(rng), b = dist(rng);
        const long long wide = static_cast<long long>(a) + b;  // точная сумма без UB
        const auto r = safe_add(a, b);
        if (wide < kMin || wide > kMax) {
            EXPECT_FALSE(r.has_value()) << a << "+" << b << " (wide=" << wide << ")";
        } else {
            ASSERT_TRUE(r.has_value()) << a << "+" << b;
            EXPECT_EQ(*r, static_cast<int>(wide));
        }
        // Коммутативность: a+b и b+a дают одинаковый результат (включая nullopt).
        EXPECT_EQ(safe_add(a, b), safe_add(b, a)) << a << "," << b;
    }
}

TEST(BasicsProps, SafeAddIdentityAndExtremes) {
    std::mt19937 rng(0x0FFEu);
    std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());
    for (int i = 0; i < 300; ++i) {
        const int a = dist(rng);
        // Прибавление нуля — тождество, переполнения быть не может.
        const auto r = safe_add(a, 0);
        ASSERT_TRUE(r.has_value());
        EXPECT_EQ(*r, a) << "a=" << a;
    }
    const int kMax = std::numeric_limits<int>::max();
    const int kMin = std::numeric_limits<int>::min();
    // Явные крайние случаи.
    EXPECT_FALSE(safe_add(kMax, 1).has_value());
    EXPECT_FALSE(safe_add(kMin, -1).has_value());
    EXPECT_FALSE(safe_add(kMax, kMax).has_value());
    EXPECT_FALSE(safe_add(kMin, kMin).has_value());
    EXPECT_EQ(safe_add(kMax, kMin).value(), -1);  // ровно -1, без переполнения
    EXPECT_EQ(safe_add(kMax - 1, 1).value(), kMax);
    EXPECT_EQ(safe_add(kMin + 1, -1).value(), kMin);
}

// --- битовые операции: сверка с масками, обратимость, идемпотентность ---

TEST(BasicsProps, BitOpsMatchMaskOracle) {
    std::mt19937 rng(0xB175C0DEu);
    std::uniform_int_distribution<std::uint32_t> vdist(0u, 0xFFFFFFFFu);
    std::uniform_int_distribution<int> pdist(0, 31);
    for (int i = 0; i < 500; ++i) {
        const unsigned int v = vdist(rng);
        const int pos = pdist(rng);
        const unsigned int mask = 1u << pos;

        // Сверка каждой операции с прямой маской.
        EXPECT_EQ(set_bit(v, pos), v | mask);
        EXPECT_EQ(clear_bit(v, pos), v & ~mask);
        EXPECT_EQ(toggle_bit(v, pos), v ^ mask);
        EXPECT_EQ(get_bit(v, pos), static_cast<bool>((v >> pos) & 1u));

        // После set бит обязательно 1; после clear — обязательно 0.
        EXPECT_TRUE(get_bit(set_bit(v, pos), pos));
        EXPECT_FALSE(get_bit(clear_bit(v, pos), pos));

        // set/clear затрагивают РОВНО один бит — остальные не меняются.
        EXPECT_EQ(set_bit(v, pos) & ~mask, v & ~mask);
        EXPECT_EQ(clear_bit(v, pos) & ~mask, v & ~mask);

        // toggle меняет ровно один бит относительно исходного значения.
        EXPECT_EQ(toggle_bit(v, pos) ^ v, mask);
    }
}

TEST(BasicsProps, BitOpsIdempotenceAndInverse) {
    std::mt19937 rng(0xFADEu);
    std::uniform_int_distribution<std::uint32_t> vdist(0u, 0xFFFFFFFFu);
    std::uniform_int_distribution<int> pdist(0, 31);
    for (int i = 0; i < 400; ++i) {
        const unsigned int v = vdist(rng);
        const int pos = pdist(rng);

        // Идемпотентность set/clear: повтор ничего не меняет.
        EXPECT_EQ(set_bit(set_bit(v, pos), pos), set_bit(v, pos));
        EXPECT_EQ(clear_bit(clear_bit(v, pos), pos), clear_bit(v, pos));

        // Двойной toggle возвращает исходное значение (обратимость).
        EXPECT_EQ(toggle_bit(toggle_bit(v, pos), pos), v);

        // set затем clear -> бит точно сброшен; clear затем set -> бит точно установлен.
        EXPECT_FALSE(get_bit(clear_bit(set_bit(v, pos), pos), pos));
        EXPECT_TRUE(get_bit(set_bit(clear_bit(v, pos), pos), pos));
    }
    // Явные границы: младший и старший биты.
    EXPECT_EQ(set_bit(0u, 0), 1u);
    EXPECT_EQ(set_bit(0u, 31), 0x80000000u);
    EXPECT_TRUE(get_bit(0x80000000u, 31));
    EXPECT_FALSE(get_bit(0u, 0));
}

// --- swap_ints: обмен значениями, двойной swap = тождество ---

TEST(BasicsProps, SwapIntsExchangesValues) {
    std::mt19937 rng(0x57A9ABCDu);  // фиксированный сид
    std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());
    for (int i = 0; i < 500; ++i) {
        const int oa = dist(rng), ob = dist(rng);
        int a = oa, b = ob;
        swap_ints(a, b);
        EXPECT_EQ(a, ob) << "oa=" << oa << " ob=" << ob;
        EXPECT_EQ(b, oa) << "oa=" << oa << " ob=" << ob;
        // Двойной swap возвращает исходное расположение.
        swap_ints(a, b);
        EXPECT_EQ(a, oa);
        EXPECT_EQ(b, ob);
    }
    // Граница: равные значения и крайние int.
    int p = std::numeric_limits<int>::min(), q = std::numeric_limits<int>::max();
    swap_ints(p, q);
    EXPECT_EQ(p, std::numeric_limits<int>::max());
    EXPECT_EQ(q, std::numeric_limits<int>::min());
}
