// Эталонный ответ (answer key) — реализация заданий модуля 10.
#include "task10.hpp"
#include <stdexcept>
#include <ranges>
#include <algorithm>

std::pair<int, int> minmax_of(const std::vector<int>& v) {
    auto [lo, hi] = std::minmax_element(v.begin(), v.end());
    return {*lo, *hi};
}

std::string color_name(Color c) {
    switch (c) {
        case Color::Red:   return "Red";
        case Color::Green: return "Green";
        case Color::Blue:  return "Blue";
    }
    return "";
}

std::vector<int> evens_squared(const std::vector<int>& v) {
    namespace rv = std::views;
    auto pipeline = v
        | rv::filter([](int x) { return x % 2 == 0; })
        | rv::transform([](int x) { return x * x; });
    std::vector<int> out;
    for (int x : pipeline) {
        out.push_back(x);
    }
    return out;
}

int sum_values(const std::map<std::string, int>& m) {
    int total = 0;
    for (const auto& [k, val] : m) {
        (void)k;
        total += val;
    }
    return total;
}

std::vector<std::string_view> split_view(std::string_view s, char sep) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (true) {
        std::size_t p = s.find(sep, start);
        if (p == std::string_view::npos) {
            parts.push_back(s.substr(start));
            break;
        }
        parts.push_back(s.substr(start, p - start));
        start = p + 1;
    }
    return parts;
}

std::pair<int, int> divmod(int a, int b) {
    if (b == 0) {
        throw std::invalid_argument("divmod: деление на ноль");
    }
    return {a / b, a % b};
}

std::optional<int> safe_divide(int a, int b) {
    if (b == 0) {
        return std::nullopt;
    }
    return a / b;
}
