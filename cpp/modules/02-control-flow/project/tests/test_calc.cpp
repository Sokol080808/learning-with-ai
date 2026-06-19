#include <gtest/gtest.h>
#include <random>
#include <cmath>
#include "calc.hpp"

TEST(Calc, Add)      { EXPECT_DOUBLE_EQ(calc(2, '+', 3), 5.0); }
TEST(Calc, Subtract) { EXPECT_DOUBLE_EQ(calc(10, '-', 4), 6.0); }
TEST(Calc, Multiply) { EXPECT_DOUBLE_EQ(calc(3, '*', 4), 12.0); }
TEST(Calc, Divide)   { EXPECT_DOUBLE_EQ(calc(9, '/', 2), 4.5); }

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Сверяем calc() с прямой арифметикой как оракулом на сотнях случайных пар,
// а также проверяем алгебраические инварианты (обратимость +/-, *//).
// RNG засеян константой — детерминированно, без флака в CI.

// Каждая операция совпадает с прямым выражением a op b.
TEST(CalcProps, MatchesArithmeticOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
    for (int iter = 0; iter < 500; ++iter) {
        double a = dist(rng);
        double b = dist(rng);
        EXPECT_DOUBLE_EQ(calc(a, '+', b), a + b) << "a=" << a << " b=" << b;
        EXPECT_DOUBLE_EQ(calc(a, '-', b), a - b) << "a=" << a << " b=" << b;
        EXPECT_DOUBLE_EQ(calc(a, '*', b), a * b) << "a=" << a << " b=" << b;
        // деление гарантировано на ненулевой делитель
        double nz = dist(rng);
        if (nz == 0.0) nz = 1.0;
        EXPECT_DOUBLE_EQ(calc(a, '/', nz), a / nz) << "a=" << a << " nz=" << nz;
    }
}

// Обратимость операций: + отменяется -, * отменяется /.
TEST(CalcProps, InverseOperations) {
    std::mt19937 rng(0x1B7E5Eu);
    std::uniform_real_distribution<double> dist(-500.0, 500.0);
    for (int iter = 0; iter < 500; ++iter) {
        double a = dist(rng);
        double b = dist(rng);
        // (a + b) - b == a — но при |b| >> |a| сложение «съедает» младшие биты a
        // (катастрофическое сокращение double), поэтому сверяем с относительной
        // погрешностью, масштабированной по величине операндов, а не бит-в-бит.
        double add_rt = calc(calc(a, '+', b), '-', b);
        EXPECT_NEAR(add_rt, a, 1e-9 * (1.0 + std::abs(a) + std::abs(b)))
            << "a=" << a << " b=" << b;
        // коммутативность сложения и умножения
        EXPECT_DOUBLE_EQ(calc(a, '+', b), calc(b, '+', a)) << "a=" << a << " b=" << b;
        EXPECT_DOUBLE_EQ(calc(a, '*', b), calc(b, '*', a)) << "a=" << a << " b=" << b;
        // (a * b) / b == a при ненулевом b — с поправкой на округление double:
        // умножение и деление в общем случае не дают точно тот же бит-в-бит a,
        // поэтому сравниваем с относительной погрешностью, а не EXPECT_DOUBLE_EQ.
        double nz = dist(rng);
        if (nz == 0.0) nz = 1.0;
        double round_trip = calc(calc(a, '*', nz), '/', nz);
        EXPECT_NEAR(round_trip, a, 1e-9 * (1.0 + std::abs(a)))
            << "a=" << a << " nz=" << nz;
    }
    // нейтральные элементы как явные края
    EXPECT_DOUBLE_EQ(calc(0.0, '+', 0.0), 0.0);
    EXPECT_DOUBLE_EQ(calc(5.0, '*', 1.0), 5.0);
    EXPECT_DOUBLE_EQ(calc(5.0, '-', 5.0), 0.0);
}
