#include "shapes.hpp"

#include <numbers>   // std::numbers::pi (C++20)

// ──────────────────────────────────────────────────────────────────────────
// Circle
// ──────────────────────────────────────────────────────────────────────────

Circle::Circle(double r) : r_(r) {}

double Circle::radius() const { return r_; }

double Circle::area() const {
    return std::numbers::pi * r_ * r_;
}

double Circle::perimeter() const {
    return 2.0 * std::numbers::pi * r_;
}

std::string Circle::name() const {
    return "Circle";
}

// ──────────────────────────────────────────────────────────────────────────
// Rectangle
// ──────────────────────────────────────────────────────────────────────────

Rectangle::Rectangle(double w, double h) : w_(w), h_(h) {}

double Rectangle::width()  const { return w_; }
double Rectangle::height() const { return h_; }

double Rectangle::area() const {
    return w_ * h_;
}

double Rectangle::perimeter() const {
    return 2.0 * (w_ + h_);
}

std::string Rectangle::name() const {
    return "Rectangle";
}
