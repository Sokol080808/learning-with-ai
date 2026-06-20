#include "task08.hpp"
// Эталонное решение (answer key) — не входит в материалы для учащихся.
#include <algorithm>
#include <iterator>
#include <ranges>
#include <sstream>
#include <cctype>

std::vector<int> evens(const std::vector<int>& v) {
    std::vector<int> out;
    std::copy_if(v.begin(), v.end(), std::back_inserter(out),
                 [](int x) { return x % 2 == 0; });
    return out;
}

int count_greater(const std::vector<int>& v, int threshold) {
    return static_cast<int>(std::count_if(
        v.begin(), v.end(), [threshold](int x) { return x > threshold; }));
}

std::vector<int> squared(const std::vector<int>& v) {
    std::vector<int> out;
    out.reserve(v.size());
    std::transform(v.begin(), v.end(), std::back_inserter(out),
                   [](int x) { return x * x; });
    return out;
}

std::vector<int> sorted_desc(std::vector<int> v) {
    std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
    return v;
}

std::map<char, int> char_frequency(const std::string& s) {
    std::map<char, int> freq;
    for (char c : s) {
        ++freq[c];
    }
    return freq;
}

std::map<std::string, int> word_frequency(const std::string& text) {
    std::map<std::string, int> freq;
    std::istringstream iss(text);
    std::string word;
    while (iss >> word) {
        std::transform(word.begin(), word.end(), word.begin(), [](char c) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        });
        ++freq[word];
    }
    return freq;
}

std::vector<int> top_k(std::vector<int> v, int k) {
    auto desc = [](int a, int b) { return a > b; };
    if (k <= 0) {
        return {};
    }
    if (static_cast<std::size_t>(k) >= v.size()) {
        std::sort(v.begin(), v.end(), desc);
        return v;
    }
    std::partial_sort(v.begin(), v.begin() + k, v.end(), desc);
    v.resize(static_cast<std::size_t>(k));
    return v;
}

std::size_t drop_below(std::vector<int>& v, int threshold) {
    auto new_end = std::remove_if(v.begin(), v.end(),
                                  [threshold](int x) { return x < threshold; });
    std::size_t removed = static_cast<std::size_t>(std::distance(new_end, v.end()));
    v.erase(new_end, v.end());
    return removed;
}

std::vector<int> evens_squared(const std::vector<int>& xs) {
    // Ленивый конвейер: сначала отфильтровать чётные, затем возвести в квадрат.
    // views::filter и views::transform ничего не вычисляют сразу — элементы
    // обрабатываются поэлементно при материализации через ranges::copy.
    auto pipe = xs
        | std::views::filter([](int x) { return x % 2 == 0; })
        | std::views::transform([](int x) { return x * x; });

    std::vector<int> out;
    std::ranges::copy(pipe, std::back_inserter(out));
    return out;
}
