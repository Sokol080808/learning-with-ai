// tests/test_lrucache.cpp
// Эти тесты трогать не нужно — это эталон поведения LRU-кэша.
// Они проверяют: попадания (hit), промахи (miss) и вытеснение наименее
// недавно использованной записи (LRU eviction).
//
// Заголовок написан на C; включаем его в extern "C", чтобы имена функций
// совпали с тем, что собирает компилятор C. (Заголовок и сам обёрнут в
// extern "C" — двойная обёртка безопасна.)

#include <gtest/gtest.h>

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
