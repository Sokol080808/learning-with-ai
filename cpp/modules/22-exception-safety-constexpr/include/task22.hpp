#pragma once
#include <vector>
#include <utility>
#include <cstddef>
#include <stdexcept>
#include <functional>

// =============================================================================
// Модуль 22 — Гарантии исключений и вычисления на этапе компиляции.
// Весь код пиши прямо здесь: модуль header-only (шаблоны + constexpr).
// =============================================================================

// -----------------------------------------------------------------------------
// Задание 1. Stack<T> со СТРОГОЙ гарантией исключений на push.
//
// Строгая гарантия: если push бросает исключение, состояние стека ОСТАЁТСЯ
// ровно таким, каким было до вызова (старые элементы целы, size() не меняется).
// Подсказка по реализации: зарезервируй место ЗАРАНЕЕ (reserve), и только потом,
// когда выделение памяти уже точно прошло, добавляй элемент в подготовленный
// буфер. Реаллокация — единственное место, где push мог бы бросить и испортить
// состояние; вынеси её отдельно.
// -----------------------------------------------------------------------------
template <class T>
class Stack {
public:
    void push(const T& value) {
        // TODO: добавить value на вершину так, чтобы при броске (например,
        // при копировании value или при выделении памяти) стек не изменился.
        (void)value;
        throw std::logic_error("TODO: Stack::push");
    }

    void pop() {
        // TODO: убрать верхний элемент; на пустом стеке бросить std::out_of_range
        throw std::logic_error("TODO: Stack::pop");
    }

    const T& top() const {
        // TODO: вернуть верхний элемент; на пустом стеке бросить std::out_of_range
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

    // Сколько элементов помещается без новой реаллокации (для проверки reserve).
    std::size_t capacity() const {
        return data_.capacity();
    }

private:
    std::vector<T> data_;
};

// -----------------------------------------------------------------------------
// Задание 2. ScopeGuard — выполняет действие в деструкторе (RAII для «отката»).
//
// Идея: ты заводишь guard на действие-уборку (закрыть файл, освободить ресурс).
// Если всё прошло хорошо — зовёшь dismiss(), и действие НЕ выполняется.
// Если выходишь из области по исключению/раньше — деструктор всё уберёт за тебя.
// Деструктор обязан быть noexcept: бросать из него нельзя.
// -----------------------------------------------------------------------------
class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> action)
        : action_(std::move(action)), active_(true) {}

    // Запрещаем копирование (иначе действие выполнилось бы дважды).
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // Отменить срабатывание: после этого деструктор ничего не делает.
    void dismiss() {
        // TODO
    }

    ~ScopeGuard() noexcept {
        // TODO: если guard ещё активен (не dismissed) и действие задано —
        // выполнить его. Из деструктора исключения наружу выпускать нельзя.
    }

private:
    std::function<void()> action_;
    bool active_;
};

// -----------------------------------------------------------------------------
// Задание 3. Buffer — владелец динамического массива int.
// Реализуй копирование/присваивание через идиому copy-and-swap.
//
// Контракт:
//   Buffer(n)            — буфер из n нулей.
//   size(), at(i)        — доступ; at с выходом за границы бросает std::out_of_range.
//   set(i, v)            — записать; за границей бросает std::out_of_range.
//   копирование/=        — глубокая копия.
//   operator= по значению + swap (copy-and-swap): одна реализация на копию и на
//                          перемещение, безопасная при самоприсваивании.
//   swap(a, b)           — обмен без бросков (noexcept).
// -----------------------------------------------------------------------------
class Buffer {
public:
    explicit Buffer(std::size_t n)
        : size_(n), data_(n == 0 ? nullptr : new int[n]()) {}

    Buffer(const Buffer& other)
        : size_(0), data_(nullptr) {
        // TODO: глубокая копия other (выдели свой массив и скопируй элементы)
        (void)other;
        throw std::logic_error("TODO: Buffer copy ctor");
    }

    // Перемещение: забрать ресурс у other, оставив его пустым (size 0, data null).
    Buffer(Buffer&& other) noexcept
        : size_(0), data_(nullptr) {
        // TODO: украсть ресурс у other
        (void)other;
    }

    // ОДИН operator= по значению — это и есть copy-and-swap.
    // Аргумент other уже скопирован/перемещён компилятором; остаётся swap.
    Buffer& operator=(Buffer other) noexcept {
        // TODO: swap(*this, other) и вернуть *this
        (void)other;
        return *this;
    }

    ~Buffer() {
        delete[] data_;
    }

    std::size_t size() const { return size_; }

    int at(std::size_t i) const {
        // TODO: вернуть элемент; за границей бросить std::out_of_range
        (void)i;
        throw std::logic_error("TODO: Buffer::at");
    }

    void set(std::size_t i, int v) {
        // TODO: записать v в позицию i; за границей бросить std::out_of_range
        (void)i; (void)v;
        throw std::logic_error("TODO: Buffer::set");
    }

    // Обмен внутренностей двух буферов без бросков.
    friend void swap(Buffer& a, Buffer& b) noexcept {
        using std::swap;
        // TODO: обменять size_ и data_ местами
        (void)a; (void)b;
    }

private:
    std::size_t size_;
    int* data_;
};

// -----------------------------------------------------------------------------
// Задание 4. constexpr-математика. Эти функции обязаны считаться на этапе
// компиляции (их проверяют через static_assert в тестах).
// -----------------------------------------------------------------------------

// Факториал: factorial(0) == 1, factorial(5) == 120.
constexpr unsigned long long factorial(unsigned n) {
    // TODO
    (void)n;
    return 0;
}

// Наибольший общий делитель (алгоритм Евклида). gcd(0, x) == x.
constexpr unsigned gcd(unsigned a, unsigned b) {
    // TODO
    (void)a; (void)b;
    return 0;
}

// Простое ли число. 0 и 1 — не простые; 2 — простое.
constexpr bool is_prime(unsigned n) {
    // TODO
    (void)n;
    return true;
}

// -----------------------------------------------------------------------------
// Задание 5. Работа со строками и массивами на этапе компиляции.
// -----------------------------------------------------------------------------

// Длина C-строки (без завершающего '\0'), посчитанная в compile-time.
// cstr_len("") == 0, cstr_len("abc") == 3.
constexpr std::size_t cstr_len(const char* s) {
    // TODO: пройди по символам до '\0' и посчитай их
    (void)s;
    return 0;
}

// Класс-обёртка над массивом фиксированной длины N (constexpr-дружественный).
// Нужен, чтобы вернуть массив из constexpr-функции и проверить его в static_assert.
template <class T, std::size_t N>
struct Array {
    T data[N]{};

    constexpr T& operator[](std::size_t i) { return data[i]; }
    constexpr const T& operator[](std::size_t i) const { return data[i]; }
    constexpr std::size_t size() const { return N; }

    constexpr bool operator==(const Array& other) const {
        for (std::size_t i = 0; i < N; ++i) {
            if (data[i] != other.data[i]) return false;
        }
        return true;
    }
};

// Развернуть массив задом наперёд на этапе компиляции.
// reversed({1,2,3}) == {3,2,1}.
template <class T, std::size_t N>
constexpr Array<T, N> reversed(const Array<T, N>& in) {
    // TODO: собери результат, где result[i] == in[N-1-i]
    (void)in;
    return Array<T, N>{};
}

// -----------------------------------------------------------------------------
// Задание 6. my_swap — обобщённый swap с корректным условным noexcept.
//
// Контракт:
//   my_swap(a, b) — обменять два значения через move.
//   noexcept(my_swap(a, b)) == true  iff  T — noexcept-move-constructible &&
//                                          noexcept-move-assignable.
//
// Ключевая идея: спецификатор noexcept(EXPR) принимает compile-time bool;
// std::is_nothrow_move_constructible_v<T> и std::is_nothrow_move_assignable_v<T>
// дают нужные трейты. Тело — стандартные три move-хода.
//
// Вспомогательный тип NoexceptWrapper<T>: форсирует noexcept-move у любого T,
// оборачивая его move-конструктор в явный noexcept. Нужен в тестах, чтобы
// контролировать ветку «noexcept-движение разрешено».
// -----------------------------------------------------------------------------

// Обёртка: делает move noexcept независимо от внутреннего типа.
// Хранит T по значению; все операции делегируются, move явно noexcept.
template <class T>
struct NoexceptWrapper {
    T value;

    NoexceptWrapper() = default;
    explicit NoexceptWrapper(T v) : value(std::move(v)) {}

    // Принудительно noexcept move — это и есть цель обёртки.
    // Сигнатуры noexcept сохранены, чтобы static_assert в тестах компилировался.
    NoexceptWrapper(NoexceptWrapper&& other) noexcept
        : value(std::move(other.value)) {}
    NoexceptWrapper& operator=(NoexceptWrapper&& other) noexcept {
        // TODO: реализуй перемещающее присваивание
        (void)other;
        return *this;
    }

    // Копирование разрешено через T.
    NoexceptWrapper(const NoexceptWrapper&) = default;
    NoexceptWrapper& operator=(const NoexceptWrapper&) = default;
};

// Обобщённый swap через move с корректным условным noexcept.
// noexcept-спецификация сохранена корректной (трейты), чтобы
// static_assert в тестах компилировался; рантайм-логика — заглушка.
template <class T>
void my_swap(T& a, T& b)
    noexcept(std::is_nothrow_move_constructible_v<T> &&
             std::is_nothrow_move_assignable_v<T>)
{
    // TODO: реализуй три move-хода: tmp = move(a); a = move(b); b = move(tmp)
    (void)a; (void)b;
}
