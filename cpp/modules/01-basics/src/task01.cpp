#include "task01.hpp"

#include <limits>

// Эталонный ключ ответов (reference answer key) — не поставляется учащимся.

double to_fahrenheit(double c) {
    // Приводим коэффициенты к дробным, чтобы деление было дробным, а не целочисленным.
    return c * 9.0 / 5.0 + 32.0;
}

bool is_even(int n) {
    return n % 2 == 0;
}

int min_of_three(int a, int b, int c) {
    int m = a;
    if (b < m) m = b;
    if (c < m) m = c;
    return m;
}

void triple(int& x) {
    x *= 3;
}

double average3(int a, int b, int c) {
    // Приводим к double ДО деления, иначе сумма int делилась бы нацело.
    return static_cast<double>(a + b + c) / 3.0;
}

std::optional<int> safe_add(int a, int b) {
    constexpr int kMax = std::numeric_limits<int>::max();
    constexpr int kMin = std::numeric_limits<int>::min();

    // Проверяем переполнение ДО сложения: знаковое переполнение int — это UB.
    if (b > 0 && a > kMax - b) return std::nullopt;  // переполнение вверх
    if (b < 0 && a < kMin - b) return std::nullopt;  // переполнение вниз
    return a + b;
}

unsigned int set_bit(unsigned int value, int pos) {
    return value | (1u << pos);
}

unsigned int clear_bit(unsigned int value, int pos) {
    return value & ~(1u << pos);
}

unsigned int toggle_bit(unsigned int value, int pos) {
    return value ^ (1u << pos);
}

bool get_bit(unsigned int value, int pos) {
    return (value >> pos) & 1u;
}

void swap_ints(int& a, int& b) {
    // Через временную переменную — корректно работает и когда a и b — одна переменная.
    int tmp = a;
    a = b;
    b = tmp;
}
