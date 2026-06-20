// test_database.cpp — приёмочные тесты ядра. Они ЗАДАЮТ поведение Database; пока src/database.cpp —
// стаб, все они КРАСНЫЕ. Особо обрати внимание на ДВА детерминированных приёма:
//   1) TTL проверяется через ИНЪЕКЦИЮ ЧАСОВ: общий fake_now двигается руками, без реального sleep.
//   2) Персистентность проверяется через ВРЕМЕННЫЙ файл (std::filesystem::temp_directory_path),
//      без сети и без хардкода путей.
// Эти тесты МЕНЯТЬ не нужно — меняй реализацию, пока они не позеленеют.

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "database.hpp"

using minidb::Database;
using minidb::Type;

// --- хелпер: временный путь под файл базы (детерминированно, без сети) ---
// Уникальность пути обеспечиваем id потока — без платформозависимого getpid().
static std::string temp_db_path(const std::string& tag) {
    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    auto p = std::filesystem::temp_directory_path() /
             ("minidb_" + tag + "_" + std::to_string(tid) + ".db");
    return p.string();
}

// ============================ strings ============================
TEST(Database, SetGet) {
    Database db;
    EXPECT_FALSE(db.get("missing").has_value());

    db.set("name", "Ada");
    auto v = db.get("name");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "Ada");

    db.set("name", "Grace");  // перезапись
    EXPECT_EQ(*db.get("name"), "Grace");
}

// ============================= lists =============================
TEST(Database, ListPushRangeLen) {
    Database db;
    EXPECT_EQ(db.lpush("xs", "a"), 1u);  // [a]
    EXPECT_EQ(db.lpush("xs", "b"), 2u);  // [b, a]  (push в начало!)
    EXPECT_EQ(db.lpush("xs", "c"), 3u);  // [c, b, a]
    EXPECT_EQ(db.llen("xs"), 3u);

    EXPECT_EQ(db.lrange("xs", 0, -1), (std::vector<std::string>{"c", "b", "a"}));
    EXPECT_EQ(db.lrange("xs", 0, 0), (std::vector<std::string>{"c"}));
    EXPECT_EQ(db.lrange("xs", -2, -1), (std::vector<std::string>{"b", "a"}));
    EXPECT_EQ(db.lrange("xs", 1, 1), (std::vector<std::string>{"b"}));
    // выход за границы аккуратно обрезается, не падает
    EXPECT_EQ(db.lrange("xs", 0, 100), (std::vector<std::string>{"c", "b", "a"}));
    EXPECT_TRUE(db.lrange("missing", 0, -1).empty());
}

// ============================= hashes ============================
TEST(Database, HashSetGetKeys) {
    Database db;
    EXPECT_TRUE(db.hset("h", "b", "2"));   // новое поле -> true
    EXPECT_TRUE(db.hset("h", "a", "1"));   // новое поле -> true
    EXPECT_FALSE(db.hset("h", "a", "9"));  // перезапись -> false

    EXPECT_EQ(*db.hget("h", "a"), "9");
    EXPECT_EQ(*db.hget("h", "b"), "2");
    EXPECT_FALSE(db.hget("h", "missing").has_value());

    // hkeys отсортированы
    EXPECT_EQ(db.hkeys("h"), (std::vector<std::string>{"a", "b"}));
    EXPECT_TRUE(db.hkeys("missing").empty());
}

// ============================ generic ============================
TEST(Database, TypeDelKeysSize) {
    Database db;
    db.set("s", "x");
    db.lpush("l", "x");
    db.hset("hh", "f", "x");

    EXPECT_EQ(db.type("s"), std::optional<Type>{Type::String});
    EXPECT_EQ(db.type("l"), std::optional<Type>{Type::List});
    EXPECT_EQ(db.type("hh"), std::optional<Type>{Type::Hash});
    EXPECT_FALSE(db.type("nope").has_value());

    EXPECT_EQ(db.size(), 3u);
    // keys() отсортированы
    EXPECT_EQ(db.keys(), (std::vector<std::string>{"hh", "l", "s"}));

    EXPECT_TRUE(db.del("l"));
    EXPECT_FALSE(db.del("l"));  // уже нет
    EXPECT_EQ(db.size(), 2u);
    EXPECT_EQ(db.keys(), (std::vector<std::string>{"hh", "s"}));
}

// ===================== expiry через инъекцию часов ====================
TEST(Database, TtlWithInjectedClock) {
    // Общий «текущий момент», которым тест управляет вручную — никакого реального ожидания.
    auto fake_now = std::make_shared<std::int64_t>(1000);
    Database db([fake_now] { return *fake_now; });

    db.set("k", "v");
    EXPECT_EQ(db.ttl("k"), -1);  // ключ есть, TTL не задан
    EXPECT_EQ(db.ttl("absent"), -2);

    EXPECT_TRUE(db.expire("k", 10));   // протухнет в момент 1010
    EXPECT_FALSE(db.expire("absent", 10));
    EXPECT_EQ(db.ttl("k"), 10);

    *fake_now = 1005;                  // «прошло 5 секунд»
    EXPECT_EQ(db.ttl("k"), 5);
    EXPECT_TRUE(db.get("k").has_value());  // ещё жив

    *fake_now = 1010;                  // ровно момент протухания -> считается истёкшим
    EXPECT_EQ(db.ttl("k"), -2);
    EXPECT_FALSE(db.get("k").has_value());  // ленивое протухание: ключа больше «нет»
    EXPECT_EQ(db.size(), 0u);
}

TEST(Database, PersistRemovesTtl) {
    auto fake_now = std::make_shared<std::int64_t>(0);
    Database db([fake_now] { return *fake_now; });

    db.set("k", "v");
    EXPECT_TRUE(db.expire("k", 5));
    EXPECT_EQ(db.ttl("k"), 5);

    EXPECT_TRUE(db.persist("k"));   // снять TTL
    EXPECT_FALSE(db.persist("k"));  // снимать уже нечего
    EXPECT_EQ(db.ttl("k"), -1);

    *fake_now = 1000;               // время ушло далеко вперёд
    EXPECT_TRUE(db.get("k").has_value());  // но ключ уже вечный
}

TEST(Database, SetClearsTtl) {
    auto fake_now = std::make_shared<std::int64_t>(0);
    Database db([fake_now] { return *fake_now; });

    db.set("k", "v");
    EXPECT_TRUE(db.expire("k", 5));
    db.set("k", "v2");             // SET по тому же ключу должен сбросить TTL
    EXPECT_EQ(db.ttl("k"), -1);
}

// ===================== персистентность через временный файл =====================
TEST(Database, SaveLoadRoundtrip) {
    const std::string path = temp_db_path("roundtrip");
    std::filesystem::remove(path);

    auto fake_now = std::make_shared<std::int64_t>(100);
    {
        Database db([fake_now] { return *fake_now; });
        db.set("s", "hello world");           // значение с пробелом
        db.lpush("l", "a");
        db.lpush("l", "b");                   // [b, a]
        db.hset("h", "f1", "v1");
        db.hset("h", "f2", "v2");
        db.expire("s", 50);                   // протухнет в 150
        EXPECT_TRUE(db.save(path));
    }

    {
        Database db2([fake_now] { return *fake_now; });
        EXPECT_TRUE(db2.load(path));

        EXPECT_EQ(*db2.get("s"), "hello world");
        EXPECT_EQ(db2.lrange("l", 0, -1), (std::vector<std::string>{"b", "a"}));
        EXPECT_EQ(db2.type("h"), std::optional<Type>{Type::Hash});
        EXPECT_EQ(db2.hkeys("h"), (std::vector<std::string>{"f1", "f2"}));

        EXPECT_EQ(db2.ttl("s"), 50);  // TTL пережил сохранение/загрузку

        *fake_now = 150;              // протухание после загрузки тоже работает
        EXPECT_FALSE(db2.get("s").has_value());
    }

    std::filesystem::remove(path);
}

TEST(Database, LoadMissingFileFails) {
    Database db;
    EXPECT_FALSE(db.load(temp_db_path("definitely_absent_file")));
}

// ======================================================================
//  PROPERTY-ТЕСТЫ (рандомизированные, но ДЕТЕРМИНИРОВАННЫЕ)
// ----------------------------------------------------------------------
//  Идея property-теста (см. README соответствующих модулей): вместо одного
//  «ручного» примера мы формулируем ИНВАРИАНТ — утверждение, которое должно
//  держаться для ЛЮБОГО входа, — и прогоняем его на множестве случайных
//  входов. Генератор std::mt19937 с ФИКСИРОВАННЫМ сидом делает прогон
//  воспроизводимым: упавший кейс всегда упадёт снова (никакой флакости).
//  Это ловит классы ошибок, которые точечные тесты пропускают: краевые
//  индексы, редкие комбинации операций, рассинхрон сериализации.
// ======================================================================

// Генерация случайной печатной строки БЕЗ пробелов/переводов строки.
// (Текстовый протокол execute() режет по пробелам; но здесь мы бьём по
//  Database напрямую — и всё же избегаем управляющих символов, чтобы тело
//  теста было читаемым и чтобы заодно крыть length-prefixed сериализацию.)
static std::string random_token(std::mt19937& rng, std::size_t max_len = 12) {
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<std::size_t> len_dist(1, max_len);
    std::uniform_int_distribution<std::size_t> ch_dist(0, sizeof(alphabet) - 2);
    const std::size_t len = len_dist(rng);
    std::string s;
    s.reserve(len);
    for (std::size_t i = 0; i < len; ++i) s.push_back(alphabet[ch_dist(rng)]);
    return s;
}

// (а) Инвариант: llen(key) == числу выполненных lpush в этот ключ.
//     Для случайной последовательности lpush по нескольким ключам ведём
//     эталонный счётчик в обычном std::map и сверяем его с llen.
TEST(Database, PropertyLpushCountMatchesLlen) {
    std::mt19937 rng(12345);  // фиксированный сид → воспроизводимость
    Database db;

    const int kKeys = 6;
    const int kOps = 2000;
    std::map<std::string, std::size_t> expected_len;

    std::uniform_int_distribution<int> key_dist(0, kKeys - 1);
    for (int i = 0; i < kOps; ++i) {
        const std::string key = "k" + std::to_string(key_dist(rng));
        const std::size_t new_len = db.lpush(key, random_token(rng));
        const std::size_t want = ++expected_len[key];
        // lpush должен вернуть новую длину == накопленному счётчику
        ASSERT_EQ(new_len, want) << "lpush returned wrong length for " << key;
    }
    // И финальный llen по каждому ключу обязан совпасть со счётчиком.
    for (const auto& [key, len] : expected_len) {
        EXPECT_EQ(db.llen(key), len) << "llen mismatch for " << key;
    }
}

// (б) Инвариант: save/load — это round-trip. Для случайного состояния БД
//     (строки/списки/хеши, БЕЗ TTL) сохранение и загрузка в новую базу дают
//     ИДЕНТИЧНОЕ наблюдаемое состояние. Сверяем по keys()+type()+содержимому.
TEST(Database, PropertySaveLoadRoundtrip) {
    std::mt19937 rng(777);
    const std::string path = temp_db_path("prop_roundtrip");
    std::filesystem::remove(path);

    Database src;
    // Эталонные «зеркала» состояния — то, что мы ожидаем прочитать обратно.
    std::map<std::string, std::string> exp_strings;
    std::map<std::string, std::vector<std::string>> exp_lists;
    std::map<std::string, std::map<std::string, std::string>> exp_hashes;

    const int kOps = 400;
    std::uniform_int_distribution<int> shape_dist(0, 2);  // 0=string 1=list 2=hash
    std::uniform_int_distribution<int> key_dist(0, 20);

    for (int i = 0; i < kOps; ++i) {
        const std::string key = "key" + std::to_string(key_dist(rng));
        const int shape = shape_dist(rng);
        if (shape == 0) {  // строка: set перезаписывает любой прежний тип
            const std::string val = random_token(rng);
            src.set(key, val);
            exp_strings[key] = val;
            exp_lists.erase(key);
            exp_hashes.erase(key);
        } else if (shape == 1) {  // список: lpush в начало — только если ключ ещё не занят иным типом
            if (exp_strings.count(key) || exp_hashes.count(key)) continue;
            const std::string val = random_token(rng);
            src.lpush(key, val);
            exp_lists[key].insert(exp_lists[key].begin(), val);
        } else {  // хеш
            if (exp_strings.count(key) || exp_lists.count(key)) continue;
            const std::string field = random_token(rng, 6);
            const std::string val = random_token(rng);
            src.hset(key, field, val);
            exp_hashes[key][field] = val;
        }
    }

    ASSERT_TRUE(src.save(path));

    Database dst;
    ASSERT_TRUE(dst.load(path));

    // Множество живых ключей должно совпасть.
    std::set<std::string> exp_keys;
    for (const auto& [k, _] : exp_strings) exp_keys.insert(k);
    for (const auto& [k, _] : exp_lists) exp_keys.insert(k);
    for (const auto& [k, _] : exp_hashes) exp_keys.insert(k);

    auto loaded_keys = dst.keys();
    std::set<std::string> loaded_set(loaded_keys.begin(), loaded_keys.end());
    EXPECT_EQ(loaded_set, exp_keys);

    // Содержимое каждого ключа должно совпасть с зеркалом.
    for (const auto& [k, v] : exp_strings) {
        auto got = dst.get(k);
        ASSERT_TRUE(got.has_value()) << "missing string " << k;
        EXPECT_EQ(*got, v);
    }
    for (const auto& [k, items] : exp_lists) {
        EXPECT_EQ(dst.lrange(k, 0, -1), items) << "list mismatch " << k;
    }
    for (const auto& [k, fields] : exp_hashes) {
        EXPECT_EQ(dst.type(k), std::optional<Type>{Type::Hash});
        for (const auto& [f, val] : fields) {
            auto got = dst.hget(k, f);
            ASSERT_TRUE(got.has_value()) << "missing field " << k << "/" << f;
            EXPECT_EQ(*got, val);
        }
    }

    std::filesystem::remove(path);
}

// (в) Инвариант: set/get — round-trip для случайных ключей/значений. Последняя
//     запись по ключу «выигрывает»; size() == числу различных живых ключей.
TEST(Database, PropertySetGetRoundtrip) {
    std::mt19937 rng(2024);
    Database db;
    std::map<std::string, std::string> mirror;  // эталон «последнее значение по ключу»

    const int kOps = 3000;
    std::uniform_int_distribution<int> key_dist(0, 50);
    for (int i = 0; i < kOps; ++i) {
        const std::string key = "k" + std::to_string(key_dist(rng));
        const std::string val = random_token(rng);
        db.set(key, val);
        mirror[key] = val;  // перезапись отражаем и в зеркале
    }

    EXPECT_EQ(db.size(), mirror.size());
    for (const auto& [key, val] : mirror) {
        auto got = db.get(key);
        ASSERT_TRUE(got.has_value()) << "missing key " << key;
        EXPECT_EQ(*got, val) << "value mismatch for " << key;
    }

    // keys() обязан быть отсортирован и совпадать с множеством ключей зеркала.
    auto ks = db.keys();
    EXPECT_TRUE(std::is_sorted(ks.begin(), ks.end()));
    std::vector<std::string> want;
    for (const auto& [k, _] : mirror) want.push_back(k);  // std::map уже отсортирован
    EXPECT_EQ(ks, want);
}
