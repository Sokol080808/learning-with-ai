#pragma once
#include <cstddef>  // std::size_t

// Учебный аналог std::vector<int>: динамический массив, который сам управляет памятью.
// Цель — реализовать «большую пятёрку» и понять copy/move-семантику.
class IntVector {
public:
    IntVector();                                      // пустой
    explicit IntVector(std::size_t count, int value = 0);
    ~IntVector();

    IntVector(const IntVector& other);                // копирующий конструктор
    IntVector& operator=(const IntVector& other);     // копирующее присваивание
    IntVector(IntVector&& other) noexcept;            // перемещающий конструктор
    IntVector& operator=(IntVector&& other) noexcept; // перемещающее присваивание

    void push_back(int value);

    std::size_t size() const;
    std::size_t capacity() const;
    bool empty() const;

    int& operator[](std::size_t i);
    const int& operator[](std::size_t i) const;

private:
    int* data_;
    std::size_t size_;
    std::size_t capacity_;
};
