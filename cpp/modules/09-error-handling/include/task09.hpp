#pragma once
#include <optional>
#include <variant>
#include <string>
#include <vector>

// Делит a на b; при b == 0 бросает std::invalid_argument.
int safe_divide(int a, int b);

// Возвращает v[index]; при выходе за границы бросает std::out_of_range.
int element_at(const std::vector<int>& v, int index);

// Парсит целое число; при некорректной строке возвращает std::nullopt.
std::optional<int> to_int(const std::string& s);

// Первый чётный элемент или std::nullopt.
std::optional<int> first_even(const std::vector<int>& v);

// "int: N" для int, "string: S" для строки.
std::string describe(const std::variant<int, std::string>& v);
