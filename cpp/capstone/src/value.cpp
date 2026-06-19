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

// TODO: собрать Value со строкой внутри. Стаб возвращает ПУСТУЮ строку независимо от s.
Value Value::make_string(std::string /*s*/) {
    return Value(Storage{std::string{}});
}

// TODO: собрать Value с пустым списком. (Стаб уже почти верен — но не полагайся на это, проверь тестом.)
Value Value::make_list() {
    return Value(Storage{ListData{}});
}

// TODO: собрать Value с пустым хешем.
Value Value::make_hash() {
    return Value(Storage{HashData{}});
}

// TODO: вернуть реальный тип по содержимому variant. Стаб ВСЕГДА говорит String — неверно для list/hash.
Type Value::type() const {
    return Type::String;
}

// TODO: вернуть ссылку на строку или бросить runtime_error, если внутри не строка.
// Стаб бросает всегда — даже когда внутри действительно строка.
std::string& Value::as_string() {
    throw std::runtime_error("Value::as_string не реализован (стаб)");
}
const std::string& Value::as_string() const {
    throw std::runtime_error("Value::as_string не реализован (стаб)");
}

// TODO: вернуть ссылку на список или бросить runtime_error при неверном типе.
ListData& Value::as_list() {
    throw std::runtime_error("Value::as_list не реализован (стаб)");
}
const ListData& Value::as_list() const {
    throw std::runtime_error("Value::as_list не реализован (стаб)");
}

// TODO: вернуть ссылку на хеш или бросить runtime_error при неверном типе.
HashData& Value::as_hash() {
    throw std::runtime_error("Value::as_hash не реализован (стаб)");
}
const HashData& Value::as_hash() const {
    throw std::runtime_error("Value::as_hash не реализован (стаб)");
}

}  // namespace minidb
