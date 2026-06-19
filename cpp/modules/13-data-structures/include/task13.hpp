#pragma once
#include <vector>
#include <optional>
#include <utility>
#include <cstddef>

// Стек (LIFO) поверх std::vector. Реализуй методы прямо здесь (шаблон).
template <class T>
class Stack {
public:
    void push(const T& value) {
        // TODO
        (void)value;
    }
    void pop() {
        // TODO: убрать верхний элемент
    }
    const T& top() const {
        // TODO: вернуть верхний элемент
        return data_.back();
    }
    bool empty() const {
        // TODO
        return true;
    }
    std::size_t size() const {
        // TODO
        return 0;
    }

private:
    std::vector<T> data_;
};

// Индекс target в отсортированном массиве или -1. Сложность O(log n).
int binary_search_idx(const std::vector<int>& sorted, int target);

// Сортировка по возрастанию своей реализацией (не std::sort).
std::vector<int> insertion_sort(std::vector<int> v);

// Индексы (i, j), i < j, с v[i] + v[j] == target; иначе nullopt. O(n).
std::optional<std::pair<int, int>> two_sum(const std::vector<int>& v, int target);
