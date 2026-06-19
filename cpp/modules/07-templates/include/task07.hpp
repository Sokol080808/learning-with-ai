#pragma once
#include <vector>
#include <concepts>
#include <cstddef>

// ВЕСЬ код шаблонов пишется здесь, в заголовке (см. README, идея 2).
// Сейчас — стабы. Реализуй тела так, чтобы тесты модуля 07 стали зелёными.

// 1. Больший из двух.
template <class T>
T my_max(const T& a, const T& b) {
    // TODO
    (void)b;
    return a;
}

// 2. Зажать v в диапазон [lo, hi].
template <class T>
T clamp_value(const T& v, const T& lo, const T& hi) {
    // TODO
    (void)lo; (void)hi;
    return v;
}

// 3. Сумма элементов вектора (для пустого — T{}).
template <class T>
T sum_vector(const std::vector<T>& v) {
    // TODO
    (void)v;
    return T{};
}

// 4. Шаблон пары с методом swapped().
template <class First, class Second>
struct Pair {
    First  first;
    Second second;

    Pair(First f, Second s) : first(f), second(s) {}

    Pair<Second, First> swapped() const {
        // TODO: вернуть пару с переставленными значениями (и типами)
        return Pair<Second, First>(Second{}, First{});
    }
};

// 5. Является ли x положительной степенью двойки. Ограничено целочисленными типами.
template <std::integral T>
bool is_power_of_two(T x) {
    // TODO
    (void)x;
    return false;
}
