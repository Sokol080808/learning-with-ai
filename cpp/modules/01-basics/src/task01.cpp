#include "task01.hpp"

// Реализуй функции ниже. Сейчас это стабы-заглушки — тесты красные.

double to_fahrenheit(double c) {
    // TODO: F = C * 9/5 + 32 (следи за дробным делением)
    (void)c;
    return 0.0;
}

bool is_even(int n) {
    // TODO
    (void)n;
    return false;
}

int min_of_three(int a, int b, int c) {
    // TODO
    (void)a; (void)b; (void)c;
    return 0;
}

void triple(int& x) {
    // TODO: измени x на месте
    (void)x;
}

double average3(int a, int b, int c) {
    // TODO: верни дробное среднее
    (void)a; (void)b; (void)c;
    return 0.0;
}

std::optional<int> safe_add(int a, int b) {
    // TODO: проверь переполнение ДО сложения (см. <limits>: INT_MAX / INT_MIN).
    // Если переполнение — верни std::nullopt, иначе std::optional с суммой.
    (void)a; (void)b;
    return std::nullopt;
}

unsigned int set_bit(unsigned int value, int pos) {
    // TODO: установи бит pos в 1 (подумай про 1u << pos и |).
    (void)pos;
    return value;
}

unsigned int clear_bit(unsigned int value, int pos) {
    // TODO: сбрось бит pos в 0 (маска и ~).
    (void)pos;
    return value;
}

unsigned int toggle_bit(unsigned int value, int pos) {
    // TODO: инвертируй бит pos (^).
    (void)pos;
    return value;
}

bool get_bit(unsigned int value, int pos) {
    // TODO: верни true, если бит pos равен 1.
    (void)value; (void)pos;
    return false;
}

void swap_ints(int& a, int& b) {
    // TODO: поменяй значения a и b местами (без std::swap).
    (void)a; (void)b;
}
