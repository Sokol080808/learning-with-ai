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

TEST(ControlFlow, Lcm) {
    EXPECT_EQ(lcm(4, 6), 12);
    EXPECT_EQ(lcm(3, 5), 15);
    EXPECT_EQ(lcm(6, 6), 6);
    EXPECT_EQ(lcm(21, 6), 42);
    EXPECT_EQ(lcm(0, 7), 0);   // НОК с нулём — ноль
    EXPECT_EQ(lcm(7, 0), 0);
}

TEST(ControlFlow, Sieve) {
    EXPECT_EQ(sieve(1), std::vector<int>{});
    EXPECT_EQ(sieve(2), (std::vector<int>{2}));
    EXPECT_EQ(sieve(10), (std::vector<int>{2, 3, 5, 7}));
    EXPECT_EQ(sieve(20), (std::vector<int>{2, 3, 5, 7, 11, 13, 17, 19}));
    // граница включается: 13 — простое
    EXPECT_EQ(sieve(13), (std::vector<int>{2, 3, 5, 7, 11, 13}));
}

TEST(ControlFlow, BinarySearchFound) {
    std::vector<int> v{1, 3, 5, 7, 9, 11};
    EXPECT_EQ(binary_search(v, 1), 0);
    EXPECT_EQ(binary_search(v, 7), 3);
    EXPECT_EQ(binary_search(v, 11), 5);
    EXPECT_EQ(binary_search(v, 5), 2);
}

TEST(ControlFlow, BinarySearchMissing) {
    std::vector<int> v{1, 3, 5, 7, 9, 11};
    EXPECT_EQ(binary_search(v, 0), -1);   // меньше всех
    EXPECT_EQ(binary_search(v, 4), -1);   // в середине, но отсутствует
    EXPECT_EQ(binary_search(v, 12), -1);  // больше всех
    EXPECT_EQ(binary_search({}, 5), -1);  // пустой вектор
}
