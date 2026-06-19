#pragma once
#include <vector>
#include <map>
#include <string>

// Только чётные элементы, в исходном порядке.
std::vector<int> evens(const std::vector<int>& v);

// Сколько элементов строго больше threshold.
int count_greater(const std::vector<int>& v, int threshold);

// Каждый элемент возведён в квадрат.
std::vector<int> squared(const std::vector<int>& v);

// Копия v, отсортированная по убыванию.
std::vector<int> sorted_desc(std::vector<int> v);

// Частота каждого символа строки.
std::map<char, int> char_frequency(const std::string& s);
