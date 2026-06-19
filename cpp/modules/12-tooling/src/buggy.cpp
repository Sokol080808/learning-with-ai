#include "buggy.hpp"

#include <stdexcept>   // std::out_of_range, std::logic_error
#include <cassert>     // assert

// ВНИМАНИЕ: первые пять функций — НЕ стабы, а РЕАЛИЗАЦИИ С БАГАМИ. Все компилируются, но
// ведут себя неправильно. Найди и почини каждую (см. README: тесты, санитайзер, gdb).
// Функции из заданий 6-8 (внизу файла) — стабы: их тела нужно написать самому.

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

// ===== Новые задания (посложнее) — это СТАБЫ, реализуй сам =====

// Задание 6. Безопасный доступ по индексу.
// Должен возвращать v[i], а при i >= v.size() бросать std::out_of_range.
int checked_at(const std::vector<int>& v, std::size_t i) {
    // TODO: проверь границу (i >= v.size()) и брось std::out_of_range, иначе верни v[i].
    (void)v;
    (void)i;
    throw std::logic_error("TODO: checked_at не реализован");
}

// Задание 7. Целочисленный квадратный корень: наибольшее r >= 0 c r*r <= n.
// Предусловие n >= 0 защищай через assert. Берегись переполнения r*r (считай в long long).
int int_sqrt(int n) {
    // TODO: assert(n >= 0 && "..."); найди наибольшее r с (long long)r*r <= n.
    (void)n;
    return -1;   // заведомо неверный ответ: int_sqrt никогда не отрицателен
}

// Задание 8. Округлённое вниз среднее двух int БЕЗ переполнения. Контракт: a <= b.
int midpoint_int(int a, int b) {
    // TODO: посчитай как a + (b - a) / 2, чтобы не было переполнения (см. Идею 2в).
    (void)a;
    (void)b;
    return 0;   // заведомо неверный ответ
}
