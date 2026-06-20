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
auto sum(First first, Rest... /*rest*/) {
    // TODO: верни сумму ВСЕХ аргументов через fold-выражение.
    // (заглушка возвращает только первый — тесты на этом и упадут)
    return first;
}

template <class... Ts>
std::string to_string_all(const Ts&... /*args*/) {
    // TODO: для каждого аргумента вызови std::to_string и склей через ", ".
    return std::string{"TODO"};
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
    // TODO: по умолчанию type должен совпадать с T...
    using type = const T;  // ...а это нарочно неверная заглушка.
};
// TODO: добавь частичную специализацию для `const T`, снимающую const.

template <class T>
using my_remove_const_t = typename my_remove_const<T>::type;

// --- my_is_pointer ---
template <class T>
struct my_is_pointer {
    // TODO: по умолчанию false, специализация для T* — true.
    static constexpr bool value = true;  // нарочно неверно
};
// TODO: добавь специализацию my_is_pointer<T*>.

template <class T>
inline constexpr bool my_is_pointer_v = my_is_pointer<T>::value;

// --- my_is_same ---
template <class A, class B>
struct my_is_same {
    // TODO: по умолчанию false, специализация для <T, T> — true.
    static constexpr bool value = true;  // нарочно неверно
};
// TODO: добавь специализацию my_is_same<T, T>.

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
    // TODO: верни "integer".
    throw std::logic_error("TODO: describe(integral)");
}

template <class T,
          typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
std::string describe(T /*value*/) {
    // TODO: верни "floating".
    throw std::logic_error("TODO: describe(floating)");
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

    friend bool operator!=(const Derived& /*a*/, const Derived& /*b*/) {
        // TODO: через == производного класса.
        throw std::logic_error("TODO: operator!=");
    }
    friend bool operator>(const Derived& /*a*/, const Derived& /*b*/) {
        // TODO: a > b  <=>  b < a.
        throw std::logic_error("TODO: operator>");
    }
    friend bool operator<=(const Derived& /*a*/, const Derived& /*b*/) {
        // TODO: a <= b  <=>  !(b < a).
        throw std::logic_error("TODO: operator<=");
    }
    friend bool operator>=(const Derived& /*a*/, const Derived& /*b*/) {
        // TODO: a >= b  <=>  !(a < b).
        throw std::logic_error("TODO: operator>=");
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
std::ptrdiff_t distance_impl(It /*begin*/, It /*end*/,
                             std::input_iterator_tag) {
    // TODO: посчитай шаги в цикле.
    throw std::logic_error("TODO: distance_impl(input)");
}

// Перегрузка для итератора случайного доступа (быстрый путь).
template <class It>
std::ptrdiff_t distance_impl(It /*begin*/, It /*end*/,
                             std::random_access_iterator_tag) {
    // TODO: ++fast_path_calls(); верни end - begin.
    throw std::logic_error("TODO: distance_impl(random)");
}

template <class It>
std::ptrdiff_t distance_dispatch(It begin, It end) {
    // TODO: вызови distance_impl, передав тег категории итератора.
    (void)begin;
    (void)end;
    throw std::logic_error("TODO: distance_dispatch");
}

// =====================================================================
// Задание 6. all_of_pred — fold по &&.
//   all_of_pred(pred, xs...) -> bool
//   Возвращает true, если предикат pred истинен для ВСЕХ аргументов xs.
//   Пустой пакет -> true (vacuous truth).
//   Реализация: fold-выражение по &&.
// =====================================================================

template <class Pred, class... Ts>
bool all_of_pred(Pred /*p*/, Ts... /*xs*/) {
    // TODO: реализуй через fold-выражение (p(xs) && ...).
    // Подсказка: унарный && над пустым пакетом = true, над непустым = p(x0) && p(x1) && ...
    return false;  // нарочно неверная заглушка
}

}  // namespace adv
