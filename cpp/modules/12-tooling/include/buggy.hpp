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

// --- Новые задания (посложнее) ---

// Задание 6. Безопасный доступ по индексу: возвращает v[i], но при i >= v.size()
// бросает std::out_of_range (а не лезет за границу). Поведение как у std::vector::at.
int checked_at(const std::vector<int>& v, std::size_t i);

// Задание 7. Целочисленный квадратный корень: наибольшее r >= 0, такое что r*r <= n.
// Предусловие: n >= 0 (защищается через assert; отрицательный n — баг вызывающего кода).
int int_sqrt(int n);

// Задание 8. Округлённое вниз среднее двух int БЕЗ переполнения. Контракт: a <= b.
// Должно работать корректно даже у границы INT_MAX (наивное (a+b)/2 там переполняется).
int midpoint_int(int a, int b);
