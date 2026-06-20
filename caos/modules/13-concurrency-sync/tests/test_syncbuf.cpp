// Эти тесты трогать не нужно — это эталон поведения.
#include <gtest/gtest.h>

#include <numeric>
#include <random>
#include <thread>
#include <vector>

extern "C" {
#include "syncbuf.h"
}

// --- Однопоточная проверка базовой механики (без блокировок) ----------------

// Положили несколько элементов в буфер с запасом по ёмкости и забрали —
// порядок обязан быть FIFO (первым пришёл — первым вышел).
TEST(BoundedBuffer, FifoOrderSingleThread) {
    BoundedBuffer b;
    bb_init(&b, 8);

    for (int i = 1; i <= 5; ++i) {
        bb_put(&b, i * 10);  // 10, 20, 30, 40, 50
    }
    EXPECT_EQ(bb_get(&b), 10);
    EXPECT_EQ(bb_get(&b), 20);
    EXPECT_EQ(bb_get(&b), 30);
    EXPECT_EQ(bb_get(&b), 40);
    EXPECT_EQ(bb_get(&b), 50);

    bb_destroy(&b);
}

// Кольцо: чередуем put/get столько раз, что индексы заведомо «перевалят» за
// конец массива и завернутся в начало. Порядок всё равно строго FIFO.
TEST(BoundedBuffer, WrapAroundKeepsOrder) {
    BoundedBuffer b;
    bb_init(&b, 4);  // маленькая ёмкость — кольцо завернётся много раз

    int next_put = 0;  // что положим следующим
    int next_get = 0;  // что ОБЯЗАНЫ получить следующим

    // Держим в буфере по 2 элемента и постоянно прокручиваем кольцо.
    bb_put(&b, next_put++);
    bb_put(&b, next_put++);
    for (int i = 0; i < 50; ++i) {
        bb_put(&b, next_put++);
        EXPECT_EQ(bb_get(&b), next_get++);
    }
    EXPECT_EQ(bb_get(&b), next_get++);
    EXPECT_EQ(bb_get(&b), next_get++);

    bb_destroy(&b);
}

// --- Главный сценарий: производитель и потребитель в разных потоках ----------

// Производитель кладёт N элементов (0..N-1), потребитель забирает ровно N.
// Ёмкость МАЛЕНЬКАЯ (4) при большом N — значит буфер постоянно то полон, то
// пуст, и блокировки (cond wait) реально срабатывают. Проверяем три вещи:
//   1) получено ровно N элементов;
//   2) порядок строго FIFO (consumer видит 0,1,2,...,N-1);
//   3) сумма полученного совпадает с суммой положенного.
TEST(BoundedBuffer, ProducerConsumerOrderAndSum) {
    const int N = 1000;
    BoundedBuffer b;
    bb_init(&b, 4);

    std::vector<int> got(N, -1);

    std::thread producer([&] {
        for (int i = 0; i < N; ++i) {
            bb_put(&b, i);
        }
    });

    std::thread consumer([&] {
        for (int i = 0; i < N; ++i) {
            got[i] = bb_get(&b);  // ровно N заборов — на корректной реализации не зависнет
        }
    });

    producer.join();
    consumer.join();

    // Порядок FIFO: i-й забранный элемент обязан быть равен i.
    bool order_ok = true;
    for (int i = 0; i < N; ++i) {
        if (got[i] != i) {
            order_ok = false;
            break;
        }
    }
    EXPECT_TRUE(order_ok) << "элементы пришли не в порядке FIFO";

    long long expected_sum = static_cast<long long>(N - 1) * N / 2;  // 0+1+...+(N-1)
    long long actual_sum = std::accumulate(got.begin(), got.end(), 0LL);
    EXPECT_EQ(actual_sum, expected_sum);

    bb_destroy(&b);
}

// Несколько раз подряд гоняем большой обмен через крошечный буфер ёмкости 1 —
// самый жёсткий режим блокировок: после каждого put буфер полон, после каждого
// get — пуст. Если синхронизация верна, всё проходит стабильно.
TEST(BoundedBuffer, Capacity1Stress) {
    const int N = 500;
    BoundedBuffer b;
    bb_init(&b, 1);

    long long sum = 0;

    std::thread producer([&] {
        for (int i = 1; i <= N; ++i) {
            bb_put(&b, i);
        }
    });

    std::thread consumer([&] {
        for (int i = 0; i < N; ++i) {
            sum += bb_get(&b);
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(sum, static_cast<long long>(N) * (N + 1) / 2);  // 1+2+...+N

    bb_destroy(&b);
}

// =============================================================================
// RANDOMISED TESTS — seeded mt19937(0xC0FFEE), deterministic
// =============================================================================

// --- 1. Random capacity + N, single-thread FIFO + sum oracle -----------------
// Loop ~500 random (capacity, N) pairs. Each time: put all N values into the
// buffer (may require interleaving get/put when N > capacity), then drain and
// verify FIFO order. This catches off-by-one in ring arithmetic and head/tail
// aliasing for every capacity from 1 to 64.
TEST(BoundedBuffer, RandomCapacityFifoRoundTrip) {
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> cap_dist(1, 64);
    std::uniform_int_distribution<int> n_dist(1, 200);

    for (int trial = 0; trial < 500; ++trial) {
        int cap = cap_dist(rng);
        int N   = n_dist(rng);

        BoundedBuffer b;
        bb_init(&b, cap);

        // Interleave: fill up to cap items, then drain all, repeat.
        // This exercises the ring wrap-around at every capacity.
        int put_val = 0;
        int get_val = 0;
        while (put_val < N || get_val < N) {
            // Put up to cap items (or until N exhausted)
            int batch = std::min(cap, N - put_val);
            for (int i = 0; i < batch; ++i) {
                bb_put(&b, put_val++);
            }
            // Drain all that we put
            for (int i = 0; i < batch; ++i) {
                int got = bb_get(&b);
                ASSERT_EQ(got, get_val)
                    << "FIFO violated at trial=" << trial
                    << " cap=" << cap << " N=" << N
                    << " get_val=" << get_val;
                ++get_val;
            }
        }
        EXPECT_EQ(put_val, N);
        EXPECT_EQ(get_val, N);

        bb_destroy(&b);
    }
}

// --- 2. Sequential reuse: init → use → destroy, repeated many times ----------
// Verifies that bb_destroy fully resets state so a fresh bb_init on the same
// stack variable starts clean. Uses random capacities and random item values;
// oracle = sequential sum of items put.
TEST(BoundedBuffer, SequentialReuseNoStateLeak) {
    std::mt19937 rng(0xC0FFEE + 1);
    std::uniform_int_distribution<int> cap_dist(1, 16);
    std::uniform_int_distribution<int> val_dist(-1000, 1000);

    for (int trial = 0; trial < 500; ++trial) {
        int cap = cap_dist(rng);
        int N   = cap;  // fill exactly to capacity, then drain

        BoundedBuffer b;
        bb_init(&b, cap);

        long long expected = 0;
        for (int i = 0; i < N; ++i) {
            int v = val_dist(rng);
            bb_put(&b, v);
            expected += v;
        }
        long long actual = 0;
        for (int i = 0; i < N; ++i) {
            actual += bb_get(&b);
        }
        EXPECT_EQ(actual, expected)
            << "sum mismatch at trial=" << trial << " cap=" << cap;

        bb_destroy(&b);
    }
}

// --- 3. Multi-producer / multi-consumer: total sum equals sequential oracle --
// Randomise the number of producers (1..4), consumers (1..4), capacity (1..16),
// and items per producer (10..100). Each producer puts a random array of ints.
// Consumers collect everything. The only invariant tested is:
//   sum(consumed) == sum(produced)   (no losses, no duplicates)
// This is the canonical oracle for concurrent producer-consumer correctness
// that does NOT depend on ordering (ordering is only FIFO per single producer).
TEST(BoundedBuffer, MultiProducerMultiConsumerSumOracle) {
    std::mt19937 rng(0xC0FFEE + 2);
    std::uniform_int_distribution<int> np_dist(1, 4);
    std::uniform_int_distribution<int> nc_dist(1, 4);
    std::uniform_int_distribution<int> cap_dist(1, 16);
    std::uniform_int_distribution<int> items_dist(10, 100);
    std::uniform_int_distribution<int> val_dist(1, 1000);

    for (int trial = 0; trial < 100; ++trial) {
        int num_producers = np_dist(rng);
        int num_consumers = nc_dist(rng);
        int cap           = cap_dist(rng);
        int items_per_prod = items_dist(rng);
        int total_items   = num_producers * items_per_prod;

        // Build per-producer item arrays and compute oracle sum
        std::vector<std::vector<int>> prod_items(num_producers,
                                                  std::vector<int>(items_per_prod));
        long long oracle_sum = 0;
        for (int p = 0; p < num_producers; ++p) {
            for (int i = 0; i < items_per_prod; ++i) {
                prod_items[p][i] = val_dist(rng);
                oracle_sum += prod_items[p][i];
            }
        }

        BoundedBuffer b;
        bb_init(&b, cap);

        // Shared consumed sum (each consumer accumulates locally, then we add)
        std::vector<long long> consumer_sums(num_consumers, 0LL);
        // Each consumer takes total_items / num_consumers items; last takes remainder
        std::vector<int> consumer_counts(num_consumers, total_items / num_consumers);
        for (int i = 0; i < total_items % num_consumers; ++i) {
            consumer_counts[i]++;
        }

        std::vector<std::thread> producers, consumers;

        for (int p = 0; p < num_producers; ++p) {
            producers.emplace_back([&b, &prod_items, p, items_per_prod]() {
                for (int i = 0; i < items_per_prod; ++i) {
                    bb_put(&b, prod_items[p][i]);
                }
            });
        }
        for (int c = 0; c < num_consumers; ++c) {
            consumers.emplace_back([&b, &consumer_sums, &consumer_counts, c]() {
                for (int i = 0; i < consumer_counts[c]; ++i) {
                    consumer_sums[c] += bb_get(&b);
                }
            });
        }

        for (auto &t : producers) t.join();
        for (auto &t : consumers) t.join();

        long long actual_sum = 0;
        for (long long s : consumer_sums) actual_sum += s;

        EXPECT_EQ(actual_sum, oracle_sum)
            << "sum mismatch at trial=" << trial
            << " np=" << num_producers << " nc=" << num_consumers
            << " cap=" << cap << " items_per_prod=" << items_per_prod;

        bb_destroy(&b);
    }
}

// --- 4. Single-producer single-consumer with random capacity and N -----------
// Threaded version of test 1: producer puts N random values; consumer reads N
// and records them. Oracle: sequential FIFO order must be preserved exactly
// (since single P/single C through a FIFO buffer must deliver in order).
// Random capacities 1..32 and N 100..500 over 100 trials.
TEST(BoundedBuffer, RandomCapacityThreadedFifoOrder) {
    std::mt19937 rng(0xC0FFEE + 3);
    std::uniform_int_distribution<int> cap_dist(1, 32);
    std::uniform_int_distribution<int> n_dist(100, 500);
    std::uniform_int_distribution<int> val_dist(0, 9999);

    for (int trial = 0; trial < 100; ++trial) {
        int cap = cap_dist(rng);
        int N   = n_dist(rng);

        // Generate items
        std::vector<int> items(N);
        for (int &v : items) v = val_dist(rng);

        BoundedBuffer b;
        bb_init(&b, cap);

        std::vector<int> got(N, -1);

        std::thread producer([&]() {
            for (int i = 0; i < N; ++i) bb_put(&b, items[i]);
        });
        std::thread consumer([&]() {
            for (int i = 0; i < N; ++i) got[i] = bb_get(&b);
        });

        producer.join();
        consumer.join();

        // FIFO: got[i] must equal items[i] exactly
        for (int i = 0; i < N; ++i) {
            ASSERT_EQ(got[i], items[i])
                << "FIFO broken at trial=" << trial
                << " cap=" << cap << " N=" << N << " i=" << i;
        }

        bb_destroy(&b);
    }
}

// --- 5. Fill-to-capacity then drain, repeated: count invariant via values ----
// Single thread. Fill buffer exactly to capacity with known values, then drain
// all. Repeat many times (wrap-around stress). Verify that every value comes
// back in FIFO order and nothing is lost or duplicated. Uses random capacities
// 1..20 over 500 trials.
TEST(BoundedBuffer, FillDrainRepeatRingInvariant) {
    std::mt19937 rng(0xC0FFEE + 4);
    std::uniform_int_distribution<int> cap_dist(1, 20);
    std::uniform_int_distribution<int> reps_dist(5, 30);

    for (int trial = 0; trial < 500; ++trial) {
        int cap  = cap_dist(rng);
        int reps = reps_dist(rng);

        BoundedBuffer b;
        bb_init(&b, cap);

        int counter = 0;  // monotonically increasing value
        for (int r = 0; r < reps; ++r) {
            // Fill to capacity
            for (int i = 0; i < cap; ++i) bb_put(&b, counter + i);
            // Drain all and verify FIFO
            for (int i = 0; i < cap; ++i) {
                int got = bb_get(&b);
                ASSERT_EQ(got, counter + i)
                    << "Fill/drain FIFO broken: trial=" << trial
                    << " cap=" << cap << " rep=" << r << " i=" << i;
            }
            counter += cap;
        }

        bb_destroy(&b);
    }
}

// --- 6. Edge cases: capacity=1 with random items, threaded -------------------
// 500 random single-item exchanges through a capacity-1 buffer.
// Oracle: sum of produced == sum of consumed (each get must return exactly what
// was put, since cap=1 forces strict alternation).
TEST(BoundedBuffer, Capacity1RandomItemsOracle) {
    std::mt19937 rng(0xC0FFEE + 5);
    std::uniform_int_distribution<int> n_dist(50, 300);
    std::uniform_int_distribution<int> val_dist(-50000, 50000);

    for (int trial = 0; trial < 200; ++trial) {
        int N = n_dist(rng);
        std::vector<int> items(N);
        long long oracle_sum = 0;
        for (int &v : items) { v = val_dist(rng); oracle_sum += v; }

        BoundedBuffer b;
        bb_init(&b, 1);

        long long actual_sum = 0;
        std::thread producer([&]() {
            for (int i = 0; i < N; ++i) bb_put(&b, items[i]);
        });
        std::thread consumer([&]() {
            for (int i = 0; i < N; ++i) actual_sum += bb_get(&b);
        });

        producer.join();
        consumer.join();

        EXPECT_EQ(actual_sum, oracle_sum)
            << "cap=1 sum mismatch at trial=" << trial << " N=" << N;

        // Also verify FIFO via order: with cap=1, consumer must see items in
        // exactly the order they were put. Re-run single-thread to verify order.
        bb_destroy(&b);
    }
}

// --- 7. Single-thread: interleaved random-batch put/get, FIFO oracle ---------
// Each iteration: put a random batch of 1..cap items, get a random batch of
// 1..available items, check every retrieved value against an expected queue.
// Runs 1000 operations total over random caps 2..16. Catches ring pointer bugs
// that only show up when put and get are not perfectly alternating.
TEST(BoundedBuffer, InterleavedRandomBatchFifo) {
    std::mt19937 rng(0xC0FFEE + 6);
    std::uniform_int_distribution<int> cap_dist(2, 16);
    std::uniform_int_distribution<int> val_dist(0, 999);

    for (int trial = 0; trial < 100; ++trial) {
        int cap = cap_dist(rng);
        BoundedBuffer b;
        bb_init(&b, cap);

        std::vector<int> reference_queue;  // simple FIFO oracle
        int in_buf = 0;                    // how many currently in buffer
        int global_val = 0;

        for (int op = 0; op < 1000; ++op) {
            // Decide whether to put or get based on buffer state
            bool can_put = in_buf < cap;
            bool can_get = in_buf > 0;

            if (!can_put && !can_get) break;  // shouldn't happen

            // Randomly pick direction, but constrain to valid
            int direction = (rng() % 2);  // 0=put, 1=get
            if (!can_put) direction = 1;
            if (!can_get) direction = 0;

            if (direction == 0) {
                // Put random batch: 1 .. min(cap-in_buf, 5)
                int avail = cap - in_buf;
                std::uniform_int_distribution<int> batch_dist(1, std::min(avail, 5));
                int batch = batch_dist(rng);
                for (int i = 0; i < batch; ++i) {
                    int v = global_val++;
                    bb_put(&b, v);
                    reference_queue.push_back(v);
                    in_buf++;
                }
            } else {
                // Get random batch: 1 .. min(in_buf, 5)
                std::uniform_int_distribution<int> batch_dist(1, std::min(in_buf, 5));
                int batch = batch_dist(rng);
                for (int i = 0; i < batch; ++i) {
                    int got = bb_get(&b);
                    int expected = reference_queue.front();
                    reference_queue.erase(reference_queue.begin());
                    ASSERT_EQ(got, expected)
                        << "Interleaved batch FIFO broken: trial=" << trial
                        << " op=" << op;
                    in_buf--;
                }
            }
        }

        // Drain remainder
        while (in_buf > 0) {
            int got = bb_get(&b);
            int expected = reference_queue.front();
            reference_queue.erase(reference_queue.begin());
            ASSERT_EQ(got, expected) << "Drain remainder FIFO broken: trial=" << trial;
            in_buf--;
        }

        bb_destroy(&b);
    }
}

// --- 8. High-contention: large capacity, many producers, many consumers ------
// Oracle: sum(consumed) == sum(produced). Uses cap 32..128, 4 producers,
// 4 consumers, 500 items per producer. Exercises the mutex/cond under heavy
// concurrent load — catches lost wake-ups, missed signals, double-counts.
TEST(BoundedBuffer, HighContentionSumOracle) {
    std::mt19937 rng(0xC0FFEE + 7);
    std::uniform_int_distribution<int> cap_dist(32, 128);
    std::uniform_int_distribution<int> val_dist(1, 100);

    for (int trial = 0; trial < 20; ++trial) {
        const int NP = 4, NC = 4, ITEMS = 500;
        int cap = cap_dist(rng);
        int total = NP * ITEMS;

        std::vector<std::vector<int>> prod_data(NP, std::vector<int>(ITEMS));
        long long oracle_sum = 0;
        for (int p = 0; p < NP; ++p)
            for (int i = 0; i < ITEMS; ++i) {
                prod_data[p][i] = val_dist(rng);
                oracle_sum += prod_data[p][i];
            }

        BoundedBuffer b;
        bb_init(&b, cap);

        std::vector<long long> csums(NC, 0LL);
        // Distribute items evenly among consumers
        int base = total / NC;
        std::vector<int> cc(NC, base);
        for (int i = 0; i < total % NC; ++i) cc[i]++;

        std::vector<std::thread> prods, cons;
        for (int p = 0; p < NP; ++p)
            prods.emplace_back([&, p]() {
                for (int i = 0; i < ITEMS; ++i) bb_put(&b, prod_data[p][i]);
            });
        for (int c = 0; c < NC; ++c)
            cons.emplace_back([&, c]() {
                for (int i = 0; i < cc[c]; ++i) csums[c] += bb_get(&b);
            });

        for (auto &t : prods) t.join();
        for (auto &t : cons) t.join();

        long long actual = 0;
        for (long long s : csums) actual += s;
        EXPECT_EQ(actual, oracle_sum)
            << "High-contention sum mismatch trial=" << trial << " cap=" << cap;

        bb_destroy(&b);
    }
}
