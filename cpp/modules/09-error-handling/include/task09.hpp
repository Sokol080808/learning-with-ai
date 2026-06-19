#pragma once
#include <optional>
#include <variant>
#include <string>
#include <vector>
#include <stdexcept>

// Делит a на b; при b == 0 бросает std::invalid_argument.
int safe_divide(int a, int b);

// Возвращает v[index]; при выходе за границы бросает std::out_of_range.
int element_at(const std::vector<int>& v, int index);

// Парсит целое число; при некорректной строке возвращает std::nullopt.
std::optional<int> to_int(const std::string& s);

// Первый чётный элемент или std::nullopt.
std::optional<int> first_even(const std::vector<int>& v);

// "int: N" для int, "string: S" для строки.
std::string describe(const std::variant<int, std::string>& v);

// ---------------------------------------------------------------------------
// Дополнительные (более сложные) задания.
// ---------------------------------------------------------------------------

// Задание 6.
// Полный разбор десятичного целого со знаком.
// Принимает необязательный ведущий '+'/'-', затем одну и более десятичных цифр.
// Возвращает std::nullopt, если строка пустая, содержит пробелы/посторонние
// символы (включая "12abc" и "  5") или не содержит ни одной цифры.
// Переполнение int также считается ошибкой и даёт std::nullopt.
std::optional<int> parse_int(const std::string& s);

// Задание 8.
// Собственный тип исключения и безопасное деление, бросающее ИМЕННО его.
// (Объявлено до Expected, потому что DivideByZero ни от чего из задания 7 не зависит.)
class DivideByZero : public std::runtime_error {
public:
    DivideByZero() : std::runtime_error("division by zero") {}
};

// При b == 0 бросает DivideByZero (а НЕ std::invalid_argument).
// Иначе возвращает a / b.
int safe_div(int a, int b);

// Задание 7.
// Expected<T, E> — «либо значение типа T, либо ошибка типа E».
// Лёгкая ручная альтернатива std::expected (которого нет в C++20).
//
// Контракт:
//   * Expected(value)      — успешный результат, has_value() == true.
//   * Expected::fail(err)  — ошибочный результат, has_value() == false.
//   * value()              — вернуть значение; если ошибка — бросить std::logic_error.
//   * error()              — вернуть ошибку; если значение — бросить std::logic_error.
//   * explicit operator bool() — синоним has_value().
//
// Реализация хранит ровно одно из двух (T или E) в std::variant.
template <typename T, typename E>
class Expected {
public:
    // Успех. std::in_place_index<0> явно кладёт значение в первую альтернативу —
    // иначе при T == E (например, Expected<int,int>) variant неоднозначен.
    Expected(T value) : data_(std::in_place_index<0>, std::move(value)) {}

    // Фабрика ошибки. Названа явно, чтобы не путать значение и ошибку,
    // когда T и E совпадают (например, Expected<int,int>).
    static Expected fail(E error) {
        Expected e;
        e.data_.template emplace<1>(std::move(error));
        return e;
    }

    // Эталонный ключ к ответам (reference answer key).

    bool has_value() const noexcept {
        return data_.index() == 0;
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    const T& value() const {
        if (!has_value())
            throw std::logic_error("Expected::value() called on an error");
        return std::get<0>(data_);
    }

    const E& error() const {
        if (has_value())
            throw std::logic_error("Expected::error() called on a value");
        return std::get<1>(data_);
    }

private:
    Expected() = default;  // используется только в fail()

    // index 0 == значение (T), index 1 == ошибка (E).
    std::variant<T, E> data_{};
};
