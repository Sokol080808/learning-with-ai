#include <gtest/gtest.h>
#include "calc.hpp"

TEST(Calc, Add)      { EXPECT_DOUBLE_EQ(calc(2, '+', 3), 5.0); }
TEST(Calc, Subtract) { EXPECT_DOUBLE_EQ(calc(10, '-', 4), 6.0); }
TEST(Calc, Multiply) { EXPECT_DOUBLE_EQ(calc(3, '*', 4), 12.0); }
TEST(Calc, Divide)   { EXPECT_DOUBLE_EQ(calc(9, '/', 2), 4.5); }
