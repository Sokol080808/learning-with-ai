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
    // TODO: верни value, приведённый к rvalue-ссылке через static_cast.
    // Сейчас заготовка возвращает ССЫЛКУ НА ЧУЖОЙ (статический) объект, а не на
    // твой value — это не перемещение исходного значения, и тест это поймает.
    using Bare = std::remove_reference_t<T>;
    static Bare placeholder{};  // заглушка: «не тот» объект
    (void)value;
    return static_cast<Bare&&>(placeholder);
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
struct is_lvalue : std::false_type {
    // TODO: добавь частичную специализацию `template <class U> struct is_lvalue<U&>`,
    // унаследованную от std::true_type, чтобы для ссылочных типов давала true.
};

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
    // TODO: верни T(std::forward<Args>(args)...)
    (void)sizeof...(args);
    throw std::logic_error("TODO: реализуй make_in_place через perfect forwarding");
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
    ScopedResource(ScopedResource&& other) noexcept {
        // TODO: забрать other.handle_ себе, а other оставить пустым (kEmpty).
        handle_ = kEmpty;  // заглушка теряет хэндл источника — тест это ловит.
        (void)other;
    }

    ScopedResource& operator=(ScopedResource&& other) noexcept {
        // TODO: защита от самоприсваивания; забрать чужой хэндл; обнулить other.
        (void)other;  // заглушка ничего не переносит.
        return *this;
    }

    int get() const noexcept { return handle_; }

    bool valid() const noexcept {
        // TODO: true, если хэндл не пустой.
        return false;
    }

    // Отдать хэндл наружу, себя оставить пустым.
    int release() noexcept {
        // TODO: вернуть текущий handle_, предварительно сбросив себя в kEmpty.
        return kEmpty;
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
        // TODO: ++copies_
    }

    CopyMoveCounter& operator=(const CopyMoveCounter&) {
        // TODO: ++copies_
        return *this;
    }

    CopyMoveCounter(CopyMoveCounter&&) noexcept {
        // TODO: ++moves_
    }

    CopyMoveCounter& operator=(CopyMoveCounter&&) noexcept {
        // TODO: ++moves_
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
