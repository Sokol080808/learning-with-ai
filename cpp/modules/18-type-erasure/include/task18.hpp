#pragma once
#include <memory>
#include <utility>
#include <typeinfo>
#include <stdexcept>
#include <variant>
#include <string>
#include <functional>  // std::bad_function_call

// =====================================================================
// Модуль 18 — Стирание типов (type erasure)
//
// Весь код шаблонов живёт здесь, в заголовке: компилятору нужно видеть
// тело шаблона в точке использования. Реализуй задания прямо в этом файле.
// =====================================================================


// ---------------------------------------------------------------------
// Задание 1. Function<R(Args...)> — минимальный аналог std::function.
//
// Хранит ЛЮБОЙ вызываемый объект (лямбду, указатель на функцию, объект
// с operator()) с сигнатурой R(Args...). Вызывается через operator().
// Внутри — приём «концепт + модель»: абстрактный базовый Concept с
// чисто виртуальным invoke(), и шаблонная Model<F>, которая помнит
// конкретный тип F за указателем на базу.
// ---------------------------------------------------------------------
template <class Signature>
class Function;  // объявляем основной шаблон, специализируем ниже

template <class R, class... Args>
class Function<R(Args...)> {
    // «Концепт» — общий интерфейс, ничего не знающий о конкретном типе.
    struct Concept {
        virtual ~Concept() = default;
        virtual R invoke(Args... args) const = 0;
    };

    // «Модель» — шаблон, который оборачивает конкретный вызываемый F.
    template <class F>
    struct Model : Concept {
        F f_;
        explicit Model(F f) : f_(std::move(f)) {}
        R invoke(Args... args) const override {
            // Эталонный ответ: вызвать f_; return с выражением типа void допустим.
            return f_(std::forward<Args>(args)...);
        }
    };

    std::unique_ptr<Concept> self_;

public:
    Function() = default;

    // Принимаем любой вызываемый объект и стираем его тип.
    template <class F>
    Function(F f) : self_(std::make_unique<Model<F>>(std::move(f))) {}

    // Есть ли внутри сохранённый вызываемый объект.
    explicit operator bool() const {
        return self_ != nullptr;
    }

    // Вызов. Если внутри пусто — бросить std::bad_function_call.
    R operator()(Args... args) const {
        if (!self_) throw std::bad_function_call{};
        return self_->invoke(std::forward<Args>(args)...);
    }
};


// ---------------------------------------------------------------------
// Задание 2. Any — хранилище значения ЛЮБОГО типа (аналог std::any).
//
// has_value() — есть ли что-то внутри; type() — typeid хранимого типа.
// Свободная функция any_cast<T>(a) возвращает копию значения, если тип
// совпадает; иначе бросает std::bad_cast.
// ---------------------------------------------------------------------
class Any {
    struct Concept {
        virtual ~Concept() = default;
        virtual const std::type_info& type() const = 0;
        virtual std::unique_ptr<Concept> clone() const = 0;
    };

    template <class T>
    struct Model : Concept {
        T value_;
        explicit Model(T v) : value_(std::move(v)) {}
        const std::type_info& type() const override {
            return typeid(T);
        }
        std::unique_ptr<Concept> clone() const override {
            return std::make_unique<Model<T>>(value_);
        }
    };

    std::unique_ptr<Concept> self_;

    // any_cast нужен доступ к Model<T>, чтобы достать value_.
    template <class T>
    friend T any_cast(const Any& a);

public:
    Any() = default;

    template <class T>
    Any(T v) : self_(std::make_unique<Model<T>>(std::move(v))) {}

    // Корректное копирование: Any владеет указателем, копия должна быть
    // независимой. Используй clone().
    Any(const Any& other)
        : self_(other.self_ ? other.self_->clone() : nullptr) {}
    Any& operator=(const Any& other) {
        Any tmp(other);          // copy-and-swap
        std::swap(self_, tmp.self_);
        return *this;
    }

    Any(Any&&) noexcept = default;
    Any& operator=(Any&&) noexcept = default;

    bool has_value() const {
        return self_ != nullptr;
    }

    // typeid хранимого типа; если пусто — typeid(void).
    const std::type_info& type() const {
        return self_ ? self_->type() : typeid(void);
    }
};

// Возвращает копию хранимого значения, если тип совпадает с T.
// Иначе (и если Any пуст) — бросает std::bad_cast.
template <class T>
T any_cast(const Any& a) {
    if (a.self_ && a.type() == typeid(T)) {
        return static_cast<const Any::Model<T>*>(a.self_.get())->value_;
    }
    throw std::bad_cast{};
}


// ---------------------------------------------------------------------
// Задание 3. overloaded-визитор для std::variant.
//
// overloaded склеивает несколько лямбд в один объект с перегруженным
// operator(). Вместе с std::visit это даёт типобезопасный «pattern
// matching» по альтернативам variant.
//
// Фигуры заданы; реализуй свободную функцию area(const Shape&), которая
// через std::visit + overloaded считает площадь.
// ---------------------------------------------------------------------
struct Circle { double r; };
struct Square { double side; };

using Shape = std::variant<Circle, Square>;

// overloaded: наследуемся от всех лямбд и подтягиваем их operator().
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Площадь фигуры: круг — pi*r*r, квадрат — side*side.
// Используй число pi = 3.141592653589793.
inline double area(const Shape& s) {
    constexpr double pi = 3.141592653589793;
    return std::visit(overloaded{
        [](const Circle& c) -> double { return pi * c.r * c.r; },
        [](const Square& sq) -> double { return sq.side * sq.side; },
    }, s);
}


// ---------------------------------------------------------------------
// Задание 4. Drawable — type-erased обёртка над «чем угодно с draw()».
//
// Хранит любой объект, у которого есть метод std::string draw() const,
// БЕЗ общего базового класса у этих объектов. Свой draw() делегирует
// вызов спрятанному объекту.
// ---------------------------------------------------------------------
class Drawable {
    struct Concept {
        virtual ~Concept() = default;
        virtual std::string draw() const = 0;
    };

    template <class T>
    struct Model : Concept {
        T obj_;
        explicit Model(T o) : obj_(std::move(o)) {}
        std::string draw() const override {
            return obj_.draw();
        }
    };

    std::unique_ptr<Concept> self_;

public:
    template <class T>
    Drawable(T obj) : self_(std::make_unique<Model<T>>(std::move(obj))) {}

    std::string draw() const {
        return self_->draw();
    }
};
