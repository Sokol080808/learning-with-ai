#include "fraction.hpp"

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
