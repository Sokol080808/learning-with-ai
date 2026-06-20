#pragma once
#include <vector>
#include <optional>
#include <utility>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <array>

// Стек (LIFO) поверх std::vector. Реализуй методы прямо здесь (шаблон).
template <class T>
class Stack {
public:
    void push(const T& value) {
        data_.push_back(value);
    }
    void pop() {
        data_.pop_back();
    }
    const T& top() const {
        return data_.back();
    }
    bool empty() const {
        return data_.empty();
    }
    std::size_t size() const {
        return data_.size();
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
        auto node = std::make_unique<Node>(value);
        node->next = std::move(head_);
        head_ = std::move(node);
        ++size_;
    }

    // Добавить элемент в КОНЕЦ списка. O(n) (или O(1), если держишь tail_).
    void push_back(const T& value) {
        if (!head_) {
            push_front(value);
            return;
        }
        Node* p = head_.get();
        while (p->next) {
            p = p->next.get();
        }
        p->next = std::make_unique<Node>(value);
        ++size_;
    }

    // Сколько элементов в списке.
    std::size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    // Перенести элементы в vector в порядке от головы к хвосту.
    std::vector<T> to_vector() const {
        std::vector<T> out;
        out.reserve(size_);
        for (const Node* p = head_.get(); p != nullptr; p = p->next.get()) {
            out.push_back(p->value);
        }
        return out;
    }

    // Развернуть список на месте: голова становится хвостом и наоборот. O(n), без копий.
    void reverse() {
        std::unique_ptr<Node> result;  // накопленный развёрнутый список
        while (head_) {
            std::unique_ptr<Node> node = std::move(head_);  // оторвать голову
            head_ = std::move(node->next);                  // сдвинуть head_ на следующий
            node->next = std::move(result);                 // пристегнуть перед результатом
            result = std::move(node);
        }
        head_ = std::move(result);
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
        buf_.push_back(value);
    }

    // Снять верхний элемент. Если стек пуст — бросить std::out_of_range.
    void pop() {
        if (buf_.empty())
            throw std::out_of_range("ArrayStack::pop on empty");
        buf_.pop_back();
    }

    // Верхний элемент (без снятия). Если пуст — бросить std::out_of_range.
    const T& top() const {
        if (buf_.empty())
            throw std::out_of_range("ArrayStack::top on empty");
        return buf_.back();
    }

    bool empty() const {
        return buf_.empty();
    }

    std::size_t size() const {
        return buf_.size();
    }

private:
    std::vector<T> buf_;
};

// ─────────────────────────────────────────────────────────────────────────
// Задание 8. Очередь на кольцевом буфере Queue<T, CAPACITY>.
// FIFO-дисциплина: enqueue добавляет в хвост, dequeue убирает из головы.
// Кольцевой буфер: массив фиксированного размера, голова/хвост двигаются
// по модулю — «обёртка» (wrap-around). Нет динамической аллокации.
// ─────────────────────────────────────────────────────────────────────────
template <class T, std::size_t CAPACITY = 64>
class Queue {
public:
    // Добавить в хвост. Если буфер полон — бросить std::overflow_error.
    void enqueue(const T& value) {
        if (size_ == CAPACITY)
            throw std::overflow_error("Queue::enqueue on full queue");
        buf_[tail_] = value;
        tail_ = (tail_ + 1) % CAPACITY;
        ++size_;
    }

    // Убрать из головы. Если пуста — бросить std::underflow_error.
    void dequeue() {
        if (size_ == 0)
            throw std::underflow_error("Queue::dequeue on empty queue");
        head_ = (head_ + 1) % CAPACITY;
        --size_;
    }

    // Первый элемент (без удаления). Если пуста — бросить std::underflow_error.
    const T& front() const {
        if (size_ == 0)
            throw std::underflow_error("Queue::front on empty queue");
        return buf_[head_];
    }

    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }

private:
    std::array<T, CAPACITY> buf_{};  // кольцевой буфер фиксированного размера
    std::size_t head_ = 0;           // индекс головы (следующий для dequeue)
    std::size_t tail_ = 0;           // индекс хвоста (следующий для enqueue)
    std::size_t size_ = 0;           // число элементов в буфере
};

// ─────────────────────────────────────────────────────────────────────────
// Задание 9. Сортировка слиянием — O(n log n), divide-and-conquer.
// Реализовано в src/task13.cpp.
// ─────────────────────────────────────────────────────────────────────────
std::vector<int> merge_sort(std::vector<int> v);

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
