#include "fraction.hpp"

#include <ostream>
#include <chrono>

// Реализуй методы класса Fraction. Сейчас это стабы.
//
// Подсказка по структуре: всю нормализацию (сокращение + знак) удобно сделать
// в конструкторе. Тогда add/multiply просто создают новый Fraction.

Fraction::Fraction(int num, int den)
    : num_(num), den_(den) {
    // TODO: нормализуй num_/den_ (сократи на НОД, перенеси знак в числитель).
}

int Fraction::numerator() const {
    // TODO
    return 0;
}

int Fraction::denominator() const {
    // TODO
    return 1;
}

Fraction Fraction::add(const Fraction& o) const {
    // TODO: a/b + c/d
    (void)o;
    return Fraction(0, 1);
}

Fraction Fraction::multiply(const Fraction& o) const {
    // TODO
    (void)o;
    return Fraction(0, 1);
}

bool Fraction::operator==(const Fraction& o) const {
    // TODO
    (void)o;
    return false;
}

std::string Fraction::to_string() const {
    // TODO: "num/den"
    return "";
}

// ── Новые перегрузки операторов (Задание 2) ───────────────────────────────

Fraction Fraction::operator+(const Fraction& o) const {
    // TODO: верни сумму. Подумай, нельзя ли переиспользовать уже написанный add().
    (void)o;
    return Fraction(0, 1);
}

Fraction Fraction::operator*(const Fraction& o) const {
    // TODO: верни произведение (можно через multiply()).
    (void)o;
    return Fraction(0, 1);
}

bool Fraction::operator<(const Fraction& o) const {
    // TODO: a/b < c/d. Сведи к сравнению целых, не теряя знак знаменателя.
    (void)o;
    return false;
}

std::ostream& operator<<(std::ostream& os, const Fraction& f) {
    // TODO: выведи f в виде "num/den". Свободная функция — у неё есть доступ
    // только к публичному интерфейсу Fraction (numerator()/denominator()/to_string()).
    (void)f;
    return os;
}

// ── RAII-таймер (Задание 3) ───────────────────────────────────────────────

Timer::Timer(std::int64_t& accumulator_ns)
    : accumulator_ns_(&accumulator_ns), start_ns_(0) {
    // TODO: запиши в start_ns_ текущий момент в наносекундах.
    // Подсказка: std::chrono::steady_clock::now().time_since_epoch(),
    // затем std::chrono::duration_cast<std::chrono::nanoseconds>(...).count().
}

Timer::~Timer() {
    // TODO: вычисли (текущее время - start_ns_) в наносекундах и ПРИБАВЬ
    // результат к *accumulator_ns_. Стаб ничего не прибавляет — это неверно.
}

// ── Vec2 (Задание 4) ──────────────────────────────────────────────────────

Vec2 Vec2::operator+(const Vec2& o) const {
    // TODO: покомпонентная сумма.
    (void)o;
    return Vec2{0.0, 0.0};
}

Vec2 Vec2::operator-(const Vec2& o) const {
    // TODO: покомпонентная разность.
    (void)o;
    return Vec2{0.0, 0.0};
}

bool Vec2::operator==(const Vec2& o) const {
    // TODO: точное равенство x и y.
    (void)o;
    return false;
}

double Vec2::dot(const Vec2& o) const {
    // TODO: скалярное произведение.
    (void)o;
    return 0.0;
}
