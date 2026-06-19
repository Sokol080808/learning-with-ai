#include <gtest/gtest.h>
#include <stdexcept>
#include "task09.hpp"

TEST(ErrorHandling, SafeDivide) {
    EXPECT_EQ(safe_divide(10, 2), 5);
    EXPECT_THROW(safe_divide(1, 0), std::invalid_argument);
}

TEST(ErrorHandling, ElementAt) {
    std::vector<int> v{10, 20, 30};
    EXPECT_EQ(element_at(v, 0), 10);
    EXPECT_EQ(element_at(v, 2), 30);
    EXPECT_THROW(element_at(v, 3), std::out_of_range);
    EXPECT_THROW(element_at(v, -1), std::out_of_range);
}

TEST(ErrorHandling, ToInt) {
    EXPECT_EQ(to_int("42"), std::optional<int>(42));
    EXPECT_EQ(to_int("-7"), std::optional<int>(-7));
    EXPECT_EQ(to_int("abc"), std::nullopt);
    EXPECT_EQ(to_int("12abc"), std::nullopt);
    EXPECT_EQ(to_int(""), std::nullopt);
}

TEST(ErrorHandling, FirstEven) {
    EXPECT_EQ(first_even({1, 3, 4, 6}), std::optional<int>(4));
    EXPECT_EQ(first_even({1, 3, 5}), std::nullopt);
    EXPECT_EQ(first_even({}), std::nullopt);
}

TEST(ErrorHandling, Describe) {
    EXPECT_EQ(describe(std::variant<int, std::string>(7)), "int: 7");
    EXPECT_EQ(describe(std::variant<int, std::string>(std::string("hi"))), "string: hi");
}

// --- Задание 6: parse_int ---------------------------------------------------

TEST(ErrorHandling, ParseIntValid) {
    EXPECT_EQ(parse_int("0"), std::optional<int>(0));
    EXPECT_EQ(parse_int("42"), std::optional<int>(42));
    EXPECT_EQ(parse_int("+42"), std::optional<int>(42));
    EXPECT_EQ(parse_int("-7"), std::optional<int>(-7));
    EXPECT_EQ(parse_int("007"), std::optional<int>(7));
    EXPECT_EQ(parse_int("2147483647"), std::optional<int>(2147483647));
    EXPECT_EQ(parse_int("-2147483648"), std::optional<int>(-2147483648));
}

TEST(ErrorHandling, ParseIntInvalid) {
    // Контрольное валидное значение в той же группе — чтобы заглушка,
    // которая всегда возвращает nullopt, не «прошла» этот тест.
    EXPECT_EQ(parse_int("123"), std::optional<int>(123));
    EXPECT_EQ(parse_int(""), std::nullopt);
    EXPECT_EQ(parse_int("abc"), std::nullopt);
    EXPECT_EQ(parse_int("12abc"), std::nullopt);
    EXPECT_EQ(parse_int("  5"), std::nullopt);   // ведущий пробел
    EXPECT_EQ(parse_int("5 "), std::nullopt);    // хвостовой пробел
    EXPECT_EQ(parse_int("+"), std::nullopt);     // только знак, без цифр
    EXPECT_EQ(parse_int("-"), std::nullopt);
    EXPECT_EQ(parse_int("3.14"), std::nullopt);
    EXPECT_EQ(parse_int("1,000"), std::nullopt);
}

TEST(ErrorHandling, ParseIntOverflow) {
    // Граница допустимого должна разбираться, а шаг за неё — нет.
    EXPECT_EQ(parse_int("2147483647"), std::optional<int>(2147483647));
    EXPECT_EQ(parse_int("2147483648"), std::nullopt);    // INT_MAX + 1
    EXPECT_EQ(parse_int("-2147483649"), std::nullopt);   // INT_MIN - 1
    EXPECT_EQ(parse_int("99999999999999999999"), std::nullopt);
}

// --- Задание 7: Expected<T, E> ----------------------------------------------

TEST(ErrorHandling, ExpectedHoldsValue) {
    Expected<int, std::string> e(42);
    EXPECT_TRUE(e.has_value());
    EXPECT_TRUE(static_cast<bool>(e));
    EXPECT_EQ(e.value(), 42);
}

TEST(ErrorHandling, ExpectedHoldsError) {
    Expected<int, std::string> e = Expected<int, std::string>::fail("boom");
    EXPECT_FALSE(e.has_value());
    EXPECT_FALSE(static_cast<bool>(e));
    EXPECT_EQ(e.error(), "boom");
    // А значение у ошибки должно быть недоступно (бросок std::logic_error) —
    // заглушка, читающая variant без проверки, этот тест провалит.
    EXPECT_THROW(e.value(), std::logic_error);
}

TEST(ErrorHandling, ExpectedWrongAccessThrows) {
    Expected<int, std::string> ok(1);
    EXPECT_THROW(ok.error(), std::logic_error);   // у значения нет ошибки

    Expected<int, std::string> bad = Expected<int, std::string>::fail("x");
    EXPECT_THROW(bad.value(), std::logic_error);  // у ошибки нет значения
}

TEST(ErrorHandling, ExpectedSameTypes) {
    // Когда T и E совпадают, fail() обязан создавать именно ошибку.
    Expected<int, int> v(5);
    Expected<int, int> err = Expected<int, int>::fail(5);
    EXPECT_TRUE(v.has_value());
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(v.value(), 5);
    EXPECT_EQ(err.error(), 5);
}

// --- Задание 8: safe_div + DivideByZero -------------------------------------

TEST(ErrorHandling, SafeDivOk) {
    EXPECT_EQ(safe_div(10, 2), 5);
    EXPECT_EQ(safe_div(-9, 3), -3);
}

TEST(ErrorHandling, SafeDivThrowsCustom) {
    EXPECT_THROW(safe_div(1, 0), DivideByZero);
}

TEST(ErrorHandling, SafeDivThrowsDerivesFromRuntimeError) {
    // DivideByZero — это std::runtime_error, поэтому ловится и по базе.
    EXPECT_THROW(safe_div(1, 0), std::runtime_error);
    try {
        safe_div(7, 0);
        FAIL() << "ожидалось исключение DivideByZero";
    } catch (const std::exception& ex) {
        EXPECT_STREQ(ex.what(), "division by zero");
    }
}
