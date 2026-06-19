// test_concurrency.cpp — КОРОННЫЙ тест капстоуна (модуль 14). Он проверяет, что внутренний
// std::mutex Database реально защищает данные при конкурентном доступе с нескольких потоков.
//
// Идея проверки: запускаем N потоков, каждый делает M операций над ОДНИМ объектом Database.
// Если синхронизации нет — будет гонка данных: потерянные вставки, повреждённые контейнеры,
// падения. Если мьютекс на месте — итог ДЕТЕРМИНИРОВАН: суммарная длина == N*M.
//
// Замечание про природу гонок (см. README модуля 14): без мьютекса этот тест может ИНОГДА
// проходить случайно. Поэтому он рассчитан на стаб, который заведомо не накапливает данные
// (длина 0 != N*M) — то есть стартует красным гарантированно. Когда реализуешь мьютекс верно,
// он станет стабильно зелёным. МЕНЯТЬ тест не нужно.

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "database.hpp"

using minidb::Database;

// Каждый поток льёт M значений в ОБЩИЙ список через lpush. Итоговая длина обязана быть N*M.
TEST(Concurrency, ConcurrentLpushKeepsAllElements) {
    Database db;
    const int kThreads = 8;
    const int kPerThread = 5000;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&db, t] {
            for (int i = 0; i < kPerThread; ++i) {
                db.lpush("shared", "t" + std::to_string(t) + "_" + std::to_string(i));
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }

    // Ни одна вставка не должна потеряться.
    EXPECT_EQ(db.llen("shared"), static_cast<std::size_t>(kThreads) * kPerThread);
    EXPECT_EQ(db.lrange("shared", 0, -1).size(),
              static_cast<std::size_t>(kThreads) * kPerThread);
}

// Каждый поток ставит СВОЙ набор уникальных ключей через set. Все ключи должны выжить,
// размер == суммарное число уникальных ключей, контейнер не повреждён.
TEST(Concurrency, ConcurrentSetDistinctKeys) {
    Database db;
    const int kThreads = 8;
    const int kPerThread = 2000;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&db, t] {
            for (int i = 0; i < kPerThread; ++i) {
                db.set("k_" + std::to_string(t) + "_" + std::to_string(i), "v");
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(db.size(), static_cast<std::size_t>(kThreads) * kPerThread);

    auto all = db.keys();
    EXPECT_EQ(all.size(), static_cast<std::size_t>(kThreads) * kPerThread);
    // keys() должны быть отсортированы и уникальны — косвенная проверка целостности map.
    std::set<std::string> uniq(all.begin(), all.end());
    EXPECT_EQ(uniq.size(), all.size());
    EXPECT_TRUE(std::is_sorted(all.begin(), all.end()));
}

// Смешанная нагрузка: одни потоки пишут строки, другие — в общий список. Проверяем, что
// программа не падает и счётчики сходятся (целостность под смешанным доступом).
TEST(Concurrency, MixedWritersAreConsistent) {
    Database db;
    const int kListWriters = 4;
    const int kPerThread = 3000;

    std::vector<std::thread> workers;
    for (int t = 0; t < kListWriters; ++t) {
        workers.emplace_back([&db, t] {
            for (int i = 0; i < kPerThread; ++i) {
                db.lpush("L", std::to_string(t * 100000 + i));
            }
        });
    }
    // параллельно кто-то трогает строковые ключи
    std::atomic<int> str_ops{0};
    for (int t = 0; t < 2; ++t) {
        workers.emplace_back([&db, &str_ops] {
            for (int i = 0; i < kPerThread; ++i) {
                db.set("counter", std::to_string(i));
                str_ops.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(db.llen("L"), static_cast<std::size_t>(kListWriters) * kPerThread);
    EXPECT_EQ(str_ops.load(), 2 * kPerThread);
    EXPECT_TRUE(db.get("counter").has_value());  // строковый ключ цел
}
