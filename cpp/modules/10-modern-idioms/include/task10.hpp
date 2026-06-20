#pragma once
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <functional>
#include <optional>

// Пара {минимум, максимум} для непустого вектора.
std::pair<int, int> minmax_of(const std::vector<int>& v);

// Типобезопасное перечисление цветов.
enum class Color { Red, Green, Blue };

// Имя цвета: "Red" / "Green" / "Blue".
std::string color_name(Color c);

// Квадрат на этапе компиляции. Эталонный ответ (constexpr-функции живут в заголовке).
constexpr int square_ce(int n) {
    return n * n;
}

// Чётные элементы, возведённые в квадрат (через std::views).
std::vector<int> evens_squared(const std::vector<int>& v);

// Сумма всех значений словаря.
int sum_values(const std::map<std::string, int>& m);

// --- Новые, более сложные задания ---

// Задание 6. Разбить строку на куски по разделителю sep БЕЗ копирования строки.
// Каждый кусок — std::string_view, ссылающийся внутрь исходной s.
// Пустые куски (например, два разделителя подряд или sep в начале/конце) сохраняются.
// "" -> {""}; "a" -> {"a"}; "a,b,c" -> {"a","b","c"}; ",a," -> {"", "a", ""}.
std::vector<std::string_view> split_view(std::string_view s, char sep);

// Задание 7. ScopeGuard — выполняет переданную лямбду в своём деструкторе ("finally").
// Гарантирует запуск кода при любом выходе из области видимости (return, throw, обычный
// конец блока). Можно отменить вызовом dismiss(). Некопируемый.
class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> fn) : fn_(std::move(fn)) {}

    // Запретить копирование (иначе действие выполнилось бы дважды).
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    // Отменить запланированное действие — после этого деструктор ничего не делает.
    // Эталонный ответ.
    void dismiss() noexcept {
        active_ = false;
    }

    // Эталонный ответ: вызвать действие ровно один раз, не выпуская исключений наружу.
    ~ScopeGuard() {
        if (active_ && fn_) {
            try {
                fn_();
            } catch (...) {
                // Деструктор не должен выпускать исключения наружу.
            }
        }
    }

private:
    std::function<void()> fn_;
    bool active_ = true;
};

// Удобный фабричный помощник: make_scope_guard([]{ ... }).
inline ScopeGuard make_scope_guard(std::function<void()> fn) {
    return ScopeGuard(std::move(fn));
}

// Задание 8. Поделить с остатком. [[nodiscard]] — игнорировать результат нельзя
// (иначе деление было бы зря). Возвращает пару {частное, остаток} для b != 0.
// При b == 0 бросает std::invalid_argument.
[[nodiscard]] std::pair<int, int> divmod(int a, int b);

// Задание 9. Безопасное деление через optional: a/b или nullopt при b==0.
// Альтернативный стиль обработки ошибок по сравнению с divmod (задание 8):
// вместо исключения возвращаем «пустое значение» — nullopt.
[[nodiscard]] std::optional<int> safe_divide(int a, int b);
