#include "task03.hpp"

#include <stdexcept>

int sum_array(const int* arr, int n) {
    int total = 0;
    for (int i = 0; i < n; ++i) {
        total += arr[i];
    }
    return total;
}

void swap_ints(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int count_value(const int* arr, int n, int value) {
    int count = 0;
    for (int i = 0; i < n; ++i) {
        if (arr[i] == value) {
            ++count;
        }
    }
    return count;
}

const int* max_element_ptr(const int* arr, int n) {
    if (n == 0) {
        return nullptr;
    }
    const int* best = arr;
    for (int i = 1; i < n; ++i) {
        if (arr[i] > *best) {
            best = arr + i;
        }
    }
    return best;
}

void reverse_in_place(int* arr, int n) {
    for (int i = 0, j = n - 1; i < j; ++i, --j) {
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

// ── Задание 6 ────────────────────────────────────────────────────────────────
void reverse_with_pointers(int* arr, int n) {
    if (n <= 0) {
        return;  // нечего разворачивать; и не считаем arr + n - 1 на nullptr
    }
    int* left = arr;
    int* right = arr + n - 1;
    while (left < right) {
        int tmp = *left;
        *left = *right;
        *right = tmp;
        ++left;
        --right;
    }
}

// ── Задание 7: DynArray ──────────────────────────────────────────────────────
DynArray::DynArray(int n, int fill) : data_(nullptr), size_(0) {
    if (n > 0) {
        data_ = new int[n];
        size_ = n;
        for (int i = 0; i < n; ++i) {
            data_[i] = fill;
        }
    }
}

DynArray::~DynArray() {
    delete[] data_;
}

int DynArray::size() const {
    return size_;
}

int& DynArray::at(int i) {
    if (i < 0 || i >= size_) {
        throw std::out_of_range("DynArray::at: index out of range");
    }
    return data_[i];
}

int DynArray::at(int i) const {
    if (i < 0 || i >= size_) {
        throw std::out_of_range("DynArray::at: index out of range");
    }
    return data_[i];
}

int DynArray::sum() const {
    int total = 0;
    for (int i = 0; i < size_; ++i) {
        total += data_[i];
    }
    return total;
}

void DynArray::fill(int value) {
    for (int i = 0; i < size_; ++i) {
        data_[i] = value;
    }
}

// ── Задание 9: DynArray — правило трёх (глубокое копирование) ───────────────────

DynArray::DynArray(const DynArray& other)
    : data_(other.size_ > 0 ? new int[other.size_] : nullptr),
      size_(other.size_) {
    // Копируем значения поэлементно — глубокая копия.
    for (int i = 0; i < size_; ++i) {
        data_[i] = other.data_[i];
    }
}

DynArray& DynArray::operator=(const DynArray& other) {
    if (this == &other) {
        return *this;  // (1) самоприсваивание: ничего не делаем
    }
    delete[] data_;    // (2) освобождаем свой старый буфер
    size_ = other.size_;
    data_ = size_ > 0 ? new int[size_] : nullptr;  // (3) новый буфер
    for (int i = 0; i < size_; ++i) {              // (4) копируем значения
        data_[i] = other.data_[i];
    }
    return *this;      // (5) возвращаем *this по соглашению
}

// ── Задание 8: OwnedInt (глубокое vs поверхностное копирование) ───────────────
OwnedInt::OwnedInt(int value) : ptr_(new int(value)) {
    // Конструктор уже корректен: один int на куче.
}

OwnedInt::OwnedInt(const OwnedInt& other) : ptr_(new int(*other.ptr_)) {
    // Глубокое копирование: свой буфер на куче со ЗНАЧЕНИЕМ оригинала.
}

OwnedInt& OwnedInt::operator=(const OwnedInt& other) {
    if (this == &other) {
        return *this;  // защита от самоприсваивания
    }
    *ptr_ = *other.ptr_;  // свой буфер уже выделен — переписываем значение
    return *this;
}

OwnedInt::~OwnedInt() {
    delete ptr_;
}

int OwnedInt::get() const {
    return *ptr_;
}

void OwnedInt::set(int value) {
    *ptr_ = value;
}

const int* OwnedInt::raw() const {
    return ptr_;
}
