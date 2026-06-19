#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <typeinfo>
#include <functional>   // std::bad_function_call
#include "task18.hpp"

// ============================ Задание 1. Function ============================

int triple(int x) { return x * 3; }

TEST(Function, CallsStoredLambda) {
    Function<int(int)> f = [](int x) { return x + 1; };
    EXPECT_EQ(f(41), 42);
}

TEST(Function, StoresFreeFunctionPointer) {
    Function<int(int)> f = &triple;
    EXPECT_EQ(f(4), 12);
}

TEST(Function, CapturesState) {
    int base = 100;
    Function<int(int)> f = [base](int x) { return base + x; };
    EXPECT_EQ(f(5), 105);
}

TEST(Function, MultipleArgsAndOtherTypes) {
    Function<std::string(const std::string&, int)> f =
        [](const std::string& s, int n) { return s + std::to_string(n); };
    EXPECT_EQ(f("x", 7), "x7");
}

TEST(Function, ReturnsVoid) {
    int seen = 0;
    Function<void(int)> f = [&seen](int x) { seen = x; };
    f(9);
    EXPECT_EQ(seen, 9);
}

TEST(Function, BoolConversion) {
    Function<int(int)> empty;
    EXPECT_FALSE(static_cast<bool>(empty));
    Function<int(int)> full = [](int x) { return x; };
    EXPECT_TRUE(static_cast<bool>(full));
}

TEST(Function, EmptyCallThrowsBadFunctionCall) {
    Function<int(int)> empty;
    // Конкретный тип, а не std::logic_error, чтобы TODO-заглушка не прошла.
    EXPECT_THROW(empty(1), std::bad_function_call);
}

// ============================ Задание 2. Any ============================

TEST(Any, HoldsIntAndCastsBack) {
    Any a = 42;
    EXPECT_TRUE(a.has_value());
    EXPECT_EQ(any_cast<int>(a), 42);
}

TEST(Any, HoldsStringAndCastsBack) {
    Any a = std::string("hello");
    EXPECT_EQ(any_cast<std::string>(a), "hello");
}

TEST(Any, ReportsType) {
    Any a = 3.5;
    EXPECT_EQ(a.type(), typeid(double));
}

TEST(Any, DefaultHasNoValue) {
    Any empty;
    EXPECT_FALSE(empty.has_value());
    EXPECT_EQ(empty.type(), typeid(void));
    // Контраст: непустой Any обязан сообщать has_value()==true — это и
    // ловит заглушку, которая всегда возвращает false.
    Any full = 7;
    EXPECT_TRUE(full.has_value());
}

TEST(Any, WrongCastThrowsBadCast) {
    Any a = 42;
    // Тип int, читаем как std::string → std::bad_cast (НЕ std::logic_error).
    EXPECT_THROW(any_cast<std::string>(a), std::bad_cast);
}

TEST(Any, CopyIsIndependent) {
    Any a = 10;
    Any b = a;            // глубокая копия через clone()
    EXPECT_EQ(any_cast<int>(a), 10);
    EXPECT_EQ(any_cast<int>(b), 10);
    EXPECT_EQ(b.type(), typeid(int));
}

// ============================ Задание 3. area / variant ============================

TEST(Area, CircleArea) {
    Shape s = Circle{2.0};
    EXPECT_DOUBLE_EQ(area(s), 3.141592653589793 * 4.0);
}

TEST(Area, SquareArea) {
    Shape s = Square{3.0};
    EXPECT_DOUBLE_EQ(area(s), 9.0);
}

TEST(Area, WorksAcrossVector) {
    std::vector<Shape> shapes{Circle{1.0}, Square{2.0}, Square{0.5}};
    double total = 0.0;
    for (const auto& sh : shapes) total += area(sh);
    EXPECT_DOUBLE_EQ(total, 3.141592653589793 + 4.0 + 0.25);
}

// ============================ Задание 4. Drawable ============================

struct Dot {
    std::string draw() const { return "."; }
};

struct Line {
    int len;
    std::string draw() const { return std::string(len, '-'); }
};

TEST(Drawable, DelegatesToHeldObject) {
    Drawable d = Dot{};
    EXPECT_EQ(d.draw(), ".");
}

TEST(Drawable, DifferentTypesNoCommonBase) {
    Drawable d = Line{4};
    EXPECT_EQ(d.draw(), "----");
}

TEST(Drawable, HeterogeneousContainer) {
    std::vector<Drawable> scene;
    scene.emplace_back(Dot{});
    scene.emplace_back(Line{3});
    std::string out;
    for (const auto& item : scene) out += item.draw();
    EXPECT_EQ(out, ".---");
}
