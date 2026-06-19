#include <gtest/gtest.h>
#include <filesystem>
#include "kvstore.hpp"

// ----- Майлстоун 1: ядро -----

TEST(KvStoreCore, SetAndGet) {
    KvStore s;
    s.set("name", "Anna");
    ASSERT_TRUE(s.get("name").has_value());
    EXPECT_EQ(s.get("name").value(), "Anna");
    EXPECT_FALSE(s.get("missing").has_value());
}

TEST(KvStoreCore, OverwriteValue) {
    KvStore s;
    s.set("k", "v1");
    s.set("k", "v2");
    EXPECT_EQ(s.get("k").value(), "v2");
    EXPECT_EQ(s.size(), 1u);
}

TEST(KvStoreCore, Erase) {
    KvStore s;
    s.set("k", "v");
    EXPECT_TRUE(s.erase("k"));
    EXPECT_FALSE(s.erase("k"));      // уже удалён
    EXPECT_FALSE(s.get("k").has_value());
}

TEST(KvStoreCore, SizeAndKeysSorted) {
    KvStore s;
    s.set("banana", "1");
    s.set("apple", "2");
    s.set("cherry", "3");
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.keys(), (std::vector<std::string>{"apple", "banana", "cherry"}));
}

// ----- Майлстоун 3: персистентность -----

TEST(KvStorePersistence, SaveThenLoad) {
    const auto path = (std::filesystem::temp_directory_path() / "kv_test_save.txt").string();
    std::filesystem::remove(path);

    KvStore a;
    a.set("name", "Anna");
    a.set("city", "Berlin");
    ASSERT_TRUE(a.save(path));

    KvStore b;
    ASSERT_TRUE(b.load(path));
    EXPECT_EQ(b.size(), 2u);
    EXPECT_EQ(b.get("name").value(), "Anna");
    EXPECT_EQ(b.get("city").value(), "Berlin");

    std::filesystem::remove(path);
}

TEST(KvStorePersistence, LoadMissingFileReturnsFalse) {
    KvStore s;
    EXPECT_FALSE(s.load("/no/such/file/should/exist_42.txt"));
}
