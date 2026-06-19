#include <gtest/gtest.h>
#include "fraction.hpp"

#include <sstream>
#include <thread>
#include <chrono>
#include <cstdint>

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
