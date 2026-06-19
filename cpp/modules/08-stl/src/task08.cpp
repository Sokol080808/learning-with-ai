#include "task08.hpp"
// #include <algorithm>
// #include <iterator>

std::vector<int> evens(const std::vector<int>& v) {
    // TODO: используй std::copy_if + std::back_inserter
    (void)v;
    return {};
}

int count_greater(const std::vector<int>& v, int threshold) {
    // TODO: std::count_if + лямбда с захватом threshold
    (void)v; (void)threshold;
    return 0;
}

std::vector<int> squared(const std::vector<int>& v) {
    // TODO: std::transform
    (void)v;
    return {};
}

std::vector<int> sorted_desc(std::vector<int> v) {
    // TODO: std::sort с компаратором «по убыванию», верни v
    return v;
}

std::map<char, int> char_frequency(const std::string& s) {
    // TODO
    (void)s;
    return {};
}

std::map<std::string, int> word_frequency(const std::string& text) {
    // TODO: std::istringstream + чтение iss >> word; каждое слово в нижний регистр
    // (только латиница), затем ++freq[word].
    (void)text;
    return {};
}

std::vector<int> top_k(std::vector<int> v, int k) {
    // TODO: обработай k<=0 и k>=size; используй std::nth_element или std::partial_sort,
    // затем верни ровно k наибольших, отсортированных по убыванию.
    (void)k;
    return v;
}

std::size_t drop_below(std::vector<int>& v, int threshold) {
    // TODO: идиома erase-remove. std::remove_if переносит «выживших» в начало и
    // возвращает итератор на новый конец; число удалённых = end() - этот итератор;
    // затем v.erase(...) реально обрезает хвост.
    (void)v; (void)threshold;
    return 0;
}
