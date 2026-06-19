#pragma once
#include <vector>
#include <map>
#include <string>
#include <utility>

// Пара {минимум, максимум} для непустого вектора.
std::pair<int, int> minmax_of(const std::vector<int>& v);

// Типобезопасное перечисление цветов.
enum class Color { Red, Green, Blue };

// Имя цвета: "Red" / "Green" / "Blue".
std::string color_name(Color c);

// Квадрат на этапе компиляции. РЕАЛИЗУЙ прямо здесь (constexpr-функции — в заголовке).
constexpr int square_ce(int n) {
    // TODO: верни n * n
    (void)n;
    return 0;
}

// Чётные элементы, возведённые в квадрат (через std::views).
std::vector<int> evens_squared(const std::vector<int>& v);

// Сумма всех значений словаря.
int sum_values(const std::map<std::string, int>& m);
