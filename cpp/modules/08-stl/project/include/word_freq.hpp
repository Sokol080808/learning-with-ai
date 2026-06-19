#pragma once
#include <map>
#include <string>
#include <vector>
#include <utility>

// Подсчёт слов: разбить text по пробельным символам и посчитать каждое слово.
// Регистр не приводится ("A" и "a" — разные слова).
std::map<std::string, int> word_count(const std::string& text);

// n самых частых слов: сортировка по убыванию частоты, при равенстве — по алфавиту.
std::vector<std::pair<std::string, int>> top_n(
    const std::map<std::string, int>& freq, int n);
