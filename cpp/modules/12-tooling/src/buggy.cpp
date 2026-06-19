#include "buggy.hpp"

// ВНИМАНИЕ: здесь НЕ стабы, а РЕАЛИЗАЦИИ С БАГАМИ. Все компилируются, но ведут себя
// неправильно. Найди и почини каждую (см. README: тесты, санитайзер, gdb).

int sum_first_n(const std::vector<int>& v, int n) {
    int total = 0;
    for (int i = 0; i <= n; ++i) {   // bug #1: где-то здесь граница не та...
        total += v[i];
    }
    return total;
}

std::string repeat(const std::string& s, int times) {
    std::string result = s;          // bug #2: подумай, со скольких копий мы стартуем
    for (int i = 0; i < times; ++i) {
        result += s;
    }
    return result;
}

int max_in(const std::vector<int>& v) {
    int best = 0;                    // bug #3: хорошее ли это начальное значение?
    for (int x : v) {
        if (x > best) best = x;
    }
    return best;
}

bool has_duplicate(std::vector<int> v) {
    for (std::size_t i = 0; i < v.size(); ++i) {
        for (std::size_t j = i; j < v.size(); ++j) {   // bug #4: с какого j сравнивать?
            if (v[i] == v[j]) return true;
        }
    }
    return false;
}

double average(const std::vector<int>& v) {
    int sum = 0;
    for (int x : v) sum += x;
    return sum / v.size();           // bug #5: какой тип у этого деления?
}
