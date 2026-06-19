#include "buggy.hpp"

#include <stdexcept>   // std::out_of_range, std::logic_error
#include <cassert>     // assert

// Это эталонный ответ (answer key): пять функций ниже починены, а задания 6-8 реализованы.

int sum_first_n(const std::vector<int>& v, int n) {
    int total = 0;
    for (int i = 0; i < n; ++i) {   // i < n: суммируем ровно первые n элементов, без выхода за границу
        total += v[i];
    }
    return total;
}

std::string repeat(const std::string& s, int times) {
    std::string result;              // стартуем с пустой строки: ровно times копий
    for (int i = 0; i < times; ++i) {
        result += s;
    }
    return result;
}

int max_in(const std::vector<int>& v) {
    int best = v[0];                 // инициализируем первым элементом, иначе ломается на всех отрицательных
    for (int x : v) {
        if (x > best) best = x;
    }
    return best;
}

bool has_duplicate(std::vector<int> v) {
    for (std::size_t i = 0; i < v.size(); ++i) {
        for (std::size_t j = i + 1; j < v.size(); ++j) {   // j = i+1: не сравниваем элемент сам с собой
            if (v[i] == v[j]) return true;
        }
    }
    return false;
}

double average(const std::vector<int>& v) {
    long long sum = 0;
    for (int x : v) sum += x;
    return static_cast<double>(sum) / v.size();   // дробное деление: приводим к double до деления
}

// ===== Новые задания (посложнее) — эталонные реализации =====

// Задание 6. Безопасный доступ по индексу.
// Возвращает v[i], а при i >= v.size() бросает std::out_of_range.
int checked_at(const std::vector<int>& v, std::size_t i) {
    if (i >= v.size()) {
        // i и v.size() — оба size_t, поэтому сравнение без -Wsign-compare.
        throw std::out_of_range("checked_at: index " + std::to_string(i) +
                                " out of range for size " + std::to_string(v.size()));
    }
    return v[i];
}

// Задание 7. Целочисленный квадратный корень: наибольшее r >= 0 c r*r <= n.
// Предусловие n >= 0 защищаем через assert. Промежуточное произведение — в long long,
// чтобы r*r не переполнило int у границы (46341^2 > INT_MAX).
int int_sqrt(int n) {
    assert(n >= 0 && "int_sqrt: n must be non-negative");
    int r = 0;
    while (static_cast<long long>(r + 1) * (r + 1) <= n) {
        ++r;
    }
    return r;
}

// Задание 8. Округлённое вниз (к меньшему концу) среднее двух int БЕЗ переполнения.
// Контракт: a <= b. Считаем как a + (b - a)/2, чтобы не складывать два больших числа.
int midpoint_int(int a, int b) {
    return a + (b - a) / 2;
}
