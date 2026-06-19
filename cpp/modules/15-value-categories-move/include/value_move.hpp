#pragma once
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

// =============================================================================
// Модуль 15 — Категории значений и move-семантика.
// Весь код — шаблонный/inline, поэтому он целиком живёт в этом заголовке.
// Реализуй TODO прямо здесь. Заготовки уже компилируются (возвращают «не то»
// или бросают исключение), а тесты на них КРАСНЫЕ — сделай их зелёными.
// =============================================================================

namespace vm {

// -----------------------------------------------------------------------------
// Задание 1. my_move — собственный аналог std::move.
//
// my_move(x) обязан вернуть x как xvalue: тип результата — это T БЕЗ ссылки,
// снабжённый && (rvalue-ссылка). То есть для аргумента типа U& или U&& результат
// имеет тип U&&. Никакого копирования и перемещения объекта тут не происходит —
// это чистый static_cast, меняющий только КАТЕГОРИЮ значения.
//
// Подсказка по сигнатуре: параметр — это forwarding-ссылка T&&, а тип возврата
// нужно собрать из std::remove_reference_t<T> и &&.
// -----------------------------------------------------------------------------
template <class T>
std::remove_reference_t<T>&& my_move(T&& value) noexcept {
    // Эталонный ответ: один static_cast меняет категорию value на xvalue,
    // ссылаясь на тот же объект (ничего не копируя и не перемещая).
    using Bare = std::remove_reference_t<T>;
    return static_cast<Bare&&>(value);
}

// -----------------------------------------------------------------------------
// Задание 5. is_lvalue — трейт «является ли тип lvalue-ссылкой».
//
// Аналог std::is_lvalue_reference: is_lvalue<T>::value == true ровно тогда,
// когда T — это U& (но НЕ U&& и не значение U). Реализуй через частичную
// специализацию шаблона (базовый случай + специализация для T&).
//
// Заготовка наследуется от std::false_type для ВСЕХ типов — значит is_lvalue<int&>
// сейчас тоже false, и тест на это сломается.
// -----------------------------------------------------------------------------
template <class T>
struct is_lvalue : std::false_type {};

// Эталонный ответ: частичная специализация ловит ровно lvalue-ссылочные типы U&.
template <class U>
struct is_lvalue<U&> : std::true_type {};

// Удобный helper-переменная (как у стандартных трейтов: std::is_lvalue_reference_v).
template <class T>
inline constexpr bool is_lvalue_v = is_lvalue<T>::value;

// -----------------------------------------------------------------------------
// Задание 4. make_in_place — фабрика с perfect forwarding.
//
// make_in_place<T>(args...) конструирует объект T, ИДЕАЛЬНО пробрасывая каждый
// аргумент в конструктор T: lvalue остаётся lvalue, rvalue остаётся rvalue,
// const сохраняется. Это значит, что временные объекты будут перемещены
// в T, а именованные — скопированы. Используй пакет параметров Args&&... и
// std::forward.
//
// Заготовка игнорирует аргументы и конструирует T по умолчанию (T{}), теряя
// перемещаемость — тест на счётчики копий/мувов это обнаружит.
// -----------------------------------------------------------------------------
template <class T, class... Args>
T make_in_place(Args&&... args) {
    // Эталонный ответ: perfect forwarding каждого аргумента в конструктор T.
    return T(std::forward<Args>(args)...);
}

// -----------------------------------------------------------------------------
// Задание 2. ScopedResource — move-only владелец целочисленного «хэндла».
//
// Семантика как у std::unique_ptr, но вместо указателя — int handle.
//   * Конструктор от int захватывает хэндл.
//   * Копирование ЗАПРЕЩЕНО (= delete).
//   * Перемещение «крадёт» хэндл у источника и оставляет источник в пустом
//     состоянии (handle == kEmpty). Перемещение помечено noexcept — это важно
//     (см. теорию: контейнеры используют move только если он noexcept).
//   * get() возвращает текущий хэндл; valid() == (handle != kEmpty).
//   * release() отдаёт хэндл наружу и обнуляет себя (как unique_ptr::release).
//
// Пустое состояние обозначается значением kEmpty (-1).
// -----------------------------------------------------------------------------
class ScopedResource {
public:
    static constexpr int kEmpty = -1;

    ScopedResource() noexcept : handle_(kEmpty) {}

    explicit ScopedResource(int handle) noexcept : handle_(handle) {}

    // Копирование запрещено — тип move-only.
    ScopedResource(const ScopedResource&) = delete;
    ScopedResource& operator=(const ScopedResource&) = delete;

    // Перемещение: укради хэндл и обнули источник.
    ScopedResource(ScopedResource&& other) noexcept : handle_(other.handle_) {
        // Эталонный ответ: украсть хэндл и обнулить источник.
        other.handle_ = kEmpty;
    }

    ScopedResource& operator=(ScopedResource&& other) noexcept {
        // Эталонный ответ: защита от самоприсваивания, затем перенос хэндла.
        if (this != &other) {
            handle_ = other.handle_;
            other.handle_ = kEmpty;
        }
        return *this;
    }

    int get() const noexcept { return handle_; }

    bool valid() const noexcept {
        // Эталонный ответ: непустой хэндл означает валидность.
        return handle_ != kEmpty;
    }

    // Отдать хэндл наружу, себя оставить пустым.
    int release() noexcept {
        // Эталонный ответ: вернуть текущий хэндл, обнулив себя.
        int handle = handle_;
        handle_ = kEmpty;
        return handle;
    }

private:
    int handle_;
};

// -----------------------------------------------------------------------------
// Задание 3. CopyMoveCounter — диагностический тип, считающий копии и мувы.
//
// Это «зонд»: каждый раз, когда объект КОПИРУЕТСЯ, увеличивается общий счётчик
// копирований; каждый раз, когда ПЕРЕМЕЩАЕТСЯ — счётчик перемещений. Счётчики
// статические (общие на все экземпляры), чтобы тест мог их прочитать.
//
// reset() обнуляет оба счётчика перед сценарием. Move-операции noexcept.
//
// Тебе нужно правильно ИНКРЕМЕНТИРОВАТЬ счётчики в copy-ctor, copy-assign,
// move-ctor и move-assign. В заготовке инкрементов нет — счётчики всегда 0,
// и тесты «после std::move ожидаем 1 мув» провалятся.
// -----------------------------------------------------------------------------
class CopyMoveCounter {
public:
    CopyMoveCounter() = default;

    CopyMoveCounter(const CopyMoveCounter&) {
        ++copies_;  // Эталонный ответ.
    }

    CopyMoveCounter& operator=(const CopyMoveCounter&) {
        ++copies_;  // Эталонный ответ.
        return *this;
    }

    CopyMoveCounter(CopyMoveCounter&&) noexcept {
        ++moves_;  // Эталонный ответ.
    }

    CopyMoveCounter& operator=(CopyMoveCounter&&) noexcept {
        ++moves_;  // Эталонный ответ.
        return *this;
    }

    static int copies() noexcept { return copies_; }
    static int moves() noexcept { return moves_; }
    static void reset() noexcept {
        copies_ = 0;
        moves_ = 0;
    }

private:
    inline static int copies_ = 0;
    inline static int moves_ = 0;
};

}  // namespace vm
