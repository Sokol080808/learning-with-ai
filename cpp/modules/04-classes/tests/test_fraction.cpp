#include <gtest/gtest.h>
#include "fraction.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <cstdint>
#include <random>
#include <numeric>
#include <vector>
#include <algorithm>
#include <cmath>
#include <type_traits>

TEST(Fraction, NormalizesOnConstruction) {
    Fraction a(2, 4);
    EXPECT_EQ(a.numerator(), 1);
    EXPECT_EQ(a.denominator(), 2);
}

TEST(Fraction, MovesSignToNumerator) {
    Fraction a(1, -2);
    EXPECT_EQ(a.numerator(), -1);
    EXPECT_EQ(a.denominator(), 2);

    Fraction b(-3, -6);
    EXPECT_EQ(b.numerator(), 1);
    EXPECT_EQ(b.denominator(), 2);
}

TEST(Fraction, Zero) {
    Fraction z(0, 5);
    EXPECT_EQ(z.numerator(), 0);
    EXPECT_EQ(z.denominator(), 1);
}

TEST(Fraction, Add) {
    EXPECT_TRUE(Fraction(1, 2).add(Fraction(1, 3)) == Fraction(5, 6));
    EXPECT_TRUE(Fraction(1, 2).add(Fraction(1, 2)) == Fraction(1, 1));
    EXPECT_TRUE(Fraction(1, 2).add(Fraction(-1, 2)) == Fraction(0, 1));
}

TEST(Fraction, Multiply) {
    EXPECT_TRUE(Fraction(2, 3).multiply(Fraction(3, 4)) == Fraction(1, 2));
}

TEST(Fraction, Equality) {
    EXPECT_TRUE(Fraction(2, 4) == Fraction(1, 2));
    EXPECT_FALSE(Fraction(1, 2) == Fraction(1, 3));
}

TEST(Fraction, ToString) {
    EXPECT_EQ(Fraction(1, 2).to_string(), "1/2");
    EXPECT_EQ(Fraction(1, -2).to_string(), "-1/2");
    EXPECT_EQ(Fraction(4, 2).to_string(), "2/1");
}

// ──────────────────────────────────────────────────────────────────────────
// Задание 2. Перегруженные операторы Fraction: + * < <<

TEST(FractionOperators, PlusMatchesAdd) {
    EXPECT_TRUE((Fraction(1, 2) + Fraction(1, 3)) == Fraction(5, 6));
    EXPECT_TRUE((Fraction(1, 2) + Fraction(1, 2)) == Fraction(1, 1));
    EXPECT_TRUE((Fraction(1, 2) + Fraction(-1, 2)) == Fraction(0, 1));
}

TEST(FractionOperators, PlusNormalizesResult) {
    Fraction r = Fraction(1, 6) + Fraction(1, 3);   // = 3/6 = 1/2
    EXPECT_EQ(r.numerator(), 1);
    EXPECT_EQ(r.denominator(), 2);
}

TEST(FractionOperators, Star) {
    EXPECT_TRUE((Fraction(2, 3) * Fraction(3, 4)) == Fraction(1, 2));
    EXPECT_TRUE((Fraction(-1, 2) * Fraction(2, 1)) == Fraction(-1, 1));
}

TEST(FractionOperators, LessThan) {
    EXPECT_TRUE (Fraction(1, 3) < Fraction(1, 2));
    EXPECT_FALSE(Fraction(1, 2) < Fraction(1, 3));
    EXPECT_FALSE(Fraction(1, 2) < Fraction(1, 2));   // строгий: равные не меньше
    EXPECT_TRUE (Fraction(-1, 2) < Fraction(1, 100));
    EXPECT_FALSE(Fraction(1, 100) < Fraction(-1, 2));
}

TEST(FractionOperators, StreamOutput) {
    std::ostringstream os;
    os << Fraction(1, 2);
    EXPECT_EQ(os.str(), "1/2");

    std::ostringstream os2;
    os2 << Fraction(3, -4);   // знак уходит в числитель
    EXPECT_EQ(os2.str(), "-3/4");

    std::ostringstream os3;
    os3 << Fraction(4, 2);    // 2/1
    EXPECT_EQ(os3.str(), "2/1");
}

TEST(FractionOperators, StreamReturnsStreamForChaining) {
    std::ostringstream os;
    os << Fraction(1, 2) << " " << Fraction(1, 3);
    EXPECT_EQ(os.str(), "1/2 1/3");
}

// ──────────────────────────────────────────────────────────────────────────
// Задание 3. RAII-таймер: при жизни объекта измеряется интервал, при разрушении —
// прибавляется к внешнему счётчику.

TEST(Timer, AddsElapsedOnDestruction) {
    std::int64_t total_ns = 0;
    {
        Timer t(total_ns);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        // total_ns ещё не должен измениться: запись происходит в деструкторе.
        EXPECT_EQ(total_ns, 0);
    }
    // 5 мс = 5'000'000 нс. Допускаем накладные расходы, но не меньше ~3 мс.
    EXPECT_GE(total_ns, 3'000'000);
}

TEST(Timer, AccumulatesAcrossScopes) {
    std::int64_t total_ns = 0;
    {
        Timer t(total_ns);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    std::int64_t after_first = total_ns;
    EXPECT_GE(after_first, 1'000'000);
    {
        Timer t(total_ns);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
    // Второй замер ДОБАВЛЯЕТСЯ к первому, а не перезаписывает его.
    EXPECT_GT(total_ns, after_first);
}

TEST(Timer, IsNonCopyable) {
    // Документируем инвариант страж-объекта: его нельзя копировать.
    EXPECT_FALSE(std::is_copy_constructible_v<Timer>);
    EXPECT_FALSE(std::is_copy_assignable_v<Timer>);
}

// ──────────────────────────────────────────────────────────────────────────
// Задание 4. Vec2: + - == и dot().

TEST(Vec2Ops, Plus) {
    Vec2 r = Vec2{1.0, 2.0} + Vec2{3.0, 4.0};
    EXPECT_DOUBLE_EQ(r.x, 4.0);
    EXPECT_DOUBLE_EQ(r.y, 6.0);
}

TEST(Vec2Ops, Minus) {
    Vec2 r = Vec2{5.0, 7.0} - Vec2{1.0, 2.0};
    EXPECT_DOUBLE_EQ(r.x, 4.0);
    EXPECT_DOUBLE_EQ(r.y, 5.0);
}

TEST(Vec2Ops, Equality) {
    EXPECT_TRUE ((Vec2{1.5, -2.0} == Vec2{1.5, -2.0}));
    EXPECT_FALSE((Vec2{1.5, -2.0} == Vec2{1.5,  2.0}));
    EXPECT_FALSE((Vec2{1.5, -2.0} == Vec2{0.0, -2.0}));
}

TEST(Vec2Ops, Dot) {
    EXPECT_DOUBLE_EQ((Vec2{1.0, 2.0}).dot(Vec2{3.0, 4.0}), 11.0);   // 3 + 8
    EXPECT_DOUBLE_EQ((Vec2{1.0, 0.0}).dot(Vec2{0.0, 1.0}), 0.0);    // перпендикуляры
    EXPECT_DOUBLE_EQ((Vec2{2.0, -3.0}).dot(Vec2{2.0, -3.0}), 13.0); // длина^2
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо новых фиксированных примеров — гоняем сотни случайных входов и
// проверяем ИНВАРИАНТЫ класса (канонический вид, согласованность операторов,
// сверка с оракулом на long long / std::gcd). Все генераторы используют
// std::mt19937 с ЖЁСТКИМ сидом — никакого time()/random_device, CI не флакует.
//
// Диапазоны намеренно ограничены (|num|,|den| <= 1000): арифметика Fraction идёт
// в int, и при таких границах произведения (<= 1e6) и суммы (<= ~2e6) не
// переполняют int. Это сознательное условие property-теста, а не «случайность».

namespace {

// Числитель в [-1000, 1000], знаменатель в [1, 1000] (никогда не 0).
struct FracGen {
    std::uniform_int_distribution<int> num_dist{-1000, 1000};
    std::uniform_int_distribution<int> den_dist{1, 1000};
    // Знаменатель может быть и отрицательным на входе — конструктор обязан
    // перенести знак. Случайно инвертируем знак знаменателя.
    std::uniform_int_distribution<int> sign_dist{0, 1};

    template <class RNG>
    Fraction operator()(RNG& rng) {
        int n = num_dist(rng);
        int d = den_dist(rng);
        if (sign_dist(rng)) d = -d;
        return Fraction(n, d);
    }
};

// Оракул сравнения двух дробей по их КАНОНИЧЕСКИМ компонентам, в long long,
// чтобы исключить любые сомнения в переполнении внутри самого теста.
// Возвращает -1 / 0 / 1 для a<b / a==b / a>b.
int oracle_cmp(const Fraction& a, const Fraction& b) {
    long long lhs = static_cast<long long>(a.numerator()) * b.denominator();
    long long rhs = static_cast<long long>(b.numerator()) * a.denominator();
    if (lhs < rhs) return -1;
    if (lhs > rhs) return 1;
    return 0;
}

}  // namespace

// --- Fraction: канонический вид (инвариант конструктора) ------------------

TEST(FractionProps, ConstructorAlwaysCanonical) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 400; ++i) {
        Fraction f = gen(rng);
        // Знаменатель строго положителен — всегда.
        EXPECT_GT(f.denominator(), 0);
        // Сокращённость: gcd(|num|, den) == 1 для ненулевой дроби.
        int g = std::gcd(f.numerator(), f.denominator());
        EXPECT_EQ(g, 1) << "не сокращено: " << f.to_string();
        // Ноль канонизируется в 0/1.
        if (f.numerator() == 0) {
            EXPECT_EQ(f.denominator(), 1) << "ноль не как 0/1: " << f.to_string();
        }
    }
}

// Любые два входа, дающие одно и то же рациональное число (после умножения
// числителя и знаменателя на общий множитель), должны быть == и иметь
// одинаковые канонические компоненты.
TEST(FractionProps, EqualUnderCommonScaling) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> n_dist{-30, 30};
    std::uniform_int_distribution<int> d_dist{1, 30};
    std::uniform_int_distribution<int> k_dist{1, 30};
    for (int i = 0; i < 400; ++i) {
        int n = n_dist(rng);
        int d = d_dist(rng);
        int k = k_dist(rng);
        Fraction base(n, d);
        Fraction scaled(n * k, d * k);   // та же дробь, масштабированная
        EXPECT_TRUE(base == scaled) << base.to_string() << " vs " << scaled.to_string();
        EXPECT_EQ(base.numerator(), scaled.numerator());
        EXPECT_EQ(base.denominator(), scaled.denominator());
    }
}

// --- Fraction: согласованность операторов друг с другом и с оракулом -------

TEST(FractionProps, PlusMatchesAdd) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 400; ++i) {
        Fraction a = gen(rng);
        Fraction b = gen(rng);
        EXPECT_TRUE((a + b) == a.add(b));
    }
}

TEST(FractionProps, StarMatchesMultiply) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 400; ++i) {
        Fraction a = gen(rng);
        Fraction b = gen(rng);
        EXPECT_TRUE((a * b) == a.multiply(b));
    }
}

// Сложение коммутативно: a + b == b + a.
TEST(FractionProps, AddCommutative) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 400; ++i) {
        Fraction a = gen(rng);
        Fraction b = gen(rng);
        EXPECT_TRUE((a + b) == (b + a));
    }
}

// Умножение коммутативно: a * b == b * a.
TEST(FractionProps, MultiplyCommutative) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 400; ++i) {
        Fraction a = gen(rng);
        Fraction b = gen(rng);
        EXPECT_TRUE((a * b) == (b * a));
    }
}

// Нейтральные элементы: a + 0/1 == a, a * 1/1 == a, a * 0/1 == 0/1.
TEST(FractionProps, IdentityElements) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    const Fraction zero(0, 1);
    const Fraction one(1, 1);
    for (int i = 0; i < 400; ++i) {
        Fraction a = gen(rng);
        EXPECT_TRUE((a + zero) == a);
        EXPECT_TRUE((a * one) == a);
        EXPECT_TRUE((a * zero) == zero);
    }
}

// operator< должен быть согласован с независимым оракулом (long long
// кросс-умножение). Заодно проверяем трихотомию: ровно одно из <, ==, >.
TEST(FractionProps, LessThanMatchesOracle) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 500; ++i) {
        Fraction a = gen(rng);
        Fraction b = gen(rng);
        int c = oracle_cmp(a, b);
        EXPECT_EQ(a < b, c < 0);
        EXPECT_EQ(b < a, c > 0);
        EXPECT_EQ(a == b, c == 0);
        // Строгость: дробь не меньше самой себя.
        EXPECT_FALSE(a < a);
    }
}

// Антисимметрия/трихотомия порядка: для любых a, b истинно ровно одно из
// (a<b), (b<a), (a==b).
TEST(FractionProps, OrderTrichotomy) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 400; ++i) {
        Fraction a = gen(rng);
        Fraction b = gen(rng);
        int cnt = (a < b ? 1 : 0) + (b < a ? 1 : 0) + (a == b ? 1 : 0);
        EXPECT_EQ(cnt, 1) << a.to_string() << " ? " << b.to_string();
    }
}

// Транзитивность порядка: a<b и b<c => a<c.
TEST(FractionProps, OrderTransitive) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 500; ++i) {
        Fraction a = gen(rng);
        Fraction b = gen(rng);
        Fraction c = gen(rng);
        if ((a < b) && (b < c)) {
            EXPECT_TRUE(a < c)
                << a.to_string() << " < " << b.to_string() << " < " << c.to_string();
        }
    }
}

// to_string сверяется с оракулом форматирования "num/den" по каноническим
// компонентам — это инвариант формата (а не повтор реализации, т.к. оракул
// строит строку независимо из numerator()/denominator()).
TEST(FractionProps, ToStringMatchesCanonical) {
    std::mt19937 rng(0xC0FFEEu);
    FracGen gen;
    for (int i = 0; i < 400; ++i) {
        Fraction f = gen(rng);
        std::string expected =
            std::to_string(f.numerator()) + "/" + std::to_string(f.denominator());
        EXPECT_EQ(f.to_string(), expected);
        // operator<< печатает то же самое.
        std::ostringstream os;
        os << f;
        EXPECT_EQ(os.str(), expected);
    }
}

// --- Fraction: явные краевые случаи ---------------------------------------

TEST(FractionProps, EdgeCases) {
    // Ноль в любом виде -> 0/1.
    EXPECT_EQ(Fraction(0, 7).numerator(), 0);
    EXPECT_EQ(Fraction(0, 7).denominator(), 1);
    EXPECT_EQ(Fraction(0, -7).numerator(), 0);
    EXPECT_EQ(Fraction(0, -7).denominator(), 1);
    EXPECT_TRUE(Fraction(0, 7) == Fraction(0, -3));

    // Отрицательный знаменатель -> знак уходит в числитель.
    EXPECT_EQ(Fraction(3, -4).numerator(), -3);
    EXPECT_EQ(Fraction(3, -4).denominator(), 4);
    EXPECT_EQ(Fraction(-3, -4).numerator(), 3);
    EXPECT_EQ(Fraction(-3, -4).denominator(), 4);

    // Целое число: den сокращается до 1.
    EXPECT_EQ(Fraction(10, 5).numerator(), 2);
    EXPECT_EQ(Fraction(10, 5).denominator(), 1);

    // Большие взаимно простые компоненты в пределах int — остаются как есть.
    Fraction big(999, 1000);
    EXPECT_EQ(big.numerator(), 999);
    EXPECT_EQ(big.denominator(), 1000);

    // a - a через add(отрицание): a + (-a) == 0/1.
    Fraction a(7, 9);
    Fraction neg_a(-7, 9);
    EXPECT_TRUE(a.add(neg_a) == Fraction(0, 1));
}

// --- Timer: RAII-инварианты -----------------------------------------------

// Аккумулятор монотонно не убывает по серии вложенных/последовательных замеров,
// и итог равен сумме приращений каждого замера. Используем фиксированный набор
// «спанов» детерминированно (сид влияет только на порядок, не на флаки —
// сравниваем неубывание, а не точные числа).
TEST(TimerProps, AccumulatorMonotonicNonDecreasing) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> us_dist{0, 200};  // 0..200 микросекунд
    std::int64_t total = 0;
    std::int64_t prev = 0;
    for (int i = 0; i < 50; ++i) {
        {
            Timer t(total);
            std::this_thread::sleep_for(std::chrono::microseconds(us_dist(rng)));
            // Внутри области total ещё не обновлён этим таймером.
            EXPECT_EQ(total, prev);
        }
        // После выхода из области аккумулятор не уменьшился.
        EXPECT_GE(total, prev);
        prev = total;
    }
    EXPECT_GE(total, 0);
}

// Свойство аддитивности: два отдельных Timer на один аккумулятор дают сумму,
// не меньшую, чем каждый по отдельности (замер всегда неотрицателен).
TEST(TimerProps, IndependentTimersAccumulate) {
    std::int64_t total = 0;
    { Timer t(total); std::this_thread::sleep_for(std::chrono::microseconds(100)); }
    std::int64_t after_first = total;
    EXPECT_GE(after_first, 0);
    { Timer t(total); std::this_thread::sleep_for(std::chrono::microseconds(100)); }
    EXPECT_GE(total, after_first);  // монотонно: второй замер добавляется
}

// Тип-инвариант: Timer некопируем и неприсваиваем (страж-объект).
TEST(TimerProps, NonCopyableTypeTraits) {
    EXPECT_FALSE(std::is_copy_constructible_v<Timer>);
    EXPECT_FALSE(std::is_copy_assignable_v<Timer>);
}

// --- Vec2: алгебраические инварианты на случайных векторах ------------------

namespace {
Vec2 rand_vec2(std::mt19937& rng) {
    std::uniform_real_distribution<double> d{-1000.0, 1000.0};
    return Vec2{d(rng), d(rng)};
}
}  // namespace

// (a + b) - b == a, и (a - b) + b == a (точно, без округления, на «круглых»
// случайных значениях суммирование обратимо). Берём целочисленные double,
// чтобы + и - были точными в плавающей арифметике.
TEST(Vec2Props, AddSubInverse) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> d{-1000, 1000};
    for (int i = 0; i < 400; ++i) {
        Vec2 a{static_cast<double>(d(rng)), static_cast<double>(d(rng))};
        Vec2 b{static_cast<double>(d(rng)), static_cast<double>(d(rng))};
        Vec2 r1 = (a + b) - b;
        EXPECT_DOUBLE_EQ(r1.x, a.x);
        EXPECT_DOUBLE_EQ(r1.y, a.y);
        Vec2 r2 = (a - b) + b;
        EXPECT_DOUBLE_EQ(r2.x, a.x);
        EXPECT_DOUBLE_EQ(r2.y, a.y);
    }
}

// Сложение коммутативно и покомпонентно.
TEST(Vec2Props, AddCommutativeComponentwise) {
    std::mt19937 rng(0xC0FFEEu);
    for (int i = 0; i < 400; ++i) {
        Vec2 a = rand_vec2(rng);
        Vec2 b = rand_vec2(rng);
        Vec2 ab = a + b;
        Vec2 ba = b + a;
        EXPECT_DOUBLE_EQ(ab.x, ba.x);
        EXPECT_DOUBLE_EQ(ab.y, ba.y);
        EXPECT_DOUBLE_EQ(ab.x, a.x + b.x);
        EXPECT_DOUBLE_EQ(ab.y, a.y + b.y);
    }
}

// dot коммутативно, dot(a,a) >= 0, dot линейно по a: dot(a+b, c) = dot(a,c)+dot(b,c).
TEST(Vec2Props, DotProperties) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> d{-100, 100};  // малые целые: точно
    for (int i = 0; i < 400; ++i) {
        Vec2 a{static_cast<double>(d(rng)), static_cast<double>(d(rng))};
        Vec2 b{static_cast<double>(d(rng)), static_cast<double>(d(rng))};
        Vec2 c{static_cast<double>(d(rng)), static_cast<double>(d(rng))};
        // Симметрия.
        EXPECT_DOUBLE_EQ(a.dot(b), b.dot(a));
        // Неотрицательность скалярного квадрата.
        EXPECT_GE(a.dot(a), 0.0);
        // dot(a,a) == |a|^2 == x^2 + y^2.
        EXPECT_DOUBLE_EQ(a.dot(a), a.x * a.x + a.y * a.y);
        // Линейность (билинейность) по первому аргументу.
        EXPECT_DOUBLE_EQ((a + b).dot(c), a.dot(c) + b.dot(c));
    }
}

// == точное и рефлексивное; неравенство при изменении любой компоненты.
TEST(Vec2Props, EqualityReflexiveAndStrict) {
    std::mt19937 rng(0xC0FFEEu);
    for (int i = 0; i < 400; ++i) {
        Vec2 a = rand_vec2(rng);
        EXPECT_TRUE(a == a);                       // рефлексивность
        Vec2 copy{a.x, a.y};
        EXPECT_TRUE(a == copy);                    // равенство копии
        // Сдвиг одной компоненты на заметную величину делает векторы разными.
        Vec2 shift_x{a.x + 1.0, a.y};
        Vec2 shift_y{a.x, a.y + 1.0};
        EXPECT_FALSE(a == shift_x);
        EXPECT_FALSE(a == shift_y);
    }
}

// Краевые случаи Vec2: нулевой вектор — нейтраль сложения, dot с нулём = 0.
TEST(Vec2Props, ZeroVectorEdge) {
    const Vec2 zero{0.0, 0.0};
    std::mt19937 rng(0xC0FFEEu);
    for (int i = 0; i < 200; ++i) {
        Vec2 a = rand_vec2(rng);
        Vec2 sum = a + zero;
        EXPECT_TRUE(sum == a);
        EXPECT_DOUBLE_EQ(a.dot(zero), 0.0);
        Vec2 diff = a - a;
        EXPECT_TRUE(diff == zero);
    }
}
