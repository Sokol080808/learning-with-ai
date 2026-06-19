#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <typeinfo>
#include <functional>   // std::bad_function_call
#include <random>
#include <cstdint>
#include <climits>
#include <cmath>
#include <variant>
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

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо новых фиксированных примеров проверяем ИНВАРИАНТЫ type-erasure
// на множестве сгенерированных входов с одним фиксированным сидом mt19937,
// чтобы CI никогда не «мигал». Везде, где есть готовый аналог в стандартной
// библиотеке (std::function, std::any, std::visit), сверяемся с ним как с
// оракулом. Плюс явные крайние случаи: пусто, 0/1, INT_MIN/MAX, дубликаты.

// ---- Задание 1. Function: оракул std::function + арифметические инварианты ----

TEST(FunctionProps, MatchesStdFunctionOracleOnRandomInputs) {
    std::mt19937 rng(0xC0FFEEu);
    // Берём умеренный диапазон, чтобы x*3 и x*x не переполняли int.
    std::uniform_int_distribution<int> dist(-20000, 20000);
    for (int i = 0; i < 400; ++i) {
        int x = dist(rng);
        // Одна и та же лямбда в нашей обёртке и в std::function — результат обязан совпасть.
        auto lam = [](int v) { return v * 3 - 7; };
        Function<int(int)> mine = lam;
        std::function<int(int)> oracle = lam;
        EXPECT_EQ(mine(x), oracle(x)) << "x=" << x;
        // И прямой инвариант формулы.
        EXPECT_EQ(mine(x), x * 3 - 7) << "x=" << x;
    }
}

TEST(FunctionProps, CapturedStateIsAdditiveInvariant) {
    std::mt19937 rng(0x1234u);
    std::uniform_int_distribution<int> dist(-1000000, 1000000);
    for (int i = 0; i < 300; ++i) {
        int base = dist(rng);
        int x = dist(rng);
        // Захват по значению: f(x) == base + x для любых base, x в диапазоне.
        Function<long long(int)> f =
            [base](int v) -> long long { return static_cast<long long>(base) + v; };
        EXPECT_EQ(f(x), static_cast<long long>(base) + x) << "base=" << base << " x=" << x;
    }
}

TEST(FunctionProps, MultiArgConcatRoundTripVsOracle) {
    std::mt19937 rng(0xBEEFu);
    std::uniform_int_distribution<int> ndist(-999, 999);
    std::uniform_int_distribution<int> ldist(0, 6);
    const std::string alphabet = "abcXYZ_0";
    std::uniform_int_distribution<std::size_t> cdist(0, alphabet.size() - 1);
    for (int i = 0; i < 300; ++i) {
        std::size_t len = static_cast<std::size_t>(ldist(rng));
        std::string s;
        for (std::size_t k = 0; k < len; ++k) s.push_back(alphabet[cdist(rng)]);
        int n = ndist(rng);
        auto lam = [](const std::string& str, int num) { return str + std::to_string(num); };
        Function<std::string(const std::string&, int)> mine = lam;
        std::function<std::string(const std::string&, int)> oracle = lam;
        EXPECT_EQ(mine(s, n), oracle(s, n));
        EXPECT_EQ(mine(s, n), s + std::to_string(n));
    }
}

TEST(FunctionProps, VoidSideEffectMatchesArgument) {
    std::mt19937 rng(0xABCDu);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
    for (int i = 0; i < 300; ++i) {
        int x = dist(rng);
        int seen = 0;
        Function<void(int)> f = [&seen](int v) { seen = v; };
        f(x);
        EXPECT_EQ(seen, x);
    }
}

TEST(FunctionProps, EmptyAlwaysThrowsBadFunctionCall) {
    std::mt19937 rng(0x5151u);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
    for (int i = 0; i < 200; ++i) {
        Function<int(int)> empty;            // дефолтно сконструирован — пуст
        EXPECT_FALSE(static_cast<bool>(empty));
        EXPECT_THROW(empty(dist(rng)), std::bad_function_call);
    }
}

TEST(FunctionProps, IdentityIsInvolutionEdgeValues) {
    // Крайние значения int: тождественная функция должна возвращать ввод как есть.
    Function<int(int)> id = [](int v) { return v; };
    for (int x : {INT_MIN, INT_MIN + 1, -1, 0, 1, INT_MAX - 1, INT_MAX}) {
        EXPECT_EQ(id(x), x) << "x=" << x;
        EXPECT_EQ(id(id(x)), x) << "x=" << x;  // involution: id(id(x)) == x
    }
}

// ---- Задание 2. Any: round-trip any_cast, type(), независимость копии ----

TEST(AnyProps, IntRoundTripAndType) {
    std::mt19937 rng(0xA11Eu);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
    for (int i = 0; i < 400; ++i) {
        int v = dist(rng);
        Any a = v;
        EXPECT_TRUE(a.has_value());
        EXPECT_EQ(a.type(), typeid(int));
        EXPECT_EQ(any_cast<int>(a), v) << "v=" << v;   // round-trip: вынули то, что положили
    }
}

TEST(AnyProps, DoubleRoundTripAndType) {
    std::mt19937 rng(0xD0DBu);
    std::uniform_real_distribution<double> dist(-1e9, 1e9);
    for (int i = 0; i < 300; ++i) {
        double v = dist(rng);
        Any a = v;
        EXPECT_EQ(a.type(), typeid(double));
        EXPECT_DOUBLE_EQ(any_cast<double>(a), v);
    }
}

TEST(AnyProps, StringRoundTrip) {
    std::mt19937 rng(0x57A1u);
    std::uniform_int_distribution<int> ldist(0, 12);
    std::uniform_int_distribution<int> cdist(32, 126);  // печатные ASCII
    for (int i = 0; i < 300; ++i) {
        std::string v;
        int len = ldist(rng);
        for (int k = 0; k < len; ++k) v.push_back(static_cast<char>(cdist(rng)));
        Any a = v;
        EXPECT_EQ(a.type(), typeid(std::string));
        EXPECT_EQ(any_cast<std::string>(a), v);
    }
}

TEST(AnyProps, CopyIsDeepAndIndependent) {
    std::mt19937 rng(0xC0C0u);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
    for (int i = 0; i < 300; ++i) {
        int v = dist(rng);
        Any a = v;
        Any b = a;                 // глубокая копия через clone()
        // Обе хранят одно значение и один тип, но это независимые объекты.
        EXPECT_EQ(any_cast<int>(a), v);
        EXPECT_EQ(any_cast<int>(b), v);
        EXPECT_EQ(b.type(), typeid(int));
        // Присваивание копии другого значения не должно затронуть исходник.
        Any c = v ^ 0x7FFFFFFF;     // заведомо иное значение в общем случае
        b = c;
        EXPECT_EQ(any_cast<int>(a), v) << "источник испорчен при копировании b";
    }
}

TEST(AnyProps, WrongTypeCastThrowsBadCast) {
    std::mt19937 rng(0xBAD0u);
    std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);
    for (int i = 0; i < 300; ++i) {
        Any a = dist(rng);                       // внутри int
        EXPECT_THROW(any_cast<std::string>(a), std::bad_cast);  // конкретный тип
        EXPECT_THROW(any_cast<double>(a), std::bad_cast);
    }
}

TEST(AnyProps, EmptyEdgeCases) {
    Any empty;
    EXPECT_FALSE(empty.has_value());
    EXPECT_EQ(empty.type(), typeid(void));
    // any_cast на пустом Any обязан кинуть bad_cast, а не вернуть мусор.
    EXPECT_THROW(any_cast<int>(empty), std::bad_cast);
    // Копия пустого тоже пуста.
    Any copy = empty;
    EXPECT_FALSE(copy.has_value());
    EXPECT_EQ(copy.type(), typeid(void));
}

// ---- Задание 3. area / variant: оракул-формула, неотрицательность, монотонность ----

TEST(AreaProps, CircleMatchesFormulaAndIsMonotone) {
    constexpr double pi = 3.141592653589793;
    std::mt19937 rng(0xF1F1u);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);
    for (int i = 0; i < 400; ++i) {
        double r = dist(rng);
        Shape s = Circle{r};
        double got = area(s);
        EXPECT_DOUBLE_EQ(got, pi * r * r) << "r=" << r;  // оракул-формула
        EXPECT_GE(got, 0.0);                              // площадь неотрицательна
        // Монотонность по радиусу: больший r → не меньшая площадь.
        double r2 = r + dist(rng);
        EXPECT_GE(area(Shape{Circle{r2}}), got) << "r=" << r << " r2=" << r2;
    }
}

TEST(AreaProps, SquareMatchesFormulaAndIsMonotone) {
    std::mt19937 rng(0x5151u);
    std::uniform_real_distribution<double> dist(0.0, 1000.0);
    for (int i = 0; i < 400; ++i) {
        double side = dist(rng);
        Shape s = Square{side};
        double got = area(s);
        EXPECT_DOUBLE_EQ(got, side * side) << "side=" << side;
        EXPECT_GE(got, 0.0);
        double side2 = side + dist(rng);
        EXPECT_GE(area(Shape{Square{side2}}), got);
    }
}

TEST(AreaProps, MixedVectorSumIsOrderIndependent) {
    constexpr double pi = 3.141592653589793;
    std::mt19937 rng(0x9A9Au);
    std::uniform_real_distribution<double> dist(0.0, 50.0);
    std::bernoulli_distribution coin(0.5);
    for (int iter = 0; iter < 200; ++iter) {
        std::vector<Shape> shapes;
        double expected = 0.0;
        int n = 1 + static_cast<int>(rng() % 8);
        for (int k = 0; k < n; ++k) {
            double d = dist(rng);
            if (coin(rng)) { shapes.push_back(Circle{d}); expected += pi * d * d; }
            else           { shapes.push_back(Square{d}); expected += d * d; }
        }
        double total = 0.0;
        for (const auto& sh : shapes) total += area(sh);
        EXPECT_NEAR(total, expected, 1e-6 * (1.0 + std::fabs(expected)));
    }
}

TEST(AreaProps, DegenerateZeroIsZero) {
    EXPECT_DOUBLE_EQ(area(Shape{Circle{0.0}}), 0.0);
    EXPECT_DOUBLE_EQ(area(Shape{Square{0.0}}), 0.0);
}

// ---- Задание 4. Drawable: делегирование = вызов нижележащего draw() ----

struct PropChar {
    char c;
    std::string draw() const { return std::string(1, c); }
};

struct PropRepeat {
    char c;
    int n;
    std::string draw() const { return std::string(n < 0 ? 0 : static_cast<std::size_t>(n), c); }
};

TEST(DrawableProps, DelegatesExactlyToUnderlyingDraw) {
    std::mt19937 rng(0xDEADu);
    std::uniform_int_distribution<int> cdist(33, 126);
    std::uniform_int_distribution<int> ndist(0, 15);
    for (int i = 0; i < 400; ++i) {
        char c = static_cast<char>(cdist(rng));
        int n = ndist(rng);
        PropRepeat obj{c, n};
        std::string direct = obj.draw();      // тот же объект напрямую
        Drawable d = obj;                      // обёрнут в type-erased Drawable
        EXPECT_EQ(d.draw(), direct);           // делегирование без искажений
        EXPECT_EQ(d.draw().size(), static_cast<std::size_t>(n));
    }
}

TEST(DrawableProps, HeterogeneousConcatMatchesManualSum) {
    std::mt19937 rng(0xBABEu);
    std::uniform_int_distribution<int> cdist(33, 126);
    std::uniform_int_distribution<int> ndist(0, 8);
    std::bernoulli_distribution coin(0.5);
    for (int iter = 0; iter < 200; ++iter) {
        std::vector<Drawable> scene;
        std::string expected;
        int parts = 1 + static_cast<int>(rng() % 6);
        for (int k = 0; k < parts; ++k) {
            char c = static_cast<char>(cdist(rng));
            if (coin(rng)) {
                scene.emplace_back(PropChar{c});
                expected += std::string(1, c);
            } else {
                int n = ndist(rng);
                scene.emplace_back(PropRepeat{c, n});
                expected += std::string(static_cast<std::size_t>(n), c);
            }
        }
        std::string out;
        for (const auto& item : scene) out += item.draw();
        EXPECT_EQ(out, expected);  // инвариант: конкатенация делегатов == ручная сумма
    }
}

TEST(DrawableProps, EmptyDrawIsEmptyString) {
    // Крайний случай: объект, рисующий пустую строку, делегирует её без изменений.
    Drawable d = PropRepeat{'x', 0};
    EXPECT_EQ(d.draw(), "");
    EXPECT_TRUE(d.draw().empty());
}
