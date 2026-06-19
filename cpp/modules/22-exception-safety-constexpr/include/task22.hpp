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
        // Reference answer key: резервируем место ЗАРАНЕЕ, до изменения видимого
        // состояния. Реаллокация (источник броска) делается отдельным шагом; если
        // она бросит, размер ещё не изменён. После reserve места заведомо хватает,
        // поэтому push_back не реаллоцирует — лишь копирует value в подготовленный
        // буфер. Если копирование value бросит, vector::push_back сам откатит и
        // size() не изменится — строгая гарантия сохранена.
        if (data_.size() == data_.capacity()) {
            // Растём с запасом, чтобы после push всегда оставалось место (capacity >
            // size). Первая реаллокация выделяет минимум 2, дальше удваиваем.
            data_.reserve(data_.empty() ? 2 : data_.capacity() * 2);
        }
        data_.push_back(value);
    }

    void pop() {
        if (data_.empty()) {
            throw std::out_of_range("Stack::pop on empty stack");
        }
        data_.pop_back();
    }

    const T& top() const {
        if (data_.empty()) {
            throw std::out_of_range("Stack::top on empty stack");
        }
        return data_.back();
    }

    bool empty() const {
        return data_.empty();
    }

    std::size_t size() const {
        return data_.size();
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
        active_ = false;
    }

    ~ScopeGuard() noexcept {
        // Reference answer key: если guard ещё активен и действие задано — выполнить
        // его ровно один раз. Из деструктора исключения наружу не выпускаем (поэтому
        // try/catch), иначе при раскрутке стека получили бы std::terminate.
        if (active_ && action_) {
            try {
                action_();
            } catch (...) {
                // молча проглатываем — деструктор обязан быть noexcept
            }
        }
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
        : size_(other.size_), data_(other.size_ == 0 ? nullptr : new int[other.size_]) {
        // Reference answer key: глубокая копия — свой массив + поэлементное копирование.
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    // Перемещение: забрать ресурс у other, оставив его пустым (size 0, data null).
    Buffer(Buffer&& other) noexcept
        : size_(other.size_), data_(other.data_) {
        // Reference answer key: украли ресурс — обнуляем источник, чтобы его
        // деструктор не освободил уже наш массив.
        other.size_ = 0;
        other.data_ = nullptr;
    }

    // ОДИН operator= по значению — это и есть copy-and-swap.
    // Аргумент other уже скопирован/перемещён компилятором; остаётся swap.
    Buffer& operator=(Buffer other) noexcept {
        // Reference answer key: вся тяжёлая работа (копия/перемещение) сделана при
        // создании other; забираем его ресурс к себе, старый уедет в other и умрёт.
        swap(*this, other);
        return *this;
    }

    ~Buffer() {
        delete[] data_;
    }

    std::size_t size() const { return size_; }

    int at(std::size_t i) const {
        if (i >= size_) {
            throw std::out_of_range("Buffer::at index out of range");
        }
        return data_[i];
    }

    void set(std::size_t i, int v) {
        if (i >= size_) {
            throw std::out_of_range("Buffer::set index out of range");
        }
        data_[i] = v;
    }

    // Обмен внутренностей двух буферов без бросков.
    friend void swap(Buffer& a, Buffer& b) noexcept {
        using std::swap;
        // Reference answer key: обмениваем именно поля-владельцы — размер и указатель.
        swap(a.size_, b.size_);
        swap(a.data_, b.data_);
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
    // Reference answer key.
    unsigned long long result = 1;
    for (unsigned i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

// Наибольший общий делитель (алгоритм Евклида). gcd(0, x) == x.
constexpr unsigned gcd(unsigned a, unsigned b) {
    // Reference answer key: пока b != 0, (a, b) = (b, a % b).
    while (b != 0) {
        unsigned t = a % b;
        a = b;
        b = t;
    }
    return a;
}

// Простое ли число. 0 и 1 — не простые; 2 — простое.
constexpr bool is_prime(unsigned n) {
    // Reference answer key: 0 и 1 не простые; проверяем делители до корня из n.
    if (n < 2) {
        return false;
    }
    for (unsigned d = 2; d * d <= n; ++d) {
        if (n % d == 0) {
            return false;
        }
    }
    return true;
}

// -----------------------------------------------------------------------------
// Задание 5. Работа со строками и массивами на этапе компиляции.
// -----------------------------------------------------------------------------

// Длина C-строки (без завершающего '\0'), посчитанная в compile-time.
// cstr_len("") == 0, cstr_len("abc") == 3.
constexpr std::size_t cstr_len(const char* s) {
    // Reference answer key: идём до нуль-терминатора.
    std::size_t n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
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
    // Reference answer key: result[i] == in[N-1-i].
    Array<T, N> out{};
    for (std::size_t i = 0; i < N; ++i) {
        out[i] = in[N - 1 - i];
    }
    return out;
}
