// database.cpp — РЕАЛИЗАЦИЯ Database. Сейчас СТАБЫ: компилируется, но логика не написана,
// поэтому tests/test_database.cpp и test_concurrency.cpp стартуют КРАСНЫМИ.
//
// ГЛАВНОЕ ПРАВИЛО ПОТОКОБЕЗОПАСНОСТИ (модуль 14): КАЖДЫЙ публичный метод первым делом берёт
// мьютекс через RAII —  std::lock_guard<std::mutex> lock(mutex_);  — и держит его до конца метода.
// Тогда параллельные lpush/set с разных потоков не устроят гонку данных. ВНИМАНИЕ на рекурсию:
// если один публичный метод зовёт другой публичный, оба попытаются взять тот же мьютекс →
// дедлок. Решение — вынести «сырую» логику в приватные хелперы БЕЗ блокировки и звать их.
//
// Истёкшие ключи считаются ОТСУТСТВУЮЩИМИ (ленивое протухание): прежде чем работать с записью,
// проверяй expired(e) по now(); протухшую можно тут же стереть из data_.
//
// Что задействуется: Value/variant (07/09), map/vector/optional/sort (08), move (05),
// ошибки (09), mutex (14), файловый ввод-вывод для save/load.

#include "database.hpp"

#include <chrono>

namespace minidb {

Database::Database(Clock clock) : clock_(std::move(clock)) {
    // TODO: если clock_ пустой — подставить реальные системные часы (секунды Unix-времени),
    // например через std::chrono::system_clock. Стаб этого НЕ делает, поэтому now() ниже
    // вернёт 0 и TTL поедет — это надо починить.
}

std::int64_t Database::now() const {
    // TODO: вернуть clock_() если он валиден. Стаб всегда 0.
    return 0;
}

bool Database::expired(const Entry& /*e*/) const {
    // TODO: true, если у записи есть expire_at и он <= now(). Стаб «никогда не протухает».
    return false;
}

// ------------------------------ strings ------------------------------
void Database::set(const std::string& /*key*/, std::string /*value*/) {
    // TODO: lock; записать Value::make_string(value), сбросить TTL. Стаб ничего не делает.
}

std::optional<std::string> Database::get(const std::string& /*key*/) {
    // TODO: lock; вернуть строку, если ключ жив и это String. Стаб всегда «нет ключа».
    return std::nullopt;
}

// ------------------------------- lists -------------------------------
std::size_t Database::lpush(const std::string& /*key*/, std::string /*value*/) {
    // TODO: lock; вставить в начало списка (создав его при необходимости), вернуть новую длину.
    return 0;
}

std::vector<std::string> Database::lrange(const std::string& /*key*/, long /*start*/, long /*stop*/) {
    // TODO: lock; срез [start,stop] включительно, с поддержкой отрицательных индексов.
    return {};
}

std::size_t Database::llen(const std::string& /*key*/) {
    // TODO: lock; длина списка (0, если нет/не список).
    return 0;
}

// ------------------------------- hashes ------------------------------
bool Database::hset(const std::string& /*key*/, const std::string& /*field*/, std::string /*value*/) {
    // TODO: lock; записать поле, вернуть true если поле было НОВЫМ. Стаб всегда false.
    return false;
}

std::optional<std::string> Database::hget(const std::string& /*key*/, const std::string& /*field*/) {
    // TODO: lock; значение поля или nullopt.
    return std::nullopt;
}

std::vector<std::string> Database::hkeys(const std::string& /*key*/) {
    // TODO: lock; отсортированные имена полей.
    return {};
}

// ------------------------------ generic ------------------------------
bool Database::del(const std::string& /*key*/) {
    // TODO: lock; удалить ключ, вернуть true если он был живым.
    return false;
}

std::optional<Type> Database::type(const std::string& /*key*/) {
    // TODO: lock; тип значения живого ключа или nullopt.
    return std::nullopt;
}

std::vector<std::string> Database::keys() {
    // TODO: lock; отсортированный список живых ключей.
    return {};
}

std::size_t Database::size() {
    // TODO: lock; количество живых ключей.
    return 0;
}

// ------------------------------- expiry ------------------------------
bool Database::expire(const std::string& /*key*/, std::int64_t /*seconds*/) {
    // TODO: lock; поставить expire_at = now()+seconds, если ключ есть. Вернуть факт наличия ключа.
    return false;
}

std::int64_t Database::ttl(const std::string& /*key*/) {
    // TODO: lock; >=0 осталось / -1 без TTL / -2 нет ключа. Стаб всегда -2.
    return -2;
}

bool Database::persist(const std::string& /*key*/) {
    // TODO: lock; снять TTL, вернуть true если он был.
    return false;
}

// ---------------------------- persistence ----------------------------
bool Database::save(const std::string& /*path*/) {
    // TODO: lock; сериализовать живые ключи (значение + TTL) в файл. Стаб «не сохранил».
    return false;
}

bool Database::load(const std::string& /*path*/) {
    // TODO: lock; прочитать файл и заменить содержимое базы. Стаб «не загрузил».
    return false;
}

}  // namespace minidb
