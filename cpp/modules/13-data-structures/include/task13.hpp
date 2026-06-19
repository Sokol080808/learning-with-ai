#pragma once
#include <vector>
#include <optional>
#include <utility>
#include <cstddef>
#include <memory>
#include <stdexcept>

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
        throw std::logic_error("TODO: Stack::top");
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

// ─────────────────────────────────────────────────────────────────────────
// Задание 5. Односвязный список SinglyLinkedList<T>.
// Узлы лежат в куче, каждый держит next через unique_ptr (владение — линия узлов).
// Реализуй методы прямо здесь (шаблон, см. модуль 07).
// ─────────────────────────────────────────────────────────────────────────
template <class T>
class SinglyLinkedList {
public:
    SinglyLinkedList() = default;

    // Добавить элемент в НАЧАЛО списка. O(1).
    void push_front(const T& value) {
        // TODO
        (void)value;
    }

    // Добавить элемент в КОНЕЦ списка. O(n) (или O(1), если держишь tail_).
    void push_back(const T& value) {
        // TODO
        (void)value;
    }

    // Сколько элементов в списке.
    std::size_t size() const {
        // TODO
        return 0;
    }

    bool empty() const {
        // TODO
        return true;
    }

    // Перенести элементы в vector в порядке от головы к хвосту.
    std::vector<T> to_vector() const {
        // TODO
        return {};
    }

    // Развернуть список на месте: голова становится хвостом и наоборот. O(n), без копий.
    void reverse() {
        // TODO
    }

private:
    struct Node {
        T value;
        std::unique_ptr<Node> next;
        explicit Node(const T& v) : value(v), next(nullptr) {}
    };
    std::unique_ptr<Node> head_;
    std::size_t size_ = 0;
};

// ─────────────────────────────────────────────────────────────────────────
// Задание 6. Стек на массиве ArrayStack<T> (без std::vector в публичном поведении —
// внутри можешь использовать vector как растущий буфер, это нормально).
// Отличие от Stack: pop() на пустом и top() на пустом должны БРОСАТЬ
// std::out_of_range (а не падать в UB).
// ─────────────────────────────────────────────────────────────────────────
template <class T>
class ArrayStack {
public:
    // Положить элемент наверх.
    void push(const T& value) {
        // TODO
        (void)value;
    }

    // Снять верхний элемент. Если стек пуст — бросить std::out_of_range.
    void pop() {
        // TODO: на пустом — throw std::out_of_range("ArrayStack::pop on empty")
    }

    // Верхний элемент (без снятия). Если пуст — бросить std::out_of_range.
    const T& top() const {
        // TODO: на пустом — throw std::out_of_range("ArrayStack::top on empty")
        throw std::logic_error("TODO: ArrayStack::top");
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
    std::vector<T> buf_;
};

// ─────────────────────────────────────────────────────────────────────────
// Задание 7. Дерево бинарного поиска (BST) по int.
// insert — вставить значение (дубликаты игнорировать, множество значений).
// contains — есть ли значение.
// inorder — обход «лево-корень-право» даёт ОТСОРТИРОВАННЫЙ вектор.
// ─────────────────────────────────────────────────────────────────────────
class BST {
public:
    BST() = default;

    // Вставить value. Дубликаты не добавлять повторно. В среднем O(log n).
    void insert(int value);

    // Есть ли value в дереве. В среднем O(log n).
    bool contains(int value) const;

    // In-order обход → отсортированный по возрастанию вектор значений.
    std::vector<int> inorder() const;

    // Число различных значений в дереве.
    std::size_t size() const;

private:
    struct Node {
        int value;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        explicit Node(int v) : value(v), left(nullptr), right(nullptr) {}
    };
    std::unique_ptr<Node> root_;
    std::size_t size_ = 0;
};
