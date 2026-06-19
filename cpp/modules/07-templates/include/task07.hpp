#pragma once
#include <vector>
#include <concepts>
#include <cstddef>
#include <array>
#include <type_traits>
#include <stdexcept>
#include <string>

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
        // TODO: проверить границы (иначе std::out_of_range) и вернуть ссылку на нужный элемент
        (void)r; (void)c;
        throw std::logic_error("TODO: Matrix::at");
    }
    const T& at(std::size_t r, std::size_t c) const {
        // TODO: то же самое, но const-версия
        (void)r; (void)c;
        throw std::logic_error("TODO: Matrix::at const");
    }

    // Поэлементное сложение. Тип гарантирует, что размеры совпадают.
    Matrix operator+(const Matrix& other) const {
        // TODO: сложить поэлементно и вернуть результат
        (void)other;
        throw std::logic_error("TODO: Matrix::operator+");
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
    // TODO: вернуть "other"
    return "stub";
}
// Полная специализация для bool:
template <>
inline const char* type_name<bool>(const bool&) {
    // TODO: вернуть "bool"
    return "stub";
}
// Частичная специализация невозможна для функций — поэтому для указателей
// делаем ОТДЕЛЬНУЮ перегрузку шаблона (это работает за счёт перегрузки, а
// перегрузка для указателей более специализирована, чем базовый шаблон):
template <class T>
const char* type_name(T* const&) {
    // TODO: вернуть "pointer"
    return "stub";
}

// 8. compile-time возведение в степень через рекурсию шаблонов.
//    Pow<Base, Exp>::value должно вычисляться ЦЕЛИКОМ на этапе компиляции
//    (годится в static_assert и как размер массива). Base^0 == 1.
template <long long Base, unsigned Exp>
struct Pow {
    // TODO: связать с Pow<Base, Exp - 1>::value так, чтобы получилось Base^Exp
    static constexpr long long value = 0;
};
// Базовый случай рекурсии — частичная специализация по Exp == 0:
template <long long Base>
struct Pow<Base, 0> {
    // TODO: чему равно Base^0 ?
    static constexpr long long value = 0;
};

// Удобный псевдоним-переменная (variable template).
template <long long Base, unsigned Exp>
inline constexpr long long pow_v = Pow<Base, Exp>::value;
