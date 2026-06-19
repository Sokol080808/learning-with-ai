#include <gtest/gtest.h>
#include "fraction.hpp"

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
