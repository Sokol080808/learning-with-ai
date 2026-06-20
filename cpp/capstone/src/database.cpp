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

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>

namespace minidb {

// ======================================================================
// ДИСЦИПЛИНА БЛОКИРОВКИ (модуль 14)
// ----------------------------------------------------------------------
// Каждый ПУБЛИЧНЫЙ метод первым делом берёт mutex_ через RAII-обёртку
// std::lock_guard. Дальше он работает с data_ напрямую — и НИКОГДА не зовёт
// другой публичный метод (тот попытался бы взять тот же НЕрекурсивный мьютекс
// под уже захваченным → дедлок). Вся повторно используемая логика, которой
// нужен доступ к data_ под уже взятым замком, вынесена в приватные хелперы
// БЕЗ собственной блокировки (now(), expired(), а внутри методов — поиск
// записи). Этот шаблон «public lock → private no-lock» — стандартный способ
// сделать класс потокобезопасным без рекурсивного мьютекса.
// ======================================================================

Database::Database(Clock clock) : clock_(std::move(clock)) {
    // Если пользователь не дал часы — берём реальные системные в СЕКУНДАХ Unix-времени.
    // Инъекция позволяет тестам управлять «сейчас» вручную и проверять TTL без sleep.
    if (!clock_) {
        clock_ = [] {
            using namespace std::chrono;
            return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        };
    }
}

std::int64_t Database::now() const {
    return clock_();
}

bool Database::expired(const Entry& e) const {
    // Протухла, если задан срок и текущий момент достиг его (>=). Момент expire_at
    // считается уже истёкшим — тест проверяет ровно равенство now == expire_at.
    return e.expire_at.has_value() && now() >= *e.expire_at;
}

// ------------------------------ strings ------------------------------
void Database::set(const std::string& key, std::string value) {
    std::lock_guard<std::mutex> lock(mutex_);
    // insert_or_assign перезапишет любой прежний тип; новый Entry без expire_at = TTL сброшен.
    // Берём именно его, а НЕ operator[]: у Entry нет конструктора по умолчанию (Value его не
    // имеет), а operator[] требует default-construct перед присваиванием.
    data_.insert_or_assign(key, Entry{Value::make_string(std::move(value)), std::nullopt});
}

std::optional<std::string> Database::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    if (expired(it->second)) {
        data_.erase(it);  // ленивое протухание: чистим прямо при обращении
        return std::nullopt;
    }
    if (it->second.value.type() != Type::String) return std::nullopt;
    return it->second.value.as_string();
}

// ------------------------------- lists -------------------------------
std::size_t Database::lpush(const std::string& key, std::string value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end() && expired(it->second)) {
        data_.erase(it);
        it = data_.end();
    }
    if (it == data_.end()) {
        // ключа нет — создаём пустой список
        it = data_.emplace(key, Entry{Value::make_list(), std::nullopt}).first;
    }
    // Если ключ занят другим типом — это «ошибка типа»; здесь сознательно НЕ кидаем
    // наружу, а перезаписываем на список (простая семантика капстоуна). Тесты не
    // смешивают типы под одним ключом, так что поведение детерминировано.
    if (it->second.value.type() != Type::List) {
        it->second = Entry{Value::make_list(), std::nullopt};
    }
    auto& lst = it->second.value.as_list();
    lst.insert(lst.begin(), std::move(value));  // LPUSH = вставка В НАЧАЛО
    return lst.size();
}

std::vector<std::string> Database::lrange(const std::string& key, long start, long stop) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return {};
    }
    if (it->second.value.type() != Type::List) return {};
    const auto& lst = it->second.value.as_list();
    const long n = static_cast<long>(lst.size());
    if (n == 0) return {};

    // Отрицательные индексы отсчитываются с конца (-1 = последний).
    if (start < 0) start += n;
    if (stop < 0) stop += n;
    // Обрезаем к допустимому диапазону [0, n-1] — выход за границы не падает.
    if (start < 0) start = 0;
    if (stop >= n) stop = n - 1;
    if (start > stop) return {};

    std::vector<std::string> out;
    out.reserve(static_cast<std::size_t>(stop - start + 1));
    for (long i = start; i <= stop; ++i) out.push_back(lst[static_cast<std::size_t>(i)]);
    return out;
}

std::size_t Database::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return 0;
    }
    if (it->second.value.type() != Type::List) return 0;
    return it->second.value.as_list().size();
}

// ------------------------------- hashes ------------------------------
bool Database::hset(const std::string& key, const std::string& field, std::string value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it != data_.end() && expired(it->second)) {
        data_.erase(it);
        it = data_.end();
    }
    if (it == data_.end()) {
        it = data_.emplace(key, Entry{Value::make_hash(), std::nullopt}).first;
    }
    if (it->second.value.type() != Type::Hash) {
        it->second = Entry{Value::make_hash(), std::nullopt};
    }
    auto& h = it->second.value.as_hash();
    const bool is_new = (h.find(field) == h.end());
    h[field] = std::move(value);
    return is_new;  // true только если поле раньше отсутствовало
}

std::optional<std::string> Database::hget(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return std::nullopt;
    }
    if (it->second.value.type() != Type::Hash) return std::nullopt;
    const auto& h = it->second.value.as_hash();
    auto fit = h.find(field);
    if (fit == h.end()) return std::nullopt;
    return fit->second;
}

std::vector<std::string> Database::hkeys(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return {};
    }
    if (it->second.value.type() != Type::Hash) return {};
    // std::map уже хранит ключи отсортированными, поэтому обход даёт возрастающий порядок.
    std::vector<std::string> out;
    const auto& h = it->second.value.as_hash();
    out.reserve(h.size());
    for (const auto& [f, _] : h) out.push_back(f);
    return out;
}

// ------------------------------ generic ------------------------------
bool Database::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end()) return false;
    const bool was_alive = !expired(it->second);
    data_.erase(it);
    return was_alive;  // протухший ключ «и так отсутствовал» → false
}

std::optional<Type> Database::type(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return std::nullopt;
    }
    return it->second.value.type();
}

std::vector<std::string> Database::keys() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> out;
    // Собираем живые ключи; протухшие попутно вычищаем (erase по итератору в цикле).
    for (auto it = data_.begin(); it != data_.end();) {
        if (expired(it->second)) {
            it = data_.erase(it);
        } else {
            out.push_back(it->first);
            ++it;
        }
    }
    // std::map обходится по возрастанию ключа, так что out уже отсортирован.
    return out;
}

std::size_t Database::size() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::size_t count = 0;
    for (auto it = data_.begin(); it != data_.end();) {
        if (expired(it->second)) {
            it = data_.erase(it);
        } else {
            ++count;
            ++it;
        }
    }
    return count;
}

// ------------------------------- expiry ------------------------------
bool Database::expire(const std::string& key, std::int64_t seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return false;
    }
    it->second.expire_at = now() + seconds;  // seconds<=0 → момент в прошлом/сейчас → мгновенно протухнет
    return true;
}

std::int64_t Database::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return -2;  // ключа нет (или уже протух)
    }
    if (!it->second.expire_at.has_value()) return -1;  // есть, но вечный
    return *it->second.expire_at - now();              // сколько секунд осталось
}

bool Database::persist(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end() || expired(it->second)) {
        if (it != data_.end()) data_.erase(it);
        return false;
    }
    if (!it->second.expire_at.has_value()) return false;  // снимать нечего
    it->second.expire_at = std::nullopt;
    return true;
}

// ---------------------------- persistence ----------------------------
//
// Формат файла — простой построчный текст. Одна логическая запись = один или
// несколько токенов в строке. Чтобы значения могли содержать пробелы (тест
// сохраняет "hello world"), мы НЕ полагаемся на >> для самих данных: служебные
// поля (тип, TTL, количество элементов, длины) читаем числами/словами, а сырые
// строки данных пишем как «<длина> <байты>» — length-prefixed, что устойчиво к
// пробелам и переводам строк внутри значения.
//
//   String:  S <ttl> <klen> <key> <vlen> <value>
//   List:    L <ttl> <klen> <key> <count> (<vlen> <item>)*
//   Hash:    H <ttl> <klen> <key> <count> (<flen> <field> <vlen> <value>)*
//
// <ttl> = abs expire_at в секундах Unix или -1 (вечный). Length-prefix делает
// формат самодостаточным: парсер не зависит от расстановки пробелов.

namespace {

// Записать length-prefixed строку: "<len> <bytes>".
void write_blob(std::ostream& os, const std::string& s) {
    os << s.size() << ' ';
    os.write(s.data(), static_cast<std::streamsize>(s.size()));
}

// Прочитать length-prefixed строку. false — формат повреждён.
bool read_blob(std::istream& is, std::string& out) {
    std::size_t len = 0;
    if (!(is >> len)) return false;
    is.get();  // съесть один пробел-разделитель после длины
    out.resize(len);
    if (len > 0) is.read(&out[0], static_cast<std::streamsize>(len));
    return static_cast<std::size_t>(is.gcount()) == len || len == 0;
}

}  // namespace

bool Database::save(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ofstream os(path, std::ios::binary | std::ios::trunc);
    if (!os) return false;

    for (const auto& [key, entry] : data_) {
        if (expired(entry)) continue;  // протухшие не сохраняем
        const std::int64_t ttl_field = entry.expire_at.value_or(-1);
        const Value& v = entry.value;
        switch (v.type()) {
            case Type::String: {
                os << "S " << ttl_field << ' ';
                write_blob(os, key);
                os << ' ';
                write_blob(os, v.as_string());
                os << '\n';
                break;
            }
            case Type::List: {
                const auto& lst = v.as_list();
                os << "L " << ttl_field << ' ';
                write_blob(os, key);
                os << ' ' << lst.size();
                for (const auto& item : lst) {
                    os << ' ';
                    write_blob(os, item);
                }
                os << '\n';
                break;
            }
            case Type::Hash: {
                const auto& h = v.as_hash();
                os << "H " << ttl_field << ' ';
                write_blob(os, key);
                os << ' ' << h.size();
                for (const auto& [f, val] : h) {
                    os << ' ';
                    write_blob(os, f);
                    os << ' ';
                    write_blob(os, val);
                }
                os << '\n';
                break;
            }
        }
    }
    return static_cast<bool>(os);
}

bool Database::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ifstream is(path, std::ios::binary);
    if (!is) return false;  // нет файла → провал (тест LoadMissingFileFails)

    std::map<std::string, Entry> loaded;
    const std::int64_t cur = now();

    std::string tag;
    while (is >> tag) {
        std::int64_t ttl_field = 0;
        if (!(is >> ttl_field)) return false;
        is.get();  // пробел после ttl
        std::string key;
        if (!read_blob(is, key)) return false;

        std::optional<std::int64_t> expire_at =
            (ttl_field < 0) ? std::nullopt : std::optional<std::int64_t>(ttl_field);

        if (tag == "S") {
            is.get();
            std::string val;
            if (!read_blob(is, val)) return false;
            loaded.insert_or_assign(key, Entry{Value::make_string(std::move(val)), expire_at});
        } else if (tag == "L") {
            std::size_t count = 0;
            if (!(is >> count)) return false;
            Value v = Value::make_list();
            auto& lst = v.as_list();
            lst.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                is.get();
                std::string item;
                if (!read_blob(is, item)) return false;
                lst.push_back(std::move(item));
            }
            loaded.insert_or_assign(key, Entry{std::move(v), expire_at});
        } else if (tag == "H") {
            std::size_t count = 0;
            if (!(is >> count)) return false;
            Value v = Value::make_hash();
            auto& h = v.as_hash();
            for (std::size_t i = 0; i < count; ++i) {
                is.get();
                std::string field;
                if (!read_blob(is, field)) return false;
                is.get();
                std::string val;
                if (!read_blob(is, val)) return false;
                h[field] = std::move(val);
            }
            loaded.insert_or_assign(key, Entry{std::move(v), expire_at});
        } else {
            return false;  // неизвестный тег = повреждённый файл
        }
    }

    // Отбрасываем уже протухшие к моменту загрузки (можно не загружать).
    for (auto it = loaded.begin(); it != loaded.end();) {
        if (it->second.expire_at.has_value() && cur >= *it->second.expire_at) {
            it = loaded.erase(it);
        } else {
            ++it;
        }
    }

    data_ = std::move(loaded);  // полная замена содержимого базы
    return true;
}

}  // namespace minidb
