// database.hpp — ядро MiniDB: потокобезопасное хранилище ключ→Value с TTL и персистентностью.
//
// Это центральный «контракт» капстоуна. РЕАЛИЗАЦИЯ — в src/database.cpp (сейчас стабы).
// Сигнатуры здесь МЕНЯТЬ НЕЛЬЗЯ: tests/test_database.cpp и test_concurrency.cpp зовут ровно их.
//
// Здесь сходится почти весь курс:
//   - Value поверх std::variant ....... модули 07/09
//   - умные указатели / RAII .......... модули 04/06 (lock_guard — тоже RAII!)
//   - move-семантика .................. модуль 05 (значения передаются по значению + std::move)
//   - STL-контейнеры/алгоритмы ........ модуль 08 (map, vector, sort, optional)
//   - обработка ошибок ................ модуль 09 (optional для «нет ключа», исключения у Value)
//   - МНОГОПОТОЧНОСТЬ ................. модуль 14 (std::mutex лочит каждый публичный метод)

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "value.hpp"

namespace minidb {

class Database {
public:
    // Часы БД — функция, возвращающая «текущее время» в СЕКУНДАХ (Unix-время).
    // Зачем инъекция времени, а не прямой вызов std::chrono внутри: тесты на TTL должны быть
    // ДЕТЕРМИНИРОВАНЫ. Подсунув лямбду-clock, тест двигает «сейчас» руками и проверяет протухание
    // без реального ожидания. Пустой clock (по умолчанию) = реальные системные часы.
    using Clock = std::function<std::int64_t()>;

    explicit Database(Clock clock = {});

    // ============================ strings ============================
    // SET key value — записать строковое значение (перезаписывает любой прежний тип; TTL сбрасывается).
    void set(const std::string& key, std::string value);
    // GET key — строка, если ключ жив И это String; иначе std::nullopt.
    std::optional<std::string> get(const std::string& key);

    // ============================= lists =============================
    // LPUSH key value — вставить В НАЧАЛО списка (создаёт список при отсутствии ключа).
    // Возвращает новую длину списка.
    std::size_t lpush(const std::string& key, std::string value);
    // LRANGE key start stop — срез [start, stop] ВКЛЮЧИТЕЛЬНО. Индексы могут быть
    // отрицательными (-1 = последний, -2 = предпоследний и т.д.). Выход за границы — обрезается.
    std::vector<std::string> lrange(const std::string& key, long start, long stop);
    // LLEN key — длина списка (0, если ключа нет).
    std::size_t llen(const std::string& key);

    // ============================= hashes ============================
    // HSET key field value — записать поле хеша. Возвращает true, если поле НОВОЕ (его не было),
    // false — если перезаписали существующее. Создаёт хеш при отсутствии ключа.
    bool hset(const std::string& key, const std::string& field, std::string value);
    // HGET key field — значение поля, если ключ жив, это Hash и поле есть; иначе std::nullopt.
    std::optional<std::string> hget(const std::string& key, const std::string& field);
    // HKEYS key — имена полей хеша, ОТСОРТИРОВАННЫЕ по возрастанию (пусто, если ключа нет).
    std::vector<std::string> hkeys(const std::string& key);

    // ============================ generic ============================
    // DEL key — удалить ключ. true, если он существовал (и был жив).
    bool del(const std::string& key);
    // TYPE key — тип значения, если ключ жив; иначе std::nullopt.
    std::optional<Type> type(const std::string& key);
    // KEYS — все ЖИВЫЕ ключи, ОТСОРТИРОВАННЫЕ по возрастанию.
    std::vector<std::string> keys();
    // SIZE — количество живых ключей.
    std::size_t size();

    // ============================= expiry ============================
    // EXPIRE key seconds — поставить TTL: ключ протухнет через seconds секунд от «сейчас».
    // Возвращает true, если ключ существует (и жив). seconds<=0 допустимо (мгновенное протухание).
    bool expire(const std::string& key, std::int64_t seconds);
    // TTL key — сколько секунд осталось жить:
    //   >=0  — осталось ровно столько,
    //   -1   — ключ есть, но БЕЗ TTL (живёт вечно),
    //   -2   — ключа нет (или уже протух).
    std::int64_t ttl(const std::string& key);
    // PERSIST key — снять TTL (сделать вечным). true, если ключ есть и TTL был снят.
    bool persist(const std::string& key);

    // =========================== persistence =========================
    // SAVE path — сохранить ВСЮ базу (живые ключи, их значения и TTL) в файл. true при успехе.
    // Формат — твой (текстовый/построчный — проще всего); важно лишь, чтобы load() его читал.
    bool save(const std::string& path);
    // LOAD path — заменить содержимое базы данными из файла. true при успехе.
    // Протухшие на момент загрузки ключи можно не загружать.
    bool load(const std::string& path);

private:
    // ----- внутреннее представление одной записи -----
    struct Entry {
        Value value;
        // Момент протухания в секундах Unix-времени; std::nullopt = без TTL (вечный).
        std::optional<std::int64_t> expire_at;
    };

    // Текущее «сейчас» в секундах (через clock_).
    std::int64_t now() const;
    // Протух ли ключ к моменту now (учитывая expire_at). Вспомогательная, БЕЗ блокировки мьютекса.
    bool expired(const Entry& e) const;

    // mutable: now()/const-методы должны уметь захватывать мьютекс, оставаясь логически «const».
    mutable std::mutex mutex_;
    std::map<std::string, Entry> data_;
    Clock clock_;
};

}  // namespace minidb
