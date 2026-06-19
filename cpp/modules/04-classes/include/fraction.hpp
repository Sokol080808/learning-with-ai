#pragma once
#include <string>
#include <iosfwd>
#include <cstdint>

// Рациональная дробь. Инварианты (всегда истинны для корректного объекта):
//   * хранится в сокращённом виде (числитель и знаменатель взаимно просты);
//   * знаменатель строго положителен, знак — в числителе;
//   * ноль представлен как 0/1.
class Fraction {
public:
    // Создаёт дробь num/den и сразу нормализует её. den != 0 (гарантируется тестами).
    Fraction(int num, int den);

    int numerator() const;
    int denominator() const;

    Fraction add(const Fraction& o) const;
    Fraction multiply(const Fraction& o) const;

    bool operator==(const Fraction& o) const;

    std::string to_string() const;  // вид "num/den"

    // --- Новые задания (см. README, «## Задания», п. 2) ---
    // Перегрузки операторов: естественная арифметика и сравнение для дроби.
    Fraction operator+(const Fraction& o) const;   // a + b
    Fraction operator*(const Fraction& o) const;   // a * b
    bool operator<(const Fraction& o) const;       // строгий порядок: a < b

private:
    int num_;
    int den_;
};

// operator<< печатает дробь в поток в виде "num/den" (например, "-1/2").
// Свободная функция, потому что левый операнд — это std::ostream, а не Fraction.
std::ostream& operator<<(std::ostream& os, const Fraction& f);


// ──────────────────────────────────────────────────────────────────────────
// Задание 3. RAII-таймер.
// Timer запоминает момент создания. accumulator — внешний счётчик (в наносекундах),
// к которому в ДЕСТРУКТОРЕ прибавляется прожитое объектом время. Так измеряется
// длительность области видимости: пока Timer жив — идёт замер, разрушился — записал.
//
// Это иллюстрация RAII: «ресурс» здесь — измеряемый интервал времени.
class Timer {
public:
    // Запоминает текущее время (std::chrono::steady_clock) и адрес внешнего счётчика.
    explicit Timer(std::int64_t& accumulator_ns);

    // В деструкторе: прибавляет к *accumulator_ns_ число наносекунд, прошедших
    // с момента конструирования.
    ~Timer();

    // Timer владеет уникальным замером — копировать его бессмысленно и опасно.
    // Запрещаем копирование (правило: некопируемый RAII-страж).
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

private:
    std::int64_t* accumulator_ns_;
    std::int64_t  start_ns_;        // момент старта, наносекунды от точки отсчёта часов
};


// ──────────────────────────────────────────────────────────────────────────
// Задание 4. Вектор на плоскости (2D). Простой «мешок данных» с операторами.
// Поля публичные намеренно: инварианта здесь нет, любые x/y допустимы.
struct Vec2 {
    double x;
    double y;

    Vec2 operator+(const Vec2& o) const;   // покомпонентная сумма
    Vec2 operator-(const Vec2& o) const;   // покомпонентная разность
    bool operator==(const Vec2& o) const;  // точное равенство компонент

    double dot(const Vec2& o) const;       // скалярное произведение x*x' + y*y'
};
