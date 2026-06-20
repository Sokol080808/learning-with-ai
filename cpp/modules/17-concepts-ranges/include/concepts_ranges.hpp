#pragma once
#include <vector>
#include <cstddef>
#include <utility>
#include <iterator>
#include <concepts>
#include <ranges>
#include <algorithm>
#include <type_traits>

// =====================================================================
// Модуль 17 — Concepts и ranges (C++20)
// Это эталонный ключ ответов (reference branch) — образец корректной
// реализации, к учащимся не попадает.
// =====================================================================

namespace m17 {

// ---------------------------------------------------------------------
// Задание 1а. Концепт Addable<T>
//   Тип T удовлетворяет Addable, если выражение (a + b) корректно для
//   двух значений типа T И его результат конвертируется обратно в T.
//   Сейчас концепт «соврамши»: он истинен для ЛЮБОГО T. Почини его так,
//   чтобы он проверял наличие подходящего operator+.
// ---------------------------------------------------------------------
template <class T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
};

// ---------------------------------------------------------------------
// Задание 1б. Концепт Container<C>
//   Тип C — контейнер, если у него есть std::begin(c) и std::end(c),
//   причём по begin можно разыменоваться (*it) — то есть это диапазон,
//   по которому можно пройтись циклом range-for.
//   Сейчас концепт всегда ложен. Почини его.
// ---------------------------------------------------------------------
template <class C>
concept Container = requires(C c) {
    std::begin(c);
    std::end(c);
    *std::begin(c);
};

// ---------------------------------------------------------------------
// Задание 2. sum_container
//   Принимает ТОЛЬКО контейнер (Container) с элементами, которые можно
//   складывать (Addable). Возвращает сумму всех элементов; тип суммы —
//   это тип элемента контейнера. Для пустого контейнера верни Value{}.
//
//   Подпись фиксирована тестом: возвращаемый тип — это тип элемента,
//   выведенный из *begin(c). Здесь мы заранее называем его Value.
// ---------------------------------------------------------------------
template <Container C,
          class Value = std::remove_cvref_t<decltype(*std::begin(std::declval<C&>()))>>
    requires Addable<Value>
Value sum_container(const C& c) {
    Value total{};
    for (const auto& x : c) {
        total = total + x;
    }
    return total;
}

// ---------------------------------------------------------------------
// Задание 3. even_times_two_take
//   Ленивый конвейер через std::views: из вектора оставить ЧЁТНЫЕ числа,
//   каждое умножить на 2, взять первые k штук и материализовать в вектор.
//   Порядок шагов важен: фильтр чётности применяется к ИСХОДНЫМ числам,
//   потом удвоение, потом take(k).
//   Если k больше количества подходящих элементов — вернуть сколько есть.
// ---------------------------------------------------------------------
inline std::vector<int> even_times_two_take(const std::vector<int>& xs, std::size_t k) {
    auto pipeline = xs
        | std::views::filter([](int x) { return x % 2 == 0; })
        | std::views::transform([](int x) { return x * 2; })
        | std::views::take(k);
    std::vector<int> out;
    for (int x : pipeline) {
        out.push_back(x);
    }
    return out;
}

// ---------------------------------------------------------------------
// Задание 4. Концепт Sortable<R> и функция sorted_copy
//   Sortable<R> — диапазон со СЛУЧАЙНЫМ доступом, элементы которого
//   можно сравнивать через operator< (строгое слабое упорядочение).
//   sorted_copy(r) возвращает отсортированную по возрастанию КОПИЮ
//   (std::vector элементов r). Несортируемый тип должен давать ошибку
//   КОМПИЛЯЦИИ, а не падение в рантайме.
//   Сейчас концепт всегда ложен — почини его так, чтобы он требовал
//   random-access итератор и operator< у элементов.
// ---------------------------------------------------------------------
template <class R>
concept Sortable =
    std::ranges::random_access_range<R> &&
    requires(std::ranges::range_value_t<R> a, std::ranges::range_value_t<R> b) {
        { a < b } -> std::convertible_to<bool>;
    };

template <Sortable R,
          class Value = std::remove_cvref_t<decltype(*std::begin(std::declval<R&>()))>>
std::vector<Value> sorted_copy(const R& r) {
    std::vector<Value> out(std::begin(r), std::end(r));
    std::sort(out.begin(), out.end());
    return out;
}

// ---------------------------------------------------------------------
// Задание 5. take_n — свой простейший адаптер
//   Принимает диапазон range и число n, возвращает std::vector первых n
//   элементов (через итераторы begin/end). Если в диапазоне меньше n
//   элементов — вернуть все, не выходя за end().
//   Реализуй ВРУЧНУЮ через итераторы, без std::views::take.
// ---------------------------------------------------------------------
template <class R,
          class Value = std::remove_cvref_t<decltype(*std::begin(std::declval<R&>()))>>
std::vector<Value> take_n(const R& range, std::size_t n) {
    std::vector<Value> out;
    auto it = std::begin(range);
    auto last = std::end(range);
    for (std::size_t count = 0; it != last && count < n; ++it, ++count) {
        out.push_back(*it);
    }
    return out;
}

// ---------------------------------------------------------------------
// Задание 6. Проекции: sort_by_age и youngest_name
//   Демонстрируют std::ranges-алгоритмы с projection-аргументом на
//   структуре Person{name, age}.
//
//   sort_by_age(people) — принимает вектор по значению, сортирует на
//   месте по полю age (std::ranges::sort с &Person::age), возвращает.
//
//   youngest_name(people) — возвращает имя Person с минимальным age
//   (std::ranges::min_element с &Person::age). Для пустого вектора — "".
// ---------------------------------------------------------------------

struct Person {
    std::string name;
    int age = 0;
    friend bool operator==(const Person&, const Person&) = default;
};

inline std::vector<Person> sort_by_age(std::vector<Person> people) {
    std::ranges::sort(people, {}, &Person::age);
    return people;
}

inline std::string youngest_name(const std::vector<Person>& people) {
    if (people.empty()) return "";
    auto it = std::ranges::min_element(people, {}, &Person::age);
    return it->name;
}

}  // namespace m17
