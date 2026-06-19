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

// ───────────────────────────────────────────────────────────────────────────
// Задание 2 (сложнее): Buffer — владелец сырой памяти new[] с ПОЛНЫМ правилом пяти
// ───────────────────────────────────────────────────────────────────────────
//
// Buffer владеет массивом double фиксированного размера, выделенным через new[].
// В отличие от IntVector, здесь размер не меняется (нет push_back) — зато всё
// внимание на пяти специальных методах и на КОРРЕКТНОМ перемещении.
//
// Контракт:
//   * Buffer(n)             — выделяет n элементов, инициализирует НУЛЯМИ.
//   * Buffer(n, v)          — выделяет n элементов, заполняет значением v.
//   * Buffer(n == 0)        — допустим: data() == nullptr, size() == 0.
//   * Копирование           — ГЛУБОКОЕ (свой буфер, независимое содержимое).
//   * Перемещение           — забирает буфер у источника; источник → (nullptr, 0).
//   * Перемещение noexcept.
//   * at(i) при i >= size() — бросает std::out_of_range.
//   * operator[] границы НЕ проверяет (как у std::vector).
//   * fill(v)               — записать v во все элементы.
//   * swap(other) noexcept  — обменять внутренности двух Buffer (для copy-and-swap).
class Buffer {
public:
    explicit Buffer(std::size_t count, double value = 0.0);
    ~Buffer();

    Buffer(const Buffer& other);                 // глубокая копия
    Buffer& operator=(const Buffer& other);      // копирующее присваивание
    Buffer(Buffer&& other) noexcept;             // перемещающий конструктор
    Buffer& operator=(Buffer&& other) noexcept;  // перемещающее присваивание

    std::size_t size() const;
    double* data();              // указатель на буфер (или nullptr для пустого)
    const double* data() const;

    double& operator[](std::size_t i);
    const double& operator[](std::size_t i) const;

    double& at(std::size_t i);             // bounds-checked, бросает std::out_of_range
    const double& at(std::size_t i) const;

    void fill(double value);
    void swap(Buffer& other) noexcept;

private:
    double* data_;
    std::size_t size_;
};

// ───────────────────────────────────────────────────────────────────────────
// Задание 3 (сложнее): UniqueHandle — move-only обёртка над «ресурсом»
// ───────────────────────────────────────────────────────────────────────────
//
// Моделирует уникальное владение неким хэндлом (как std::unique_ptr или дескриптор
// файла/сокета): копировать НЕЛЬЗЯ, перемещать — можно. После перемещения источник
// становится «пустым» (не владеет ничем).
//
// «Ресурс» здесь — целочисленный id, который мы берём из учебного реестра ниже.
// Реестр позволяет тестам проверить, что каждый захваченный id ровно один раз
// освобождается (никаких утечек и никаких двойных release).
//
//   * resource_acquire()    — выдаёт новый положительный id и помечает его «живым».
//   * resource_release(id)  — освобождает id; вызывать на уже мёртвом id запрещено.
//   * resource_live_count() — сколько id сейчас «живо» (для проверок в тестах).
//   * kInvalidHandle == 0   — «пустой» хэндл (ничем не владеет).
int  resource_acquire();
void resource_release(int id);
int  resource_live_count();
void resource_reset_registry();   // только для тестов: обнулить состояние реестра

inline constexpr int kInvalidHandle = 0;

class UniqueHandle {
public:
    UniqueHandle() noexcept;                 // пустой, ничем не владеет
    explicit UniqueHandle(int id) noexcept;  // берёт во владение готовый id
    ~UniqueHandle();

    // КОПИРОВАНИЕ ЗАПРЕЩЕНО — это move-only тип.
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept;             // украсть владение
    UniqueHandle& operator=(UniqueHandle&& other) noexcept;  // освободить своё, украсть чужое

    int  get() const noexcept;      // текущий id (kInvalidHandle, если пусто)
    bool valid() const noexcept;    // владеет ли чем-то
    explicit operator bool() const noexcept { return valid(); }

    int  release() noexcept;        // отдать владение наружу: вернуть id, стать пустым
    void reset(int id = kInvalidHandle) noexcept;  // освободить текущий, взять новый id
    void swap(UniqueHandle& other) noexcept;

private:
    int id_;
};

// Удобная фабрика: захватить ресурс из реестра и сразу обернуть его.
UniqueHandle make_handle();
