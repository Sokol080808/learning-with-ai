#include "task09.hpp"

#include <charconv>

// Эталонный ключ к ответам (reference answer key).

int safe_divide(int a, int b) {
    if (b == 0)
        throw std::invalid_argument("division by zero");
    return a / b;
}

int element_at(const std::vector<int>& v, int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= v.size())
        throw std::out_of_range("element_at: index out of range");
    return v[static_cast<std::size_t>(index)];
}

std::optional<int> to_int(const std::string& s) {
    int value = 0;
    const char* begin = s.data();
    const char* end = s.data() + s.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    // Разбор удался И вся строка разобрана без «хвоста».
    if (ec == std::errc{} && ptr == end)
        return value;
    return std::nullopt;
}

std::optional<int> first_even(const std::vector<int>& v) {
    for (int x : v)
        if (x % 2 == 0)
            return x;
    return std::nullopt;
}

std::string describe(const std::variant<int, std::string>& v) {
    if (std::holds_alternative<int>(v))
        return "int: " + std::to_string(std::get<int>(v));
    return "string: " + std::get<std::string>(v);
}

std::optional<int> parse_int(const std::string& s) {
    if (s.empty())
        return std::nullopt;

    const char* begin = s.data();
    const char* end = s.data() + s.size();

    // from_chars сам понимает ведущий '-', но НЕ '+'. Ведущий '+' обрабатываем
    // вручную, сдвигая начало разбора на символ вперёд.
    if (s.front() == '+')
        ++begin;

    // Строка из одного только знака ("+" / "-") — не число.
    if (begin == end)
        return std::nullopt;

    int value = 0;
    auto [ptr, ec] = std::from_chars(begin, end, value);
    // Корректно (нет переполнения) И разобрана вся строка (нет хвоста/пробелов).
    if (ec == std::errc{} && ptr == end)
        return value;
    return std::nullopt;
}

int safe_div(int a, int b) {
    if (b == 0)
        throw DivideByZero();
    return a / b;
}
