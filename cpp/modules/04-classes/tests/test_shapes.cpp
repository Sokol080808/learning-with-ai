#include <gtest/gtest.h>
#include "shapes.hpp"

#include <memory>
#include <vector>
#include <numeric>   // std::accumulate
#include <cmath>
#include <numbers>   // std::numbers::pi (C++20)
#include <random>
#include <string>
#include <type_traits>

// ──────────────────────────────────────────────────────────────────────────
// Вспомогательная константа
// ──────────────────────────────────────────────────────────────────────────
static constexpr double PI = std::numbers::pi;

// ──────────────────────────────────────────────────────────────────────────
// Задание 5. Тесты иерархии фигур.
// ──────────────────────────────────────────────────────────────────────────

// --- Circle: точные значения с допуском для π ----------------------------

TEST(Circle, Area) {
    Circle c(1.0);
    EXPECT_NEAR(c.area(), PI, 1e-12);
}

TEST(Circle, AreaRadius2) {
    Circle c(2.0);
    EXPECT_NEAR(c.area(), 4.0 * PI, 1e-12);
}

TEST(Circle, Perimeter) {
    Circle c(1.0);
    EXPECT_NEAR(c.perimeter(), 2.0 * PI, 1e-12);
}

TEST(Circle, PerimeterRadius3) {
    Circle c(3.0);
    EXPECT_NEAR(c.perimeter(), 6.0 * PI, 1e-12);
}

TEST(Circle, Name) {
    Circle c(1.0);
    EXPECT_EQ(c.name(), "Circle");
}

TEST(Circle, RadiusAccessor) {
    Circle c(7.5);
    EXPECT_DOUBLE_EQ(c.radius(), 7.5);
}

// --- Rectangle: точные значения ------------------------------------------

TEST(Rectangle, Area) {
    Rectangle r(3.0, 4.0);
    EXPECT_DOUBLE_EQ(r.area(), 12.0);
}

TEST(Rectangle, AreaSquare) {
    Rectangle r(5.0, 5.0);
    EXPECT_DOUBLE_EQ(r.area(), 25.0);
}

TEST(Rectangle, Perimeter) {
    Rectangle r(3.0, 4.0);
    EXPECT_DOUBLE_EQ(r.perimeter(), 14.0);
}

TEST(Rectangle, Name) {
    Rectangle r(1.0, 1.0);
    EXPECT_EQ(r.name(), "Rectangle");
}

TEST(Rectangle, Accessors) {
    Rectangle r(6.0, 9.0);
    EXPECT_DOUBLE_EQ(r.width(),  6.0);
    EXPECT_DOUBLE_EQ(r.height(), 9.0);
}

// --- Полиморфизм через базовый указатель ---------------------------------

// Вызов виртуального метода через Shape* должен попасть в переопределение
// производного класса — не в Shape::area()/perimeter().
TEST(Shapes, PolymorphicDispatchViaPointer) {
    std::unique_ptr<Shape> shapes[2];
    shapes[0] = std::make_unique<Circle>(1.0);
    shapes[1] = std::make_unique<Rectangle>(2.0, 3.0);

    EXPECT_NEAR(shapes[0]->area(), PI, 1e-12);
    EXPECT_DOUBLE_EQ(shapes[1]->area(), 6.0);
    EXPECT_EQ(shapes[0]->name(), "Circle");
    EXPECT_EQ(shapes[1]->name(), "Rectangle");
}

// Суммарная площадь вектора разнотипных фигур через базовую ссылку.
TEST(Shapes, TotalAreaPolymorphic) {
    std::vector<std::unique_ptr<Shape>> vec;
    vec.push_back(std::make_unique<Circle>(1.0));         // π
    vec.push_back(std::make_unique<Rectangle>(2.0, 3.0)); // 6
    vec.push_back(std::make_unique<Circle>(2.0));         // 4π

    double total = 0.0;
    for (const auto& s : vec) {
        total += s->area();
    }
    EXPECT_NEAR(total, 5.0 * PI + 6.0, 1e-10);
}

// --- Виртуальный деструктор — счётчик разрушений -------------------------
//
// Идея: производный класс DestructorTracker держит ссылку на счётчик и
// инкрементирует его в деструкторе. Если виртуальный деструктор Shape работает
// правильно, удаление через Shape* вызывает ~DestructorTracker() → счётчик
// растёт. Без виртуального деструктора ~DestructorTracker() не вызывался бы —
// счётчик остался бы нулём (UB в нашем случае проявился бы именно так).

namespace {
class DestructorTracker final : public Shape {
public:
    explicit DestructorTracker(int& counter) : counter_(counter) {}
    ~DestructorTracker() override { ++counter_; }

    double area()      const override { return 0.0; }
    double perimeter() const override { return 0.0; }
    std::string name() const override { return "Tracker"; }

private:
    int& counter_;
};
}  // namespace

TEST(Shapes, VirtualDestructorCalled) {
    int destroyed = 0;
    {
        // Создаём объект через базовый указатель — единственный способ,
        // при котором отсутствие virtual ~Shape() было бы UB.
        std::unique_ptr<Shape> p = std::make_unique<DestructorTracker>(destroyed);
        EXPECT_EQ(destroyed, 0);  // ещё жив
    }
    // Выход из области → деструктор производного класса ДОЛЖЕН был сработать.
    EXPECT_EQ(destroyed, 1);
}

// Несколько объектов в векторе — все разрушаются корректно.
TEST(Shapes, VirtualDestructorAllDestroyed) {
    int destroyed = 0;
    {
        std::vector<std::unique_ptr<Shape>> v;
        for (int i = 0; i < 5; ++i) {
            v.push_back(std::make_unique<DestructorTracker>(destroyed));
        }
        EXPECT_EQ(destroyed, 0);
    }
    EXPECT_EQ(destroyed, 5);
}

// --- Абстрактность Shape -------------------------------------------------

// Shape нельзя инстанциировать — он абстрактный.
TEST(Shapes, ShapeIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<Shape>);
    EXPECT_FALSE(std::is_abstract_v<Circle>);
    EXPECT_FALSE(std::is_abstract_v<Rectangle>);
}

// --- SEEDED property-тесты -----------------------------------------------

// Площадь Rectangle(a, b) == a * b для случайных a, b > 0.
TEST(ShapeProps, RectangleAreaEqualsProduct) {
    std::mt19937 rng(0xBEEFu);
    std::uniform_real_distribution<double> d(0.1, 500.0);
    for (int i = 0; i < 500; ++i) {
        double a = d(rng);
        double b = d(rng);
        Rectangle r(a, b);
        EXPECT_DOUBLE_EQ(r.area(), a * b);
    }
}

// Масштабирование Circle: area(k*r) == k² * area(r).
TEST(ShapeProps, CircleAreaScaling) {
    std::mt19937 rng(0xBEEFu);
    std::uniform_real_distribution<double> r_dist(0.1, 100.0);
    std::uniform_real_distribution<double> k_dist(0.5, 10.0);
    for (int i = 0; i < 500; ++i) {
        double r = r_dist(rng);
        double k = k_dist(rng);
        Circle c1(r);
        Circle c2(k * r);
        EXPECT_NEAR(c2.area(), k * k * c1.area(), 1e-9 * c1.area() * k * k);
    }
}

// Масштабирование Circle: perimeter(k*r) == k * perimeter(r).
TEST(ShapeProps, CirclePerimeterScaling) {
    std::mt19937 rng(0xBEEFu);
    std::uniform_real_distribution<double> r_dist(0.1, 100.0);
    std::uniform_real_distribution<double> k_dist(0.5, 10.0);
    for (int i = 0; i < 500; ++i) {
        double r = r_dist(rng);
        double k = k_dist(rng);
        Circle c1(r);
        Circle c2(k * r);
        EXPECT_NEAR(c2.perimeter(), k * c1.perimeter(), 1e-9 * c1.perimeter() * k);
    }
}

// Периметр Rectangle аддитивен по сторонам: p(a,b) = 2*(a+b).
TEST(ShapeProps, RectanglePerimeterAdditive) {
    std::mt19937 rng(0xBEEFu);
    std::uniform_real_distribution<double> d(0.1, 500.0);
    for (int i = 0; i < 500; ++i) {
        double a = d(rng);
        double b = d(rng);
        Rectangle r(a, b);
        EXPECT_DOUBLE_EQ(r.perimeter(), 2.0 * (a + b));
    }
}
