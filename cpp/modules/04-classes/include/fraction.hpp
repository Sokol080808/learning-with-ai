#pragma once
#include <string>

// Рациональная дробь. Инварианты (всегда истинны для корректного объекта):
//   * хранится в сокращённом виде (числитель и знаменатель взаимно просты);
//   * знаменатель строго положителен, знак — в числителе;
//   * ноль представлен как 0/1.
class Fraction {
public:
    // Создаёт дробь num/den и сразу нормализует её. den != 0 (гарантируется тестами).
    Fraction(int num, int den);

    int numerator() const;
    int denominator() const;

    Fraction add(const Fraction& o) const;
    Fraction multiply(const Fraction& o) const;

    bool operator==(const Fraction& o) const;

    std::string to_string() const;  // вид "num/den"

private:
    int num_;
    int den_;
};
