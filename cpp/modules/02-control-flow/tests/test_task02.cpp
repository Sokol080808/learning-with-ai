#include <gtest/gtest.h>
#include "task02.hpp"

TEST(ControlFlow, Factorial) {
    EXPECT_EQ(factorial(0), 1);
    EXPECT_EQ(factorial(1), 1);
    EXPECT_EQ(factorial(5), 120);
    EXPECT_EQ(factorial(7), 5040);
}

TEST(ControlFlow, Fibonacci) {
    EXPECT_EQ(fib(0), 0);
    EXPECT_EQ(fib(1), 1);
    EXPECT_EQ(fib(2), 1);
    EXPECT_EQ(fib(7), 13);
    EXPECT_EQ(fib(10), 55);
}

TEST(ControlFlow, Gcd) {
    EXPECT_EQ(gcd(12, 8), 4);
    EXPECT_EQ(gcd(17, 5), 1);
    EXPECT_EQ(gcd(100, 25), 25);
    EXPECT_EQ(gcd(0, 9), 9);
}

TEST(ControlFlow, FizzBuzz) {
    EXPECT_EQ(fizzbuzz(1), "1");
    EXPECT_EQ(fizzbuzz(3), "Fizz");
    EXPECT_EQ(fizzbuzz(5), "Buzz");
    EXPECT_EQ(fizzbuzz(15), "FizzBuzz");
    EXPECT_EQ(fizzbuzz(7), "7");
    EXPECT_EQ(fizzbuzz(30), "FizzBuzz");
}

TEST(ControlFlow, AreaOverloads) {
    EXPECT_EQ(area(4), 16);
    EXPECT_EQ(area(3, 4), 12);
    EXPECT_EQ(area(5), 25);
}
