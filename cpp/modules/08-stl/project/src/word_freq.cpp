#include "word_freq.hpp"
// #include <sstream>   // std::istringstream
// #include <algorithm>

std::map<std::string, int> word_count(const std::string& text) {
    // TODO: разбей text на слова (istringstream + >>) и посчитай частоты.
    (void)text;
    return {};
}

std::vector<std::pair<std::string, int>> top_n(
    const std::map<std::string, int>& freq, int n) {
    // TODO: перенеси пары в vector, отсортируй компаратором
    // (частота по убыванию, при равенстве слово по возрастанию), верни первые n.
    (void)freq; (void)n;
    return {};
}
