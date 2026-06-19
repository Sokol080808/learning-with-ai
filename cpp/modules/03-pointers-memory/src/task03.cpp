#include "task03.hpp"

#include <stdexcept>

int sum_array(const int* arr, int n) {
    // TODO
    (void)arr; (void)n;
    return 0;
}

void swap_ints(int* a, int* b) {
    // TODO: поменять *a и *b местами
    (void)a; (void)b;
}

int count_value(const int* arr, int n, int value) {
    // TODO
    (void)arr; (void)n; (void)value;
    return 0;
}

const int* max_element_ptr(const int* arr, int n) {
    // TODO: вернуть указатель на максимум; nullptr если n == 0
    (void)arr; (void)n;
    return nullptr;
}

void reverse_in_place(int* arr, int n) {
    // TODO: развернуть на месте
    (void)arr; (void)n;
}

// ── Задание 6 ────────────────────────────────────────────────────────────────
void reverse_with_pointers(int* arr, int n) {
    // TODO: завести два указателя (на начало и на конец) и сходиться к центру,
    // меняя *left и *right местами. Без arr[i], без выделения памяти.
    (void)arr; (void)n;
}

// ── Задание 7: DynArray ──────────────────────────────────────────────────────
DynArray::DynArray(int n, int fill) : data_(nullptr), size_(0) {
    // TODO: выделить буфер на n элементов (new int[n]) и заполнить значением fill.
    // Не забудь сохранить size_. Для n == 0 буфер можно не выделять.
    (void)n; (void)fill;
}

DynArray::~DynArray() {
    // TODO: освободить буфер парным delete[] (а не delete).
}

int DynArray::size() const {
    // TODO
    return 0;
}

int& DynArray::at(int i) {
    // TODO: проверить диапазон, иначе бросить std::out_of_range; вернуть data_[i].
    (void)i;
    throw std::logic_error("TODO: DynArray::at (неконстантный) не реализован");
}

int DynArray::at(int i) const {
    // TODO: проверить диапазон, иначе бросить std::out_of_range; вернуть data_[i].
    (void)i;
    throw std::logic_error("TODO: DynArray::at (const) не реализован");
}

int DynArray::sum() const {
    // TODO: сложить все элементы.
    return 0;
}

void DynArray::fill(int value) {
    // TODO: записать value во все элементы.
    (void)value;
}

// ── Задание 8: OwnedInt (глубокое vs поверхностное копирование) ───────────────
OwnedInt::OwnedInt(int value) : ptr_(new int(value)) {
    // Конструктор уже корректен: один int на куче.
}

OwnedInt::OwnedInt(const OwnedInt& other) : ptr_(new int(0)) {
    // TODO: сделать копию ГЛУБОКОЙ — скопировать ЗНАЧЕНИЕ other в свой буфер.
    // Сейчас буфер свой, но значение не скопировано — тест это поймает.
    (void)other;
}

OwnedInt& OwnedInt::operator=(const OwnedInt& other) {
    // TODO: глубокое присваивание. Защитись от self-assignment,
    // скопируй значение в СВОЙ буфер (не дели указатель, не теки).
    (void)other;
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
