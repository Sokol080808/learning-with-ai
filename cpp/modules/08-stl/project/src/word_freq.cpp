#include "word_freq.hpp"
// Эталонное решение (answer key) — не входит в материалы для учащихся.
#include <sstream>   // std::istringstream
#include <algorithm>

std::map<std::string, int> word_count(const std::string& text) {
    std::map<std::string, int> freq;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        ++freq[word];
    }
    return freq;
}

std::vector<std::pair<std::string, int>> top_n(
    const std::map<std::string, int>& freq, int n) {
    std::vector<std::pair<std::string, int>> pairs(freq.begin(), freq.end());
    std::sort(pairs.begin(), pairs.end(),
              [](const std::pair<std::string, int>& a,
                 const std::pair<std::string, int>& b) {
                  if (a.second != b.second) {
                      return a.second > b.second;  // частота по убыванию
                  }
                  return a.first < b.first;        // при равенстве — по алфавиту
              });
    if (n < 0) {
        n = 0;
    }
    if (static_cast<std::size_t>(n) < pairs.size()) {
        pairs.resize(static_cast<std::size_t>(n));
    }
    return pairs;
}
