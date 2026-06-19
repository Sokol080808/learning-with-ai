#include <gtest/gtest.h>
#include "task10.hpp"

TEST(Modern, MinMax) {
    auto [lo, hi] = minmax_of({3, 1, 4, 1, 5, 9, 2});
    EXPECT_EQ(lo, 1);
    EXPECT_EQ(hi, 9);
}

TEST(Modern, ColorName) {
    EXPECT_EQ(color_name(Color::Red), "Red");
    EXPECT_EQ(color_name(Color::Green), "Green");
    EXPECT_EQ(color_name(Color::Blue), "Blue");
}

TEST(Modern, SquareConstexpr) {
    // Использование в constexpr-контексте: компилируется, только если square_ce —
    // корректная constexpr-функция. Значение проверяем в рантайме, чтобы недописанный
    // стаб не ломал СБОРКУ всего проекта (он просто даст красный тест).
    constexpr int v5 = square_ce(5);
    constexpr int v0 = square_ce(0);
    EXPECT_EQ(v5, 25);
    EXPECT_EQ(v0, 0);
    EXPECT_EQ(square_ce(7), 49);  // и в рантайме тоже
}

TEST(Modern, EvensSquared) {
    EXPECT_EQ(evens_squared({1, 2, 3, 4, 5, 6}), (std::vector<int>{4, 16, 36}));
    EXPECT_TRUE(evens_squared({1, 3, 5}).empty());
}

TEST(Modern, SumValues) {
    std::map<std::string, int> m{{"a", 1}, {"b", 2}, {"c", 3}};
    EXPECT_EQ(sum_values(m), 6);
    EXPECT_EQ(sum_values({}), 0);
}
