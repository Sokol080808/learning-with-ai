#include <gtest/gtest.h>
#include "command.hpp"
#include "kvstore.hpp"

// ----- Майлстоун 2: обработка команд -----

TEST(Command, SetReturnsOk) {
    KvStore s;
    EXPECT_EQ(execute(s, "SET name Anna"), "OK");
}

TEST(Command, GetExistingAndMissing) {
    KvStore s;
    execute(s, "SET name Anna");
    EXPECT_EQ(execute(s, "GET name"), "Anna");
    EXPECT_EQ(execute(s, "GET missing"), "(nil)");
}

TEST(Command, ValueCanContainSpaces) {
    KvStore s;
    EXPECT_EQ(execute(s, "SET greeting Hello World"), "OK");
    EXPECT_EQ(execute(s, "GET greeting"), "Hello World");
}

TEST(Command, Del) {
    KvStore s;
    execute(s, "SET k v");
    EXPECT_EQ(execute(s, "DEL k"), "1");
    EXPECT_EQ(execute(s, "DEL k"), "0");
}

TEST(Command, Count) {
    KvStore s;
    EXPECT_EQ(execute(s, "COUNT"), "0");
    execute(s, "SET a 1");
    execute(s, "SET b 2");
    EXPECT_EQ(execute(s, "COUNT"), "2");
}

TEST(Command, Keys) {
    KvStore s;
    EXPECT_EQ(execute(s, "KEYS"), "(empty)");
    execute(s, "SET banana 1");
    execute(s, "SET apple 2");
    EXPECT_EQ(execute(s, "KEYS"), "apple banana");
}

TEST(Command, UnknownCommand) {
    KvStore s;
    EXPECT_EQ(execute(s, "FOObar x"), "ERR unknown command");
}
