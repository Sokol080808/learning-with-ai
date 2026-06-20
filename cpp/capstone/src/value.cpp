// value.cpp — РЕАЛИЗАЦИЯ Value. Сейчас это СТАБЫ: код компилируется, но логика не написана,
// поэтому тесты (tests/test_value.cpp) стартуют КРАСНЫМИ. Твоя задача — заменить тела на рабочие.
//
// Подсказки по реализации (детали — спрашивай меня):
//   - make_string/make_list/make_hash просто оборачивают нужную альтернативу variant.
//   - type(): index() у variant совпадает с порядком альтернатив; сопоставь его с enum Type.
//   - as_xxx(): std::get_if<T>(&data_) вернёт указатель или nullptr; на nullptr — throw
//     std::runtime_error("...") (модуль 09). std::get<T> кидает std::bad_variant_access — тоже
//     вариант, но своё сообщение полезнее для отладки.

#include "value.hpp"

#include <stdexcept>

namespace minidb {

Value::Value(Storage data) : data_(std::move(data)) {}

// Фабрики оборачивают нужную альтернативу variant. Аргумент s принят ПО ЗНАЧЕНИЮ
// и перемещён внутрь (модуль 05): один move вместо копии — данные «утекают» в variant
// без лишнего выделения памяти.
Value Value::make_string(std::string s) {
    return Value(Storage{std::move(s)});
}

Value Value::make_list() {
    return Value(Storage{ListData{}});
}

Value Value::make_hash() {
    return Value(Storage{HashData{}});
}

// index() variant — это номер активной альтернативы (String=0, List=1, Hash=2).
// Порядок в Storage намеренно совпадает с enum Type, поэтому отображение прямое.
Type Value::type() const {
    return static_cast<Type>(data_.index());
}

// «Бросающие аксессоры» (модуль 09). std::get_if<T>(&data_) вернёт указатель на T,
// если активна именно эта альтернатива, иначе nullptr — тогда бросаем runtime_error
// с понятным сообщением (полезнее, чем стандартное std::bad_variant_access от std::get).
std::string& Value::as_string() {
    if (auto* p = std::get_if<std::string>(&data_)) return *p;
    throw std::runtime_error("Value::as_string: value is not a string");
}
const std::string& Value::as_string() const {
    if (auto* p = std::get_if<std::string>(&data_)) return *p;
    throw std::runtime_error("Value::as_string: value is not a string");
}

ListData& Value::as_list() {
    if (auto* p = std::get_if<ListData>(&data_)) return *p;
    throw std::runtime_error("Value::as_list: value is not a list");
}
const ListData& Value::as_list() const {
    if (auto* p = std::get_if<ListData>(&data_)) return *p;
    throw std::runtime_error("Value::as_list: value is not a list");
}

HashData& Value::as_hash() {
    if (auto* p = std::get_if<HashData>(&data_)) return *p;
    throw std::runtime_error("Value::as_hash: value is not a hash");
}
const HashData& Value::as_hash() const {
    if (auto* p = std::get_if<HashData>(&data_)) return *p;
    throw std::runtime_error("Value::as_hash: value is not a hash");
}

}  // namespace minidb
