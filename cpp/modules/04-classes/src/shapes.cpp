#include "shapes.hpp"

#include <numbers>   // std::numbers::pi (C++20)

// ──────────────────────────────────────────────────────────────────────────
// Circle
// ──────────────────────────────────────────────────────────────────────────

Circle::Circle(double r) : r_(r) {
    // TODO: конструктор может также проверять r > 0 (опционально).
}

double Circle::radius() const {
    // TODO: верни радиус.
    return 0.0;
}

double Circle::area() const {
    // TODO: π·r². Подсказка: std::numbers::pi (C++20) или M_PI.
    return 0.0;
}

double Circle::perimeter() const {
    // TODO: 2·π·r.
    return 0.0;
}

std::string Circle::name() const {
    // TODO: верни "Circle".
    return "";
}

// ──────────────────────────────────────────────────────────────────────────
// Rectangle
// ──────────────────────────────────────────────────────────────────────────

Rectangle::Rectangle(double w, double h) : w_(w), h_(h) {
    // TODO: конструктор может проверять w > 0 и h > 0 (опционально).
}

double Rectangle::width() const {
    // TODO: верни ширину.
    return 0.0;
}

double Rectangle::height() const {
    // TODO: верни высоту.
    return 0.0;
}

double Rectangle::area() const {
    // TODO: w·h.
    return 0.0;
}

double Rectangle::perimeter() const {
    // TODO: 2·(w+h).
    return 0.0;
}

std::string Rectangle::name() const {
    // TODO: верни "Rectangle".
    return "";
}
