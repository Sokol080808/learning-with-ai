#include "task10.hpp"
#include <stdexcept>
#include <optional>
// #include <ranges>
// #include <algorithm>

std::pair<int, int> minmax_of(const std::vector<int>& v) {
    // TODO: вернуть {min, max}
    (void)v;
    return {0, 0};
}

std::string color_name(Color c) {
    // TODO
    (void)c;
    return "";
}

std::vector<int> evens_squared(const std::vector<int>& v) {
    // TODO: std::views::filter | std::views::transform, затем материализовать
    (void)v;
    return {};
}

int sum_values(const std::map<std::string, int>& m) {
    // TODO: for (const auto& [k, val] : m)
    (void)m;
    return 0;
}

std::vector<std::string_view> split_view(std::string_view s, char sep) {
    // TODO: пройди по s, находя позиции sep (s.find(sep, pos)); каждый кусок —
    // s.substr(start, len) (это снова string_view, без копии). Сохраняй пустые куски.
    (void)s;
    (void)sep;
    return {};
}

std::pair<int, int> divmod(int a, int b) {
    // TODO: при b == 0 -> throw std::invalid_argument("..."); иначе {a / b, a % b}.
    (void)a;
    (void)b;
    return {0, 0};
}

std::optional<int> safe_divide(int a, int b) {
    // TODO: если b == 0 — вернуть std::nullopt; иначе std::optional<int>{a / b}.
    (void)a;
    (void)b;
    return std::nullopt;  // заглушка: всегда nullopt (неверно для b != 0)
}
