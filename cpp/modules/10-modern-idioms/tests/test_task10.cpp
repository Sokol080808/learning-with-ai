#include <gtest/gtest.h>
#include "task10.hpp"
#include <stdexcept>
#include <string>
#include <vector>

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

// --- Задание 6: split_view ---

TEST(Modern, SplitViewBasic) {
    auto parts = split_view("a,b,c", ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");
}

TEST(Modern, SplitViewEmptyPieces) {
    auto parts = split_view(",a,", ',');
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "a");
    EXPECT_EQ(parts[2], "");
}

TEST(Modern, SplitViewSingleAndEmpty) {
    auto one = split_view("hello", ',');
    ASSERT_EQ(one.size(), 1u);
    EXPECT_EQ(one[0], "hello");

    auto empty = split_view("", ',');
    ASSERT_EQ(empty.size(), 1u);
    EXPECT_EQ(empty[0], "");
}

TEST(Modern, SplitViewNoCopy) {
    // Куски должны СМОТРЕТЬ внутрь исходной строки, а не быть копиями.
    std::string src = "alpha:beta:gamma";
    auto parts = split_view(src, ':');
    ASSERT_EQ(parts.size(), 3u);
    // Адрес первого символа куска совпадает с адресом внутри src — значит копии нет.
    EXPECT_EQ(parts[0].data(), src.data());
    EXPECT_EQ(parts[1].data(), src.data() + 6);
}

// --- Задание 7: ScopeGuard ---

TEST(Modern, ScopeGuardRunsOnExit) {
    int calls = 0;
    {
        ScopeGuard g([&] { ++calls; });
        EXPECT_EQ(calls, 0);  // ещё не сработал — мы внутри блока
    }
    EXPECT_EQ(calls, 1);  // сработал ровно один раз при выходе
}

TEST(Modern, ScopeGuardRunsOnThrow) {
    int calls = 0;
    try {
        ScopeGuard g([&] { ++calls; });
        throw std::runtime_error("boom");
    } catch (const std::runtime_error&) {
        // исключение поймали — но guard уже должен был отработать
    }
    EXPECT_EQ(calls, 1);
}

TEST(Modern, ScopeGuardDismiss) {
    int dismissed = 0;
    int kept = 0;
    {
        ScopeGuard g_dismissed([&] { ++dismissed; });
        ScopeGuard g_kept([&] { ++kept; });
        g_dismissed.dismiss();  // отменяем только первый
    }
    EXPECT_EQ(dismissed, 0);  // отменён — не сработал
    EXPECT_EQ(kept, 1);       // не отменён — сработал ровно один раз
}

TEST(Modern, ScopeGuardFactory) {
    int calls = 0;
    {
        auto g = make_scope_guard([&] { ++calls; });
    }
    EXPECT_EQ(calls, 1);
}

// --- Задание 8: divmod + [[nodiscard]] + structured bindings ---

TEST(Modern, DivmodBasic) {
    auto [q, r] = divmod(17, 5);
    EXPECT_EQ(q, 3);
    EXPECT_EQ(r, 2);
}

TEST(Modern, DivmodExact) {
    auto [q, r] = divmod(20, 4);
    EXPECT_EQ(q, 5);
    EXPECT_EQ(r, 0);
}

TEST(Modern, DivmodNegative) {
    auto [q, r] = divmod(-7, 2);
    // C++ truncation toward zero: -7 / 2 == -3, -7 % 2 == -1
    EXPECT_EQ(q, -3);
    EXPECT_EQ(r, -1);
}

TEST(Modern, DivmodThrowsOnZero) {
    EXPECT_THROW(divmod(1, 0), std::invalid_argument);
}
