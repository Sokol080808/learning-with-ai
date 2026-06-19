#include <gtest/gtest.h>
#include <string>
#include <stdexcept>
#include <type_traits>
#include "task07.hpp"

TEST(Templates, MyMax) {
    EXPECT_EQ(my_max(3, 5), 5);
    EXPECT_EQ(my_max(5, 3), 5);
    EXPECT_DOUBLE_EQ(my_max(2.5, 1.5), 2.5);
    EXPECT_EQ(my_max(std::string("a"), std::string("b")), "b");
}

TEST(Templates, ClampValue) {
    EXPECT_EQ(clamp_value(5, 0, 10), 5);
    EXPECT_EQ(clamp_value(-3, 0, 10), 0);
    EXPECT_EQ(clamp_value(42, 0, 10), 10);
    EXPECT_DOUBLE_EQ(clamp_value(1.5, 0.0, 1.0), 1.0);
}

TEST(Templates, SumVector) {
    EXPECT_EQ(sum_vector(std::vector<int>{1, 2, 3, 4}), 10);
    EXPECT_DOUBLE_EQ(sum_vector(std::vector<double>{1.5, 2.5}), 4.0);
    EXPECT_EQ(sum_vector(std::vector<int>{}), 0);
}

TEST(Templates, PairSwapped) {
    Pair<int, std::string> p(1, "one");
    auto q = p.swapped();
    EXPECT_EQ(q.first, "one");
    EXPECT_EQ(q.second, 1);
}

TEST(Templates, IsPowerOfTwo) {
    EXPECT_TRUE(is_power_of_two(1));
    EXPECT_TRUE(is_power_of_two(2));
    EXPECT_TRUE(is_power_of_two(1024));
    EXPECT_FALSE(is_power_of_two(0));
    EXPECT_FALSE(is_power_of_two(3));
    EXPECT_FALSE(is_power_of_two(-4));
}

// ---- 6. Matrix<T, Rows, Cols> (нетиповые параметры) ------------------------

TEST(Matrix, Dimensions) {
    EXPECT_EQ((Matrix<int, 2, 3>::rows()), 2u);
    EXPECT_EQ((Matrix<int, 2, 3>::cols()), 3u);
    // Размер зашит в тип, доступен на этапе компиляции:
    static_assert(Matrix<double, 4, 5>::rows() == 4);
    static_assert(Matrix<double, 4, 5>::cols() == 5);
}

TEST(Matrix, AtReadWrite) {
    Matrix<int, 2, 2> m;
    m.at(0, 0) = 1;
    m.at(0, 1) = 2;
    m.at(1, 0) = 3;
    m.at(1, 1) = 4;
    EXPECT_EQ(m.at(0, 0), 1);
    EXPECT_EQ(m.at(0, 1), 2);
    EXPECT_EQ(m.at(1, 0), 3);
    EXPECT_EQ(m.at(1, 1), 4);
    // row-major: (1,0) лежит на data[2]
    EXPECT_EQ(m.data[2], 3);
}

TEST(Matrix, AtConst) {
    Matrix<int, 2, 2> m;
    m.at(1, 1) = 9;
    const Matrix<int, 2, 2>& cm = m;
    EXPECT_EQ(cm.at(1, 1), 9);
}

TEST(Matrix, AtOutOfRangeThrows) {
    Matrix<int, 2, 3> m;
    EXPECT_THROW(m.at(2, 0), std::out_of_range);  // строка вне границ
    EXPECT_THROW(m.at(0, 3), std::out_of_range);  // столбец вне границ
    EXPECT_NO_THROW(m.at(1, 2));                   // последний валидный элемент
}

TEST(Matrix, ElementwiseAdd) {
    Matrix<int, 2, 2> a;
    a.at(0, 0) = 1; a.at(0, 1) = 2; a.at(1, 0) = 3; a.at(1, 1) = 4;
    Matrix<int, 2, 2> b;
    b.at(0, 0) = 10; b.at(0, 1) = 20; b.at(1, 0) = 30; b.at(1, 1) = 40;

    Matrix<int, 2, 2> c = a + b;
    EXPECT_EQ(c.at(0, 0), 11);
    EXPECT_EQ(c.at(0, 1), 22);
    EXPECT_EQ(c.at(1, 0), 33);
    EXPECT_EQ(c.at(1, 1), 44);
    // исходные матрицы не должны измениться
    EXPECT_EQ(a.at(0, 0), 1);
    EXPECT_EQ(b.at(0, 0), 10);
}

// ---- 7. Специализация type_name (bool / указатели) ------------------------

TEST(TypeName, PrimaryTemplate) {
    EXPECT_STREQ(type_name(42), "other");
    EXPECT_STREQ(type_name(3.14), "other");
    EXPECT_STREQ(type_name(std::string("hi")), "other");
}

TEST(TypeName, BoolSpecialization) {
    EXPECT_STREQ(type_name(true), "bool");
    EXPECT_STREQ(type_name(false), "bool");
    // важно: bool не должен срабатывать как "other"
    bool b = true;
    EXPECT_STREQ(type_name(b), "bool");
}

TEST(TypeName, PointerOverload) {
    int x = 5;
    int* p = &x;
    EXPECT_STREQ(type_name(p), "pointer");
    const char* s = "abc";
    EXPECT_STREQ(type_name(s), "pointer");
    double* dp = nullptr;
    EXPECT_STREQ(type_name(dp), "pointer");
}

// ---- 8. compile-time Pow<Base, Exp> ---------------------------------------

TEST(Pow, BasicValues) {
    EXPECT_EQ((Pow<2, 0>::value), 1);
    EXPECT_EQ((Pow<2, 1>::value), 2);
    EXPECT_EQ((Pow<2, 10>::value), 1024);
    EXPECT_EQ((Pow<3, 4>::value), 81);
    EXPECT_EQ((Pow<5, 3>::value), 125);
    EXPECT_EQ((Pow<7, 0>::value), 1);
}

// Используем результат как РАЗМЕР массива: это работает только если значение —
// настоящая compile-time константа. Когда задание решено правильно, размер == 16.
template <unsigned N>
constexpr std::size_t pow_as_array_size() {
    std::array<int, Pow<2, N>::value> arr{};  // размер берётся из шаблона на этапе компиляции
    return arr.size();
}

TEST(Pow, IsCompileTime) {
    // pow_v — variable template поверх Pow.
    EXPECT_EQ((pow_v<3, 3>), 27);
    // Значение реально использовано как размер массива => оно compile-time.
    EXPECT_EQ(pow_as_array_size<4>(), 16u);  // Pow<2,4> == 16
    EXPECT_EQ(pow_as_array_size<5>(), 32u);  // Pow<2,5> == 32
}
