#pragma once
#include <vector>
#include <string>

// Сумма первых n элементов вектора.
int sum_first_n(const std::vector<int>& v, int n);

// Строка s, повторённая times раз ("ab", 3 -> "ababab"). times >= 0.
std::string repeat(const std::string& s, int times);

// Максимум вектора (вектор непустой).
int max_in(const std::vector<int>& v);

// Есть ли в векторе хотя бы два одинаковых значения.
bool has_duplicate(std::vector<int> v);

// Среднее арифметическое (вектор непустой).
double average(const std::vector<int>& v);
