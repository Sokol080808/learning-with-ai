// test_database.cpp — приёмочные тесты ядра. Они ЗАДАЮТ поведение Database; пока src/database.cpp —
// стаб, все они КРАСНЫЕ. Особо обрати внимание на ДВА детерминированных приёма:
//   1) TTL проверяется через ИНЪЕКЦИЮ ЧАСОВ: общий fake_now двигается руками, без реального sleep.
//   2) Персистентность проверяется через ВРЕМЕННЫЙ файл (std::filesystem::temp_directory_path),
//      без сети и без хардкода путей.
// Эти тесты МЕНЯТЬ не нужно — меняй реализацию, пока они не позеленеют.

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
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
