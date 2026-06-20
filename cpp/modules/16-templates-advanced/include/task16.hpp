#pragma once
//
// Модуль 16 — Продвинутые шаблоны.
// Header-only: всё пишется прямо здесь, в шаблонах.
//
#include <string>
#include <type_traits>
#include <stdexcept>
#include <vector>
#include <list>
#include <iterator>
#include <cstddef>

namespace adv {

// =====================================================================
// Задание 1. Variadic + fold-выражения.
//   sum(args...)          — сумма любого числа аргументов одного типа.
//   to_string_all(args...)— склейка строковых представлений всех
//                           аргументов через ", " (см. README/тесты).
// =====================================================================

template <class First, class... Rest>
auto sum(First first, Rest... rest) {
    // Reference answer key: унарная правая свёртка по сложению.
    return (first + ... + rest);
}

template <class... Ts>
std::string to_string_all(const Ts&... args) {
    // Reference answer key: склейка std::to_string каждого аргумента через ", ".
    std::string out;
    bool first = true;
    (((out += (first ? "" : ", "), out += std::to_string(args), first = false)), ...);
    return out;
}

// =====================================================================
// Задание 2. Свои type traits.
//   my_remove_const<T>::type  / my_remove_const_t<T>
//   my_is_pointer<T>::value   / my_is_pointer_v<T>
//   my_is_same<A, B>::value   / my_is_same_v<A, B>
// Реализуй через частичные специализации (БЕЗ <type_traits>-аналогов).
// =====================================================================

// --- my_remove_const ---
template <class T>
struct my_remove_const {
    // Reference answer key: по умолчанию type совпадает с T.
    using type = T;
};
template <class T>
struct my_remove_const<const T> {
    // Снимаем const с верхнего уровня.
    using type = T;
};

template <class T>
using my_remove_const_t = typename my_remove_const<T>::type;

// --- my_is_pointer ---
template <class T>
struct my_is_pointer {
    // Reference answer key: по умолчанию не указатель.
    static constexpr bool value = false;
};
template <class T>
struct my_is_pointer<T*> {
    static constexpr bool value = true;
};

template <class T>
inline constexpr bool my_is_pointer_v = my_is_pointer<T>::value;

// --- my_is_same ---
template <class A, class B>
struct my_is_same {
    // Reference answer key: разные типы -> false.
    static constexpr bool value = false;
};
template <class T>
struct my_is_same<T, T> {
    static constexpr bool value = true;
};

template <class A, class B>
inline constexpr bool my_is_same_v = my_is_same<A, B>::value;

// =====================================================================
// Задание 3. enable_if-перегрузка describe().
//   Для целочисленных типов  -> "integer"
//   Для типов с плавающей точкой -> "floating"
// Должны существовать ДВЕ перегрузки, выбираемые через std::enable_if.
// =====================================================================

template <class T,
          typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
std::string describe(T /*value*/) {
    // Reference answer key.
    return "integer";
}

template <class T,
          typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
std::string describe(T /*value*/) {
    // Reference answer key.
    return "floating";
}

// =====================================================================
// Задание 4. CRTP-миксин Comparable<Derived>.
//   Производный класс предоставляет operator< и operator==.
//   Миксин ДАЁТ ему: !=, >, <=, >=.
// =====================================================================

template <class Derived>
struct Comparable {
    // Удобный доступ к производному объекту.
    const Derived& self() const { return static_cast<const Derived&>(*this); }

    friend bool operator!=(const Derived& a, const Derived& b) {
        // Reference answer key: a != b  <=>  !(a == b).
        return !(a == b);
    }
    friend bool operator>(const Derived& a, const Derived& b) {
        // Reference answer key: a > b  <=>  b < a.
        return b < a;
    }
    friend bool operator<=(const Derived& a, const Derived& b) {
        // Reference answer key: a <= b  <=>  !(b < a).
        return !(b < a);
    }
    friend bool operator>=(const Derived& a, const Derived& b) {
        // Reference answer key: a >= b  <=>  !(a < b).
        return !(a < b);
    }
};

// =====================================================================
// Задание 5. Tag dispatch.
//   distance_dispatch(begin, end) — число шагов от begin до end.
//   Для итераторов случайного доступа — быстрый путь (end - begin).
//   Для остальных — обычный путь (считаем шаги в цикле).
//   Реализуй ДВЕ внутренние перегрузки, выбираемые по категории тега,
//   плюс счётчик быстрых вызовов (см. тесты).
// =====================================================================

// Счётчик: сколько раз отработал быстрый путь (для проверки в тестах).
inline int& fast_path_calls() {
    static int n = 0;
    return n;
}

// Перегрузка для произвольного итератора (обычный путь).
template <class It>
std::ptrdiff_t distance_impl(It begin, It end,
                             std::input_iterator_tag) {
    // Reference answer key: считаем шаги в цикле.
    std::ptrdiff_t n = 0;
    for (; begin != end; ++begin) {
        ++n;
    }
    return n;
}

// Перегрузка для итератора случайного доступа (быстрый путь).
template <class It>
std::ptrdiff_t distance_impl(It begin, It end,
                             std::random_access_iterator_tag) {
    // Reference answer key: быстрый путь.
    ++fast_path_calls();
    return end - begin;
}

template <class It>
std::ptrdiff_t distance_dispatch(It begin, It end) {
    // Reference answer key: диспетчеризация по тегу категории итератора.
    return distance_impl(
        begin, end,
        typename std::iterator_traits<It>::iterator_category{});
}

// =====================================================================
// Задание 6. all_of fold.
//   all_of_pred(p, xs...) -> bool
//   Возвращает true, если предикат p истинен для ВСЕХ аргументов.
//   Пустой пакет -> true (vacuous truth).
//   Реализация: fold-выражение по &&.
// =====================================================================

template <class Pred, class... Ts>
bool all_of_pred(Pred p, Ts... xs) {
    // Правая свёртка по &&: p(x0) && p(x1) && ... && p(xN).
    // Пустой пакет: унарный && даёт true (нейтральный элемент конъюнкции).
    return (p(xs) && ...);
}

}  // namespace adv
