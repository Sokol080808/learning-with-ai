#pragma once
#include <memory>
#include <utility>
#include <typeinfo>
#include <stdexcept>
#include <variant>
#include <string>

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
            // TODO: вызвать сохранённый f_ с переданными аргументами
            //       и вернуть результат (с учётом R == void).
            throw std::logic_error("TODO: Function::Model::invoke");
        }
    };

    std::unique_ptr<Concept> self_;

public:
    Function() = default;

    // Принимаем любой вызываемый объект и стираем его тип.
    template <class F>
    Function(F f) {
        // TODO: создать Model<F> и положить в self_.
        (void)f;
    }

    // Есть ли внутри сохранённый вызываемый объект.
    explicit operator bool() const {
        // TODO
        return false;
    }

    // Вызов. Если внутри пусто — бросить std::bad_function_call.
    R operator()(Args... args) const {
        // TODO: проверить self_, иначе бросить; иначе делегировать invoke.
        throw std::logic_error("TODO: Function::operator()");
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
            // TODO: вернуть typeid(T)
            throw std::logic_error("TODO: Any::Model::type");
        }
        std::unique_ptr<Concept> clone() const override {
            // TODO: глубокая копия — новая Model<T> с тем же значением
            throw std::logic_error("TODO: Any::Model::clone");
        }
    };

    std::unique_ptr<Concept> self_;

    // any_cast нужен доступ к Model<T>, чтобы достать value_.
    template <class T>
    friend T any_cast(const Any& a);

public:
    Any() = default;

    template <class T>
    Any(T v) {
        // TODO: завернуть значение в Model<T>
        (void)v;
    }

    // Корректное копирование: Any владеет указателем, копия должна быть
    // независимой. Используй clone().
    Any(const Any& other) {
        // TODO
        (void)other;
    }
    Any& operator=(const Any& other) {
        // TODO
        (void)other;
        return *this;
    }

    Any(Any&&) noexcept = default;
    Any& operator=(Any&&) noexcept = default;

    bool has_value() const {
        // TODO
        return false;
    }

    // typeid хранимого типа; если пусто — typeid(void).
    const std::type_info& type() const {
        // TODO
        return typeid(void);
    }
};

// Возвращает копию хранимого значения, если тип совпадает с T.
// Иначе (и если Any пуст) — бросает std::bad_cast.
template <class T>
T any_cast(const Any& a) {
    // TODO: сравнить a.type() с typeid(T); при совпадении достать value_
    //       из Any::Model<T>; иначе throw std::bad_cast{}.
    (void)a;
    throw std::logic_error("TODO: any_cast");
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
    // TODO: std::visit с overloaded{...} по двум альтернативам.
    (void)s;
    throw std::logic_error("TODO: area");
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
            // TODO: вызвать obj_.draw()
            throw std::logic_error("TODO: Drawable::Model::draw");
        }
    };

    std::unique_ptr<Concept> self_;

public:
    template <class T>
    Drawable(T obj) {
        // TODO: завернуть obj в Model<T>
        (void)obj;
    }

    std::string draw() const {
        // TODO: делегировать self_->draw()
        throw std::logic_error("TODO: Drawable::draw");
    }
};
