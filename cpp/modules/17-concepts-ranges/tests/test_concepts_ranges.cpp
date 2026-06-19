#include <gtest/gtest.h>
#include <vector>
#include <list>
#include <string>
#include <array>
#include "concepts_ranges.hpp"

using namespace m17;

// ---- вспомогательные типы для проверки концептов -------------------

// Складывается: число с operator+ (int подойдёт, но возьмём и свой).
struct Money {
    int cents = 0;
    friend Money operator+(const Money& a, const Money& b) { return {a.cents + b.cents}; }
    friend bool operator==(const Money&, const Money&) = default;
};

// НЕ складывается: нет operator+.
struct NoPlus {
    int x = 0;
};

// =====================================================================
// Задание 1а — концепт Addable
// =====================================================================
TEST(Addable, TrueForArithmeticAndUserPlus) {
    EXPECT_TRUE(Addable<int>);
    EXPECT_TRUE(Addable<double>);
    EXPECT_TRUE(Addable<std::string>);  // строки складываются через +
    EXPECT_TRUE(Addable<Money>);
}

TEST(Addable, FalseWhenNoPlus) {
    EXPECT_FALSE(Addable<NoPlus>);
}

// =====================================================================
// Задание 1б — концепт Container
// =====================================================================
TEST(Container, TrueForStandardContainers) {
    EXPECT_TRUE(Container<std::vector<int>>);
    EXPECT_TRUE(Container<std::list<double>>);
    EXPECT_TRUE((Container<std::array<int, 3>>));
    EXPECT_TRUE(Container<std::string>);
}

TEST(Container, FalseForScalars) {
    EXPECT_FALSE(Container<int>);
    EXPECT_FALSE(Container<double>);
    EXPECT_FALSE(Container<Money>);
}

// =====================================================================
// Задание 2 — sum_container
// =====================================================================
TEST(SumContainer, SumsInts) {
    std::vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(sum_container(v), 15);
}

TEST(SumContainer, SumsDoubles) {
    std::vector<double> v{0.5, 0.25, 0.25};
    EXPECT_DOUBLE_EQ(sum_container(v), 1.0);
}

TEST(SumContainer, EmptyIsZeroValue) {
    std::vector<int> v{};
    EXPECT_EQ(sum_container(v), 0);
}

TEST(SumContainer, WorksOnList) {
    std::list<int> l{10, 20, 30};
    EXPECT_EQ(sum_container(l), 60);
}

TEST(SumContainer, SumsUserType) {
    std::vector<Money> v{{100}, {250}, {50}};
    EXPECT_EQ(sum_container(v), Money{400});
}

// =====================================================================
// Задание 3 — even_times_two_take (ленивый конвейер)
// =====================================================================
TEST(Pipeline, EvenThenDoubleThenTake) {
    std::vector<int> xs{1, 2, 3, 4, 5, 6, 7, 8};
    // чётные: 2,4,6,8 -> удвоить: 4,8,12,16 -> взять 2: {4,8}
    EXPECT_EQ(even_times_two_take(xs, 2), (std::vector<int>{4, 8}));
}

TEST(Pipeline, TakeMoreThanAvailable) {
    std::vector<int> xs{2, 4, 6};
    // чётные все -> удвоить: 4,8,12 -> взять 10: всё
    EXPECT_EQ(even_times_two_take(xs, 10), (std::vector<int>{4, 8, 12}));
}

TEST(Pipeline, NoEvensGivesEmpty) {
    std::vector<int> xs{1, 3, 5, 7};
    EXPECT_TRUE(even_times_two_take(xs, 3).empty());
}

TEST(Pipeline, TakeZeroGivesEmpty) {
    std::vector<int> xs{2, 4, 6, 8};
    EXPECT_TRUE(even_times_two_take(xs, 0).empty());
}

TEST(Pipeline, OrderIsFilterThenTransform) {
    std::vector<int> xs{2, 3, 4, 5, 6};
    // фильтр по ИСХОДНЫМ числам (2,4,6), потом *2 = 4,8,12
    EXPECT_EQ(even_times_two_take(xs, 100), (std::vector<int>{4, 8, 12}));
}

// =====================================================================
// Задание 4 — Sortable + sorted_copy
// =====================================================================
TEST(Sortable, TrueForRandomAccessComparable) {
    EXPECT_TRUE(Sortable<std::vector<int>>);
    EXPECT_TRUE((Sortable<std::array<int, 4>>));
}

TEST(Sortable, FalseForListNoRandomAccess) {
    // std::list — двунаправленный, не random-access -> не Sortable
    EXPECT_FALSE(Sortable<std::list<int>>);
}

TEST(Sortable, FalseWhenElementNotComparable) {
    EXPECT_FALSE(Sortable<std::vector<NoPlus>>);  // у NoPlus нет operator<
}

TEST(SortedCopy, SortsAscending) {
    std::vector<int> v{5, 1, 4, 2, 3};
    EXPECT_EQ(sorted_copy(v), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(SortedCopy, DoesNotMutateInput) {
    std::vector<int> v{3, 1, 2};
    auto out = sorted_copy(v);
    EXPECT_EQ(v, (std::vector<int>{3, 1, 2}));  // исходник не тронут
    EXPECT_EQ(out, (std::vector<int>{1, 2, 3}));
}

TEST(SortedCopy, EmptyStaysEmpty) {
    std::vector<int> v{};
    EXPECT_TRUE(sorted_copy(v).empty());
}

// =====================================================================
// Задание 5 — take_n (свой адаптер)
// =====================================================================
TEST(TakeN, FirstThree) {
    std::vector<int> v{10, 20, 30, 40, 50};
    EXPECT_EQ(take_n(v, 3), (std::vector<int>{10, 20, 30}));
}

TEST(TakeN, MoreThanSizeReturnsAll) {
    std::vector<int> v{1, 2};
    EXPECT_EQ(take_n(v, 100), (std::vector<int>{1, 2}));
}

TEST(TakeN, ZeroIsEmpty) {
    std::vector<int> v{1, 2, 3};
    EXPECT_TRUE(take_n(v, 0).empty());
}

TEST(TakeN, WorksOnList) {
    std::list<int> l{7, 8, 9, 10};
    EXPECT_EQ(take_n(l, 2), (std::vector<int>{7, 8}));
}

TEST(TakeN, WorksOnStrings) {
    std::vector<std::string> v{"a", "b", "c"};
    EXPECT_EQ(take_n(v, 2), (std::vector<std::string>{"a", "b"}));
}
