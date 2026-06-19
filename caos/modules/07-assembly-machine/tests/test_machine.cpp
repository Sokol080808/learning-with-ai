// Эти тесты трогать не нужно — это эталон поведения.
//
// Они описывают, ЧТО три функции обязаны возвращать. Если тест красный — смотри,
// что ожидалось (Expected) и что получилось (Actual), и правь src/machine.c.

#include <gtest/gtest.h>

extern "C" {
#include "machine.h"
}

// --- Листинг A: asm_a(x) = x*x + 1 ---

TEST(AsmA, Zero) {
    EXPECT_EQ(asm_a(0), 1);     // 0*0 + 1
}

TEST(AsmA, One) {
    EXPECT_EQ(asm_a(1), 2);     // 1*1 + 1
}

TEST(AsmA, SmallPositive) {
    EXPECT_EQ(asm_a(5), 26);    // 25 + 1
    EXPECT_EQ(asm_a(10), 101);  // 100 + 1
}

TEST(AsmA, Negative) {
    EXPECT_EQ(asm_a(-3), 10);   // (-3)*(-3) + 1 = 9 + 1
}

// --- Листинг B: asm_b(a, b) = max(a, b) ---

TEST(AsmB, FirstLarger) {
    EXPECT_EQ(asm_b(7, 3), 7);
}

TEST(AsmB, SecondLarger) {
    EXPECT_EQ(asm_b(3, 7), 7);
}

TEST(AsmB, Equal) {
    EXPECT_EQ(asm_b(4, 4), 4);
}

TEST(AsmB, Negatives) {
    EXPECT_EQ(asm_b(-5, -2), -2);  // -2 > -5
    EXPECT_EQ(asm_b(-9, 0), 0);
}

// --- Листинг C: asm_c(n) = 1 + 2 + ... + n ---

TEST(AsmC, One) {
    EXPECT_EQ(asm_c(1), 1);
}

TEST(AsmC, Five) {
    EXPECT_EQ(asm_c(5), 15);    // 1+2+3+4+5
}

TEST(AsmC, Hundred) {
    EXPECT_EQ(asm_c(100), 5050);  // классическая сумма Гаусса
}

TEST(AsmC, ZeroOrNegative) {
    EXPECT_EQ(asm_c(0), 0);     // нечего суммировать
    EXPECT_EQ(asm_c(-4), 0);    // тоже 0: цикл не выполняется ни разу
}
