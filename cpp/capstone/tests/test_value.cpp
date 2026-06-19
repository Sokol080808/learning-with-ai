// test_value.cpp — приёмочные тесты для Value. Они ЗАДАЮТ поведение: пока src/value.cpp — стаб,
// тесты КРАСНЫЕ. Реализуй value.cpp так, чтобы все они стали зелёными. Эти тесты МЕНЯТЬ не нужно.

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

#include "value.hpp"

using minidb::Type;
using minidb::Value;

TEST(Value, StringRoundtrip) {
    Value v = Value::make_string("hello");
    EXPECT_EQ(v.type(), Type::String);
    EXPECT_EQ(v.as_string(), "hello");
}

TEST(Value, ListStartsEmptyAndIsMutable) {
    Value v = Value::make_list();
    EXPECT_EQ(v.type(), Type::List);
    EXPECT_TRUE(v.as_list().empty());

    // неconst as_list() должен давать менять данные на месте
    v.as_list().push_back("a");
    v.as_list().push_back("b");
    ASSERT_EQ(v.as_list().size(), 2u);
    EXPECT_EQ(v.as_list()[0], "a");
    EXPECT_EQ(v.as_list()[1], "b");
}

TEST(Value, HashStartsEmptyAndIsMutable) {
    Value v = Value::make_hash();
    EXPECT_EQ(v.type(), Type::Hash);
    EXPECT_TRUE(v.as_hash().empty());

    v.as_hash()["k"] = "val";
    ASSERT_EQ(v.as_hash().count("k"), 1u);
    EXPECT_EQ(v.as_hash().at("k"), "val");
}

TEST(Value, WrongAccessorThrows) {
    Value s = Value::make_string("x");
    EXPECT_THROW(s.as_list(), std::runtime_error);
    EXPECT_THROW(s.as_hash(), std::runtime_error);

    Value l = Value::make_list();
    EXPECT_THROW(l.as_string(), std::runtime_error);
    EXPECT_THROW(l.as_hash(), std::runtime_error);

    Value h = Value::make_hash();
    EXPECT_THROW(h.as_string(), std::runtime_error);
    EXPECT_THROW(h.as_list(), std::runtime_error);
}

TEST(Value, ConstAccessorReads) {
    const Value v = Value::make_string("frozen");
    EXPECT_EQ(v.type(), Type::String);
    EXPECT_EQ(v.as_string(), "frozen");        // const-версия аксессора
    EXPECT_THROW(v.as_list(), std::runtime_error);
}
