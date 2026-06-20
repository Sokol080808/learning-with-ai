// test_cache.cpp — тесты СТРЕТЧ-цели Cache<K, V> (header-only LRU из include/cache.hpp).
// Это необязательная часть капстоуна: проверяет шаблонный LRU-кэш на корректность
// политики вытеснения и владения памятью (unique_ptr-цепочка). МЕНЯТЬ не нужно.

#include <gtest/gtest.h>

#include <map>
#include <random>
#include <string>

#include "cache.hpp"

using minidb::Cache;

TEST(Cache, PutGetBasic) {
    Cache<std::string, int> c(2);
    EXPECT_TRUE(c.empty());

    c.put("a", 1);
    c.put("b", 2);
    EXPECT_EQ(c.size(), 2u);

    auto a = c.get("a");
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, 1);
    EXPECT_FALSE(c.get("missing").has_value());
}

TEST(Cache, OverwriteKeepsSize) {
    Cache<std::string, int> c(2);
    c.put("a", 1);
    c.put("a", 99);  // перезапись существующего — размер не растёт
    EXPECT_EQ(c.size(), 1u);
    EXPECT_EQ(*c.get("a"), 99);
}

TEST(Cache, EvictsLeastRecentlyUsed) {
    Cache<std::string, int> c(2);
    c.put("a", 1);
    c.put("b", 2);
    // обращение к "a" делает его most-recent → следующим вытеснится "b"
    EXPECT_EQ(*c.get("a"), 1);
    c.put("c", 3);  // переполнение → вытесняется LRU = "b"

    EXPECT_EQ(c.size(), 2u);
    EXPECT_FALSE(c.contains("b"));  // "b" вытеснен
    EXPECT_TRUE(c.contains("a"));
    EXPECT_TRUE(c.contains("c"));
}

TEST(Cache, PutPromotesToMostRecent) {
    Cache<std::string, int> c(2);
    c.put("a", 1);
    c.put("b", 2);
    c.put("a", 10);  // повторный put по "a" тоже делает его most-recent
    c.put("c", 3);   // вытесняется LRU = "b"
    EXPECT_FALSE(c.contains("b"));
    EXPECT_EQ(*c.get("a"), 10);
    EXPECT_TRUE(c.contains("c"));
}

TEST(Cache, EraseAndClear) {
    Cache<int, int> c(3);
    c.put(1, 1);
    c.put(2, 2);
    c.put(3, 3);
    EXPECT_TRUE(c.erase(2));
    EXPECT_FALSE(c.erase(2));  // повторно — уже нет
    EXPECT_EQ(c.size(), 2u);
    EXPECT_FALSE(c.contains(2));

    c.clear();
    EXPECT_TRUE(c.empty());
    EXPECT_FALSE(c.get(1).has_value());
}

TEST(Cache, CapacityOneAlwaysHoldsLast) {
    Cache<int, std::string> c(1);
    c.put(1, "one");
    c.put(2, "two");  // вытесняет 1
    EXPECT_EQ(c.size(), 1u);
    EXPECT_FALSE(c.contains(1));
    EXPECT_EQ(*c.get(2), "two");
}

// PROPERTY-тест: кэш-эталон. Сравниваем поведение Cache с «медленной, но очевидно
// правильной» эталонной LRU-моделью на std::map (ключ→(значение, штамп-времени
// последнего доступа)). Инварианты: размер никогда не превышает ёмкость; присутствие
// ключа и его значение совпадают с эталоном. Сид фиксирован → воспроизводимо.
TEST(Cache, PropertyMatchesReferenceModel) {
    std::mt19937 rng(4242);
    const std::size_t cap = 4;
    Cache<int, int> c(cap);

    // эталон: key -> (value, last_use_tick)
    std::map<int, std::pair<int, long long>> ref;
    long long tick = 0;

    auto evict_if_needed = [&] {
        while (ref.size() > cap) {
            // найти ключ с минимальным last_use_tick
            auto victim = ref.begin();
            for (auto it = ref.begin(); it != ref.end(); ++it) {
                if (it->second.second < victim->second.second) victim = it;
            }
            ref.erase(victim);
        }
    };

    std::uniform_int_distribution<int> key_dist(0, 9);
    std::uniform_int_distribution<int> val_dist(0, 1000);
    std::uniform_int_distribution<int> op_dist(0, 1);  // 0=put 1=get

    for (int i = 0; i < 5000; ++i) {
        const int key = key_dist(rng);
        if (op_dist(rng) == 0) {  // put
            const int val = val_dist(rng);
            c.put(key, val);
            ref[key] = {val, ++tick};
            evict_if_needed();
        } else {  // get
            auto got = c.get(key);
            auto rit = ref.find(key);
            if (rit != ref.end()) {
                ASSERT_TRUE(got.has_value()) << "ref has key " << key << " but cache lost it";
                EXPECT_EQ(*got, rit->second.first);
                rit->second.second = ++tick;  // успешный get обновляет recency
            }
            // Если ключа нет в эталоне — он МОГ быть вытеснен в кэше; не утверждаем
            // отсутствие жёстко (политики совпадают по построению, но не сверяем здесь).
        }
        // Инвариант ёмкости держится всегда.
        ASSERT_LE(c.size(), cap);
    }
}
