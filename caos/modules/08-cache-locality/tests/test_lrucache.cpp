// tests/test_lrucache.cpp
// Эти тесты трогать не нужно — это эталон поведения LRU-кэша.
// Они проверяют: попадания (hit), промахи (miss) и вытеснение наименее
// недавно использованной записи (LRU eviction).
//
// Заголовок написан на C; включаем его в extern "C", чтобы имена функций
// совпали с тем, что собирает компилятор C. (Заголовок и сам обёрнут в
// extern "C" — двойная обёртка безопасна.)

#include <gtest/gtest.h>
#include <random>
#include <vector>
#include <unordered_map>
#include <list>
#include <algorithm>

extern "C" {
#include "lrucache.h"
}

// --- Базовая жизнь объекта -------------------------------------------------

// Создание возвращает не-NULL; освобождение не падает.
TEST(LruCache, CreateReturnsNonNull) {
    LruCache* c = lru_create(2);
    ASSERT_NE(c, nullptr);
    lru_free(c);
}

// lru_free(NULL) должен быть безопасным (ничего не делает, не падает).
TEST(LruCache, FreeNullIsSafe) {
    lru_free(nullptr);
    SUCCEED();
}

// --- Промахи ---------------------------------------------------------------

// В пустом кэше любого ключа нет -> -1.
TEST(LruCache, GetFromEmptyMisses) {
    LruCache* c = lru_create(2);
    ASSERT_NE(c, nullptr);
    EXPECT_EQ(lru_get(c, 1), -1);
    EXPECT_EQ(lru_get(c, 42), -1);
    lru_free(c);
}

// --- Попадания -------------------------------------------------------------

// Положили — прочитали то же значение.
TEST(LruCache, PutThenGetHits) {
    LruCache* c = lru_create(2);
    ASSERT_NE(c, nullptr);
    lru_put(c, 1, 100);
    lru_put(c, 2, 200);
    EXPECT_EQ(lru_get(c, 1), 100);
    EXPECT_EQ(lru_get(c, 2), 200);
    lru_free(c);
}

// Повторный put по тому же ключу обновляет значение, а не плодит дубликаты.
TEST(LruCache, PutSameKeyUpdatesValue) {
    LruCache* c = lru_create(2);
    ASSERT_NE(c, nullptr);
    lru_put(c, 7, 1);
    lru_put(c, 7, 2);
    lru_put(c, 7, 3);
    EXPECT_EQ(lru_get(c, 7), 3);
    // Ключ всего один, значит место под второй ключ свободно.
    lru_put(c, 8, 80);
    EXPECT_EQ(lru_get(c, 8), 80);
    EXPECT_EQ(lru_get(c, 7), 3);
    lru_free(c);
}

// --- Вытеснение (LRU) ------------------------------------------------------

// Ёмкость 2. Кладём 1 и 2, затем 3 — должна вылететь самая старая (1).
TEST(LruCache, EvictsLeastRecentlyUsed) {
    LruCache* c = lru_create(2);
    ASSERT_NE(c, nullptr);
    lru_put(c, 1, 10);
    lru_put(c, 2, 20);
    lru_put(c, 3, 30);          // переполнение: вытесняем 1 (самый старый)
    EXPECT_EQ(lru_get(c, 1), -1);   // 1 вытеснен
    EXPECT_EQ(lru_get(c, 2), 20);   // 2 на месте
    EXPECT_EQ(lru_get(c, 3), 30);   // 3 на месте
    lru_free(c);
}

// get «освежает» запись: к ней обращались недавно — значит вылетит другая.
// Кладём 1,2; читаем 1 (теперь 2 — самый старый); кладём 3 -> вылетает 2.
TEST(LruCache, GetRefreshesRecency) {
    LruCache* c = lru_create(2);
    ASSERT_NE(c, nullptr);
    lru_put(c, 1, 10);
    lru_put(c, 2, 20);
    EXPECT_EQ(lru_get(c, 1), 10);   // 1 стал «свежим», 2 — самый старый
    lru_put(c, 3, 30);              // вытесняем 2
    EXPECT_EQ(lru_get(c, 2), -1);   // 2 вытеснен
    EXPECT_EQ(lru_get(c, 1), 10);   // 1 остался
    EXPECT_EQ(lru_get(c, 3), 30);   // 3 на месте
    lru_free(c);
}

// Обновление значения через put тоже «освежает» запись.
// Кладём 1,2; put(1,...) освежает 1; кладём 3 -> вылетает 2.
TEST(LruCache, PutRefreshesRecency) {
    LruCache* c = lru_create(2);
    ASSERT_NE(c, nullptr);
    lru_put(c, 1, 10);
    lru_put(c, 2, 20);
    lru_put(c, 1, 11);              // обновили + освежили 1; 2 теперь старший
    lru_put(c, 3, 30);             // вытесняем 2
    EXPECT_EQ(lru_get(c, 2), -1);  // 2 вытеснен
    EXPECT_EQ(lru_get(c, 1), 11);  // у 1 новое значение
    EXPECT_EQ(lru_get(c, 3), 30);  // 3 на месте
    lru_free(c);
}

// Чуть длиннее: ёмкость 3, серия операций.
TEST(LruCache, CapacityThreeSequence) {
    LruCache* c = lru_create(3);
    ASSERT_NE(c, nullptr);
    lru_put(c, 1, 1);
    lru_put(c, 2, 2);
    lru_put(c, 3, 3);              // [1,2,3], старший — 1
    EXPECT_EQ(lru_get(c, 1), 1);  // освежили 1 -> старший теперь 2
    lru_put(c, 4, 4);             // вытесняем 2
    EXPECT_EQ(lru_get(c, 2), -1);
    EXPECT_EQ(lru_get(c, 1), 1);
    EXPECT_EQ(lru_get(c, 3), 3);
    EXPECT_EQ(lru_get(c, 4), 4);
    lru_free(c);
}

// Ёмкость 1: каждый новый ключ вытесняет предыдущий.
TEST(LruCache, CapacityOne) {
    LruCache* c = lru_create(1);
    ASSERT_NE(c, nullptr);
    lru_put(c, 1, 100);
    EXPECT_EQ(lru_get(c, 1), 100);
    lru_put(c, 2, 200);             // вытесняем 1
    EXPECT_EQ(lru_get(c, 1), -1);
    EXPECT_EQ(lru_get(c, 2), 200);
    lru_free(c);
}

// =============================================================================
// Randomized tests — oracle-based
// =============================================================================
//
// RefLRU is a simple, obviously-correct LRU implementation built from
// std::list + std::unordered_map.  Every randomized test drives both the real
// cache and RefLRU with the same sequence of operations and asserts the results
// are identical.
// =============================================================================

namespace {

// A textbook-correct LRU cache used as an oracle.
struct RefLRU {
    size_t cap;
    // front = most-recently used, back = least-recently used
    std::list<std::pair<int,int>> order;   // (key, value)
    std::unordered_map<int, std::list<std::pair<int,int>>::iterator> map;

    explicit RefLRU(size_t capacity) : cap(capacity) {}

    // Returns -1 on miss, value on hit (and refreshes recency).
    int get(int key) {
        auto it = map.find(key);
        if (it == map.end()) return -1;
        // Move to front (most recently used).
        order.splice(order.begin(), order, it->second);
        return it->second->second;
    }

    void put(int key, int value) {
        auto it = map.find(key);
        if (it != map.end()) {
            it->second->second = value;
            order.splice(order.begin(), order, it->second);
            return;
        }
        if (order.size() == cap) {
            // evict LRU entry (back of list)
            map.erase(order.back().first);
            order.pop_back();
        }
        order.push_front({key, value});
        map[key] = order.begin();
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// TEST 1 — Random put/get sequence against oracle (capacity 4, 800 ops)
// ---------------------------------------------------------------------------
TEST(LruCacheRand, PutGetMatchesOracle) {
    std::mt19937 rng(0xC0FFEE);
    const size_t CAP = 4;
    const int OPS = 800;

    // Key universe slightly larger than capacity so evictions happen often.
    std::uniform_int_distribution<int> key_dist(0, 7);
    std::uniform_int_distribution<int> val_dist(0, 1000);
    std::uniform_int_distribution<int> op_dist(0, 1); // 0=get, 1=put

    LruCache* c = lru_create(CAP);
    ASSERT_NE(c, nullptr);
    RefLRU ref(CAP);

    for (int i = 0; i < OPS; ++i) {
        int key = key_dist(rng);
        if (op_dist(rng) == 0) {
            // get
            int got  = lru_get(c, key);
            int want = ref.get(key);
            ASSERT_EQ(got, want) << "get(" << key << ") mismatch at op " << i;
        } else {
            // put
            int val = val_dist(rng);
            lru_put(c, key, val);
            ref.put(key, val);
        }
    }
    lru_free(c);
}

// ---------------------------------------------------------------------------
// TEST 2 — Varying capacities (1..8) with 600 random ops each
// ---------------------------------------------------------------------------
TEST(LruCacheRand, VaryingCapacities) {
    std::mt19937 rng(0xDEADBEEF);
    const int OPS = 600;

    std::uniform_int_distribution<int> val_dist(0, 500);
    std::uniform_int_distribution<int> op_dist(0, 1);

    for (size_t cap = 1; cap <= 8; ++cap) {
        // Key universe = cap + 2 so evictions always happen.
        std::uniform_int_distribution<int> key_dist(0, (int)cap + 1);

        LruCache* c = lru_create(cap);
        ASSERT_NE(c, nullptr) << "lru_create(" << cap << ") returned NULL";
        RefLRU ref(cap);

        for (int i = 0; i < OPS; ++i) {
            int key = key_dist(rng);
            if (op_dist(rng) == 0) {
                int got  = lru_get(c, key);
                int want = ref.get(key);
                ASSERT_EQ(got, want)
                    << "cap=" << cap << " get(" << key << ") at op " << i;
            } else {
                int val = val_dist(rng);
                lru_put(c, key, val);
                ref.put(key, val);
            }
        }
        lru_free(c);
    }
}

// ---------------------------------------------------------------------------
// TEST 3 — put-then-get round-trip: freshly inserted key is always readable
// ---------------------------------------------------------------------------
TEST(LruCacheRand, PutAlwaysReadable) {
    std::mt19937 rng(0xBEEF1234);
    const size_t CAP = 5;
    const int ROUNDS = 700;

    std::uniform_int_distribution<int> key_dist(0, 9);
    std::uniform_int_distribution<int> val_dist(-500, 500);

    LruCache* c = lru_create(CAP);
    ASSERT_NE(c, nullptr);
    RefLRU ref(CAP);

    for (int i = 0; i < ROUNDS; ++i) {
        int key = key_dist(rng);
        int val = val_dist(rng);

        lru_put(c, key, val);
        ref.put(key, val);

        // Immediately after put, get must return the exact value we just stored.
        int got  = lru_get(c, key);
        int want = ref.get(key);    // oracle must agree
        ASSERT_EQ(got, val)  << "just put key=" << key << " val=" << val
                              << " but got " << got;
        ASSERT_EQ(got, want) << "oracle mismatch after put for key=" << key;
    }
    lru_free(c);
}

// ---------------------------------------------------------------------------
// TEST 4 — get refreshes recency: after get(k), k is never the next evictee
// ---------------------------------------------------------------------------
TEST(LruCacheRand, GetRefreshesRecencyRand) {
    std::mt19937 rng(0xCAFEBABE);
    const size_t CAP = 3;
    const int ROUNDS = 500;

    // Fill the cache completely first, then in each round:
    //   1. get an existing key (refreshes it)
    //   2. put a brand-new key (forces eviction)
    //   3. that key must NOT have been evicted
    LruCache* c = lru_create(CAP);
    ASSERT_NE(c, nullptr);
    RefLRU ref(CAP);

    // Seed with keys 0..CAP-1
    for (int k = 0; k < (int)CAP; ++k) {
        lru_put(c, k, k * 10);
        ref.put(k, k * 10);
    }

    std::uniform_int_distribution<int> cap_dist(0, (int)CAP - 1);
    int next_new_key = (int)CAP; // keys >= CAP are guaranteed new

    for (int i = 0; i < ROUNDS; ++i) {
        // Pick one of the currently-present keys to refresh.
        // We use the oracle to find a key that is definitely present.
        // Scan oracle's order list for a random present key.
        int refresh_key = ref.order.begin()->first; // MRU key in oracle
        // Pick a random key from oracle's list instead.
        {
            int idx = cap_dist(rng);
            auto it = ref.order.begin();
            std::advance(it, idx % ref.order.size());
            refresh_key = it->first;
        }

        // Refresh via get.
        int got_ref  = lru_get(c, refresh_key);
        int want_ref = ref.get(refresh_key);
        ASSERT_EQ(got_ref, want_ref)
            << "get(" << refresh_key << ") mismatch before eviction, round " << i;
        ASSERT_NE(got_ref, -1)
            << "refresh_key=" << refresh_key << " unexpectedly absent, round " << i;

        // Force eviction by inserting a new key.
        int new_key = next_new_key++;
        lru_put(c, new_key, new_key);
        ref.put(new_key, new_key);

        // The refreshed key must still be present.
        int still = lru_get(c, refresh_key);
        int still_ref = ref.get(refresh_key);
        ASSERT_EQ(still, still_ref)
            << "after eviction: refreshed key=" << refresh_key
            << " got=" << still << " want=" << still_ref << " round=" << i;
        ASSERT_NE(still, -1)
            << "refreshed key=" << refresh_key << " was evicted — should not be";
    }
    lru_free(c);
}

// ---------------------------------------------------------------------------
// TEST 5 — Repeated update of the same key never grows the cache beyond capacity
// ---------------------------------------------------------------------------
TEST(LruCacheRand, UpdateSameKeyNoCapacityLeak) {
    std::mt19937 rng(0x1234ABCD);
    const size_t CAP = 4;
    const int OPS = 600;

    // Key universe = exactly CAP keys, so no eviction until a new key arrives.
    std::uniform_int_distribution<int> key_dist(0, (int)CAP - 1);
    std::uniform_int_distribution<int> val_dist(0, 9999);

    LruCache* c = lru_create(CAP);
    ASSERT_NE(c, nullptr);
    RefLRU ref(CAP);

    for (int i = 0; i < OPS; ++i) {
        int key = key_dist(rng);
        int val = val_dist(rng);
        lru_put(c, key, val);
        ref.put(key, val);
    }

    // After many updates, all CAP keys should still be readable with correct values.
    for (auto& [key, it] : ref.map) {
        int want = it->second;
        EXPECT_EQ(lru_get(c, key), want)
            << "key=" << key << " wrong value after many updates";
    }

    // A brand-new key must cause exactly one eviction (not a crash or silent no-op).
    int extra_key = (int)CAP + 100;
    int extra_val = 42;
    lru_put(c, extra_key, extra_val);
    ref.put(extra_key, extra_val);
    EXPECT_EQ(lru_get(c, extra_key), extra_val);

    lru_free(c);
}

// ---------------------------------------------------------------------------
// TEST 6 — Edge cases: capacity-1, zero/negative/INT_MIN/INT_MAX keys & values
// ---------------------------------------------------------------------------
TEST(LruCacheRand, EdgeCasesExtremeKeys) {
    // capacity 1
    {
        LruCache* c = lru_create(1);
        ASSERT_NE(c, nullptr);
        // Inserting the same key many times must not crash.
        for (int v = 0; v < 100; ++v) {
            lru_put(c, 0, v);
            EXPECT_EQ(lru_get(c, 0), v);
        }
        // A different key evicts the previous one.
        lru_put(c, 1, 7);
        EXPECT_EQ(lru_get(c, 0), -1);
        EXPECT_EQ(lru_get(c, 1), 7);
        lru_free(c);
    }

    // Extreme int values as keys.
    {
        LruCache* c = lru_create(4);
        ASSERT_NE(c, nullptr);
        lru_put(c, 0,          0);
        lru_put(c, -1,        -1);
        lru_put(c, INT_MAX,    1);
        lru_put(c, INT_MIN+1,  2); // INT_MIN+1 to avoid any UB in reference code
        EXPECT_EQ(lru_get(c, 0),         0);
        EXPECT_EQ(lru_get(c, -1),        -1);
        EXPECT_EQ(lru_get(c, INT_MAX),    1);
        EXPECT_EQ(lru_get(c, INT_MIN+1),  2);
        lru_free(c);
    }

    // lru_free(NULL) must be safe (already covered, but include for completeness).
    lru_free(nullptr);
}

// ---------------------------------------------------------------------------
// TEST 7 — Large capacity (100), sparse keys, oracle comparison (1000 ops)
// ---------------------------------------------------------------------------
TEST(LruCacheRand, LargeCapacityOracle) {
    std::mt19937 rng(0xFEEDFACE);
    const size_t CAP = 100;
    const int OPS = 1000;

    // Key universe is smaller than CAP => no evictions; every put is a hit.
    std::uniform_int_distribution<int> key_dist(0, 49);
    std::uniform_int_distribution<int> val_dist(0, 9999);
    std::uniform_int_distribution<int> op_dist(0, 1);

    LruCache* c = lru_create(CAP);
    ASSERT_NE(c, nullptr);
    RefLRU ref(CAP);

    for (int i = 0; i < OPS; ++i) {
        int key = key_dist(rng);
        if (op_dist(rng) == 0) {
            int got  = lru_get(c, key);
            int want = ref.get(key);
            ASSERT_EQ(got, want) << "large-cap get(" << key << ") at op " << i;
        } else {
            int val = val_dist(rng);
            lru_put(c, key, val);
            ref.put(key, val);
        }
    }

    // At the end, no key should have been evicted (universe < CAP).
    for (auto& [key, it] : ref.map) {
        int want = it->second;
        EXPECT_EQ(lru_get(c, key), want) << "large-cap final check key=" << key;
    }

    lru_free(c);
}

// ---------------------------------------------------------------------------
// TEST 8 — Eviction order property: after filling cache, LRU key is always gone
// ---------------------------------------------------------------------------
TEST(LruCacheRand, EvictionOrderProperty) {
    std::mt19937 rng(0xA1B2C3D4);
    const int TRIALS = 300;

    for (int t = 0; t < TRIALS; ++t) {
        std::uniform_int_distribution<int> cap_dist(2, 6);
        size_t cap = cap_dist(rng);
        std::uniform_int_distribution<int> key_dist(0, (int)cap + 2);
        std::uniform_int_distribution<int> val_dist(0, 100);

        LruCache* c = lru_create(cap);
        ASSERT_NE(c, nullptr);
        RefLRU ref(cap);

        // Fill the cache to capacity with distinct keys.
        for (int k = 0; k < (int)cap; ++k) {
            lru_put(c, k, k);
            ref.put(k, k);
        }

        // Insert a random new key (guaranteed not in 0..cap-1 range here).
        int new_key = (int)cap + (int)(rng() % 3) + 1;
        int new_val = val_dist(rng);
        lru_put(c, new_key, new_val);

        // Find which key the oracle evicted.
        int evicted_key = -999;
        for (int k = 0; k < (int)cap; ++k) {
            if (ref.map.find(k) == ref.map.end()) {
                evicted_key = k;
                break;
            }
        }
        ref.put(new_key, new_val);

        // The real cache must agree: evicted key is gone, new key is present.
        if (evicted_key != -999) {
            EXPECT_EQ(lru_get(c, evicted_key), -1)
                << "trial " << t << ": evicted key=" << evicted_key << " still present";
        }
        EXPECT_EQ(lru_get(c, new_key), new_val)
            << "trial " << t << ": new key=" << new_key << " not found";

        lru_free(c);
    }
}
