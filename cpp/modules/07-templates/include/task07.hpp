#pragma once
#include <vector>
#include <concepts>
#include <cstddef>
#include <array>
#include <type_traits>
#include <stdexcept>
#include <string>

// Эталонный ответ (answer key) — реализация шаблонов модуля 07.
// Весь код шаблонов живёт в заголовке (см. README, идея 2).

// 1. Больший из двух.
template <class T>
T my_max(const T& a, const T& b) {
    return (a < b) ? b : a;
}

// 2. Зажать v в диапазон [lo, hi].
template <class T>
T clamp_value(const T& v, const T& lo, const T& hi) {
    if (v < lo) return lo;
    if (hi < v) return hi;
    return v;
}

// 3. Сумма элементов вектора (для пустого — T{}).
template <class T>
T sum_vector(const std::vector<T>& v) {
    T total{};
    for (const T& x : v) total += x;
    return total;
}

// 4. Шаблон пары с методом swapped().
template <class First, class Second>
struct Pair {
    First  first;
    Second second;

    Pair(First f, Second s) : first(f), second(s) {}

    Pair<Second, First> swapped() const {
        return Pair<Second, First>(second, first);
    }
};

// 5. Является ли x положительной степенью двойки. Ограничено целочисленными типами.
template <std::integral T>
bool is_power_of_two(T x) {
    return x > 0 && (x & (x - 1)) == 0;
}

// ============================================================================
//  Продвинутые задания (нетиповые параметры, специализация, compile-time).
// ============================================================================

// 6. Matrix<T, Rows, Cols> — матрица фиксированного размера на нетиповых
//    параметрах шаблона. Размер зашит в ТИП: Matrix<int,2,3> и Matrix<int,3,2> —
//    это РАЗНЫЕ типы, и сложить их компилятор не даст. Данные храним
//    построчно в одном std::array.
template <class T, std::size_t Rows, std::size_t Cols>
struct Matrix {
    static_assert(Rows > 0 && Cols > 0, "Matrix не может иметь нулевую размерность");

    std::array<T, Rows * Cols> data{};  // row-major: элемент (r,c) лежит на data[r*Cols + c]

    static constexpr std::size_t rows() { return Rows; }
    static constexpr std::size_t cols() { return Cols; }

    // Доступ к элементу (r, c). Должен бросать std::out_of_range, если индекс вне границ.
    T& at(std::size_t r, std::size_t c) {
        if (r >= Rows || c >= Cols)
            throw std::out_of_range("Matrix::at: index out of range");
        return data[r * Cols + c];
    }
    const T& at(std::size_t r, std::size_t c) const {
        if (r >= Rows || c >= Cols)
            throw std::out_of_range("Matrix::at: index out of range");
        return data[r * Cols + c];
    }

    // Поэлементное сложение. Тип гарантирует, что размеры совпадают.
    Matrix operator+(const Matrix& other) const {
        Matrix result;
        for (std::size_t i = 0; i < Rows * Cols; ++i)
            result.data[i] = data[i] + other.data[i];
        return result;
    }

    bool operator==(const Matrix& other) const {
        return data == other.data;
    }
};

// 7. type_name(value) — вернуть короткое имя категории типа аргумента через
//    специализацию шаблона функции. База — "other"; нужны специализации
//    для bool и для УКАЗАТЕЛЕЙ (любого T*).
//
// База (первичный шаблон):
template <class T>
const char* type_name(const T&) {
    return "other";
}
// Полная специализация для bool:
template <>
inline const char* type_name<bool>(const bool&) {
    return "bool";
}
// Частичная специализация невозможна для функций — поэтому для указателей
// делаем ОТДЕЛЬНУЮ перегрузку шаблона (это работает за счёт перегрузки, а
// перегрузка для указателей более специализирована, чем базовый шаблон):
template <class T>
const char* type_name(T* const&) {
    return "pointer";
}

// 8. compile-time возведение в степень через рекурсию шаблонов.
//    Pow<Base, Exp>::value должно вычисляться ЦЕЛИКОМ на этапе компиляции
//    (годится в static_assert и как размер массива). Base^0 == 1.
template <long long Base, unsigned Exp>
struct Pow {
    static constexpr long long value = Base * Pow<Base, Exp - 1>::value;
};
// Базовый случай рекурсии — частичная специализация по Exp == 0:
template <long long Base>
struct Pow<Base, 0> {
    static constexpr long long value = 1;
};

// Удобный псевдоним-переменная (variable template).
template <long long Base, unsigned Exp>
inline constexpr long long pow_v = Pow<Base, Exp>::value;
