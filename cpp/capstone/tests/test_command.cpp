// test_command.cpp — приёмочные тесты текстового слоя. Пока src/command.cpp — стаб (всё "ERR ..."),
// тесты КРАСНЫЕ. Реализуй execute() так, чтобы ответы совпали. МЕНЯТЬ тесты не нужно.
//
// Внимание: эти тесты опираются и на Database (через execute), так что их зелёный цвет требует
// сперва рабочего ядра. Это нормально для капстоуна — слои зависят друг от друга.

#include <gtest/gtest.h>

#include <string>

#include "command.hpp"
#include "database.hpp"

using minidb::Database;
using minidb::execute;

TEST(Command, StringsBasic) {
    Database db;
    EXPECT_EQ(execute(db, "SET name Ada"), "OK");
    EXPECT_EQ(execute(db, "GET name"), "Ada");
    EXPECT_EQ(execute(db, "GET missing"), "(nil)");
    // имя команды регистронезависимо
    EXPECT_EQ(execute(db, "get name"), "Ada");
}

TEST(Command, Lists) {
    Database db;
    EXPECT_EQ(execute(db, "LPUSH xs a"), ":1");
    EXPECT_EQ(execute(db, "LPUSH xs b"), ":2");  // [b, a]
    EXPECT_EQ(execute(db, "LLEN xs"), ":2");
    EXPECT_EQ(execute(db, "LRANGE xs 0 -1"), "b\na");
    EXPECT_EQ(execute(db, "LRANGE empty 0 -1"), "(empty)");
}

TEST(Command, Hashes) {
    Database db;
    EXPECT_EQ(execute(db, "HSET h f1 v1"), ":1");  // новое поле
    EXPECT_EQ(execute(db, "HSET h f1 v2"), ":0");  // перезапись
    EXPECT_EQ(execute(db, "HGET h f1"), "v2");
    EXPECT_EQ(execute(db, "HGET h nope"), "(nil)");
    EXPECT_EQ(execute(db, "HSET h a 0"), ":1");
    EXPECT_EQ(execute(db, "HKEYS h"), "a\nf1");      // отсортировано
}

TEST(Command, GenericAndCount) {
    Database db;
    execute(db, "SET a 1");
    execute(db, "SET b 2");
    EXPECT_EQ(execute(db, "COUNT"), ":2");
    EXPECT_EQ(execute(db, "KEYS"), "a\nb");
    EXPECT_EQ(execute(db, "TYPE a"), "string");
    EXPECT_EQ(execute(db, "TYPE missing"), "none");
    EXPECT_EQ(execute(db, "DEL a"), ":1");
    EXPECT_EQ(execute(db, "DEL a"), ":0");
    EXPECT_EQ(execute(db, "COUNT"), ":1");
}

TEST(Command, Expiry) {
    Database db;
    execute(db, "SET k v");
    EXPECT_EQ(execute(db, "TTL k"), ":-1");      // без TTL
    EXPECT_EQ(execute(db, "TTL absent"), ":-2"); // нет ключа
    EXPECT_EQ(execute(db, "EXPIRE k 100"), ":1");
    EXPECT_EQ(execute(db, "EXPIRE absent 100"), ":0");
    EXPECT_EQ(execute(db, "PERSIST k"), ":1");
    EXPECT_EQ(execute(db, "PERSIST k"), ":0");
}

TEST(Command, Errors) {
    Database db;
    // неизвестная команда
    EXPECT_EQ(execute(db, "FOOBAR x"), "ERR unknown command");
    // не хватает аргументов
    EXPECT_EQ(execute(db, "SET onlykey"), "ERR wrong number of arguments");
    // нечисловой аргумент там, где нужно число
    EXPECT_EQ(execute(db, "EXPIRE k notanumber"), "ERR value is not an integer");
    // пустая строка
    EXPECT_EQ(execute(db, ""), "ERR empty command");
}
