#include "int_vector.hpp"
// #include <algorithm>  // может пригодиться std::copy / std::swap

// Реализуй методы IntVector. Сейчас всё — стабы (тесты красные).
// Внимание к памяти: каждому new[] должен соответствовать ровно один delete[].

IntVector::IntVector()
    : data_(nullptr), size_(0), capacity_(0) {
    // пустой вектор — ничего выделять не нужно
}

IntVector::IntVector(std::size_t count, int value)
    : data_(nullptr), size_(0), capacity_(0) {
    // TODO: выдели массив на count элементов и заполни значением value
    (void)count; (void)value;
}

IntVector::~IntVector() {
    // TODO: освободи память
}

IntVector::IntVector(const IntVector& other)
    : data_(nullptr), size_(0), capacity_(0) {
    // TODO: глубокое копирование из other
    (void)other;
}

IntVector& IntVector::operator=(const IntVector& other) {
    // TODO: безопасно (учти самоприсваивание; освободи старое)
    (void)other;
    return *this;
}

IntVector::IntVector(IntVector&& other) noexcept
    : data_(nullptr), size_(0), capacity_(0) {
    // TODO: забери ресурсы у other, оставь other пустым
    (void)other;
}

IntVector& IntVector::operator=(IntVector&& other) noexcept {
    // TODO
    (void)other;
    return *this;
}

void IntVector::push_back(int value) {
    // TODO: при необходимости вырасти, затем добавить элемент
    (void)value;
}

std::size_t IntVector::size() const {
    return size_;
}

std::size_t IntVector::capacity() const {
    return capacity_;
}

bool IntVector::empty() const {
    // TODO
    return true;
}

int& IntVector::operator[](std::size_t i) {
    // TODO
    return data_[i];
}

const int& IntVector::operator[](std::size_t i) const {
    // TODO
    return data_[i];
}
