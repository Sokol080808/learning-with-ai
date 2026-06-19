#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <list>
#include <type_traits>
#include "task16.hpp"

// ---------------------------------------------------------------------
// Задание 1. variadic + fold
// ---------------------------------------------------------------------
TEST(Variadic, SumInts) {
    EXPECT_EQ(adv::sum(1, 2, 3, 4), 10);
    EXPECT_EQ(adv::sum(42), 42);
    EXPECT_EQ(adv::sum(-5, 5), 0);
}

TEST(Variadic, SumDoubles) {
    EXPECT_DOUBLE_EQ(adv::sum(1.5, 2.5, 1.0), 5.0);
}

TEST(Variadic, ToStringAll) {
    EXPECT_EQ(adv::to_string_all(1, 2, 3), "1, 2, 3");
    EXPECT_EQ(adv::to_string_all(42), "42");
    EXPECT_EQ(adv::to_string_all(7, 8), "7, 8");
}

TEST(Variadic, ToStringAllMixedNumeric) {
    // int + double + int -> std::to_string каждого, склейка через ", "
    EXPECT_EQ(adv::to_string_all(1, 2, 3, 4, 5), "1, 2, 3, 4, 5");
}

// ---------------------------------------------------------------------
// Задание 2. свои type traits
// ---------------------------------------------------------------------
TEST(Traits, RemoveConst) {
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<const int>, int>));
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<int>, int>));
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<const double>, double>));
    // const должен сниматься только с верхнего уровня
    EXPECT_TRUE((std::is_same_v<adv::my_remove_const_t<const char*>, const char*>));
}

TEST(Traits, IsPointer) {
    EXPECT_TRUE(adv::my_is_pointer_v<int*>);
    EXPECT_TRUE(adv::my_is_pointer_v<const char*>);
    EXPECT_TRUE(adv::my_is_pointer_v<double*>);
    EXPECT_FALSE(adv::my_is_pointer_v<int>);
    EXPECT_FALSE(adv::my_is_pointer_v<double>);
    EXPECT_FALSE(adv::my_is_pointer_v<std::string>);
}

TEST(Traits, IsSame) {
    EXPECT_TRUE((adv::my_is_same_v<int, int>));
    EXPECT_TRUE((adv::my_is_same_v<double, double>));
    EXPECT_FALSE((adv::my_is_same_v<int, double>));
    EXPECT_FALSE((adv::my_is_same_v<int, long>));
    EXPECT_FALSE((adv::my_is_same_v<const int, int>));
}

// ---------------------------------------------------------------------
// Задание 3. enable_if-перегрузка describe
// ---------------------------------------------------------------------
TEST(EnableIf, DescribeIntegral) {
    EXPECT_EQ(adv::describe(5), "integer");
    EXPECT_EQ(adv::describe(0), "integer");
    EXPECT_EQ(adv::describe(static_cast<long>(123)), "integer");
}

TEST(EnableIf, DescribeFloating) {
    EXPECT_EQ(adv::describe(3.14), "floating");
    EXPECT_EQ(adv::describe(2.0f), "floating");
}

// ---------------------------------------------------------------------
// Задание 4. CRTP Comparable
// ---------------------------------------------------------------------
namespace {
struct Money : adv::Comparable<Money> {
    int cents;
    explicit Money(int c) : cents(c) {}
    bool operator<(const Money& o) const { return cents < o.cents; }
    bool operator==(const Money& o) const { return cents == o.cents; }
};
}  // namespace

TEST(CRTP, DerivedOperators) {
    Money a{100}, b{200}, c{100};

    EXPECT_TRUE(a != b);
    EXPECT_FALSE(a != c);

    EXPECT_TRUE(b > a);
    EXPECT_FALSE(a > b);

    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(b <= a);

    EXPECT_TRUE(b >= a);
    EXPECT_TRUE(a >= c);
    EXPECT_FALSE(a >= b);
}

// ---------------------------------------------------------------------
// Задание 5. tag dispatch
// ---------------------------------------------------------------------
TEST(TagDispatch, RandomAccessUsesFastPath) {
    adv::fast_path_calls() = 0;
    std::vector<int> v{10, 20, 30, 40, 50};
    EXPECT_EQ(adv::distance_dispatch(v.begin(), v.end()), 5);
    EXPECT_EQ(adv::fast_path_calls(), 1);  // быстрый путь сработал
}

TEST(TagDispatch, ListUsesSlowPath) {
    adv::fast_path_calls() = 0;
    std::list<int> l{1, 2, 3, 4};
    EXPECT_EQ(adv::distance_dispatch(l.begin(), l.end()), 4);
    EXPECT_EQ(adv::fast_path_calls(), 0);  // быстрый путь НЕ должен был сработать
}

TEST(TagDispatch, EmptyRange) {
    adv::fast_path_calls() = 0;
    std::vector<int> v;
    EXPECT_EQ(adv::distance_dispatch(v.begin(), v.end()), 0);
    std::list<int> l;
    EXPECT_EQ(adv::distance_dispatch(l.begin(), l.end()), 0);
}
