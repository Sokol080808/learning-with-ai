#include "task09.hpp"
// #include <stdexcept>
// #include <charconv>

int safe_divide(int a, int b) {
    // TODO: при b == 0 — throw std::invalid_argument(...)
    (void)a; (void)b;
    return 0;
}

int element_at(const std::vector<int>& v, int index) {
    // TODO: проверь границы; иначе throw std::out_of_range(...)
    (void)v; (void)index;
    return 0;
}

std::optional<int> to_int(const std::string& s) {
    // TODO: верни число или std::nullopt
    (void)s;
    return std::nullopt;
}

std::optional<int> first_even(const std::vector<int>& v) {
    // TODO
    (void)v;
    return std::nullopt;
}

std::string describe(const std::variant<int, std::string>& v) {
    // TODO: различи альтернативы
    (void)v;
    return "";
}
