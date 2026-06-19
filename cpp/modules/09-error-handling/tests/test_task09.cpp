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
