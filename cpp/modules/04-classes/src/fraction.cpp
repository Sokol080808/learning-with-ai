#include "fraction.hpp"

#include <ostream>
#include <chrono>
#include <numeric>
#include <string>

// Эталонный ответ (answer key). На ветке для учащихся здесь стабы.
//
// Вся нормализация (сокращение + знак) выполняется в конструкторе.
// Тогда add/multiply/operator+ просто конструируют новый Fraction,
// и инварианты класса поддерживаются автоматически.

Fraction::Fraction(int num, int den)
    : num_(num), den_(den) {
    // Знаменатель строго положителен — знак переносим в числитель.
    if (den_ < 0) {
        num_ = -num_;
        den_ = -den_;
    }
    // Сокращаем на НОД. std::gcd возвращает неотрицательное значение.
    int g = std::gcd(num_, den_);
    if (g != 0) {
        num_ /= g;
        den_ /= g;
    }
    // Ноль представляем как 0/1.
    if (num_ == 0) {
        den_ = 1;
    }
}

int Fraction::numerator() const {
    return num_;
}

int Fraction::denominator() const {
    return den_;
}

Fraction Fraction::add(const Fraction& o) const {
    // a/b + c/d = (a*d + c*b) / (b*d). Конструктор нормализует результат.
    return Fraction(num_ * o.den_ + o.num_ * den_, den_ * o.den_);
}

Fraction Fraction::multiply(const Fraction& o) const {
    return Fraction(num_ * o.num_, den_ * o.den_);
}

bool Fraction::operator==(const Fraction& o) const {
    // Оба операнда хранятся в сокращённом каноническом виде, поэтому
    // достаточно сравнить компоненты напрямую.
    return num_ == o.num_ && den_ == o.den_;
}

std::string Fraction::to_string() const {
    return std::to_string(num_) + "/" + std::to_string(den_);
}

// ── Перегрузки операторов (Задание 2) ─────────────────────────────────────

Fraction Fraction::operator+(const Fraction& o) const {
    return add(o);
}

Fraction Fraction::operator*(const Fraction& o) const {
    return multiply(o);
}

bool Fraction::operator<(const Fraction& o) const {
    // a/b < c/d. Знаменатели положительны (инвариант), значит при умножении
    // на (den_ * o.den_) > 0 знак неравенства сохраняется.
    return num_ * o.den_ < o.num_ * den_;
}

std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    return os << f.to_string();
}

// ── RAII-таймер (Задание 3) ───────────────────────────────────────────────

namespace {
std::int64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
}  // namespace

Timer::Timer(std::int64_t& accumulator_ns)
    : accumulator_ns_(&accumulator_ns), start_ns_(now_ns()) {}

Timer::~Timer() {
    *accumulator_ns_ += now_ns() - start_ns_;
}

// ── Vec2 (Задание 4) ──────────────────────────────────────────────────────

Vec2 Vec2::operator+(const Vec2& o) const {
    return Vec2{x + o.x, y + o.y};
}

Vec2 Vec2::operator-(const Vec2& o) const {
    return Vec2{x - o.x, y - o.y};
}

bool Vec2::operator==(const Vec2& o) const {
    return x == o.x && y == o.y;
}

double Vec2::dot(const Vec2& o) const {
    return x * o.x + y * o.y;
}
