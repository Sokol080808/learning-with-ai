// Эти тесты трогать не нужно — это эталон поведения.
// Они на C++ (GoogleTest), но вызывают твои C-функции через extern "C".
#include <gtest/gtest.h>

extern "C" {
#include "scheduling.h"
}

// ---------- FCFS: среднее время ожидания ----------
// Все процессы пришли в момент 0 и считаются по порядку индексов.
// Время ожидания процесса = сумма длительностей всех, кто стоит перед ним.

TEST(FcfsAvgWaiting, OneProcessWaitsZero) {
    // Единственный процесс никого не ждёт.
    const int burst[] = {4};
    EXPECT_DOUBLE_EQ(fcfs_avg_waiting(burst, 1), 0.0);
}

TEST(FcfsAvgWaiting, EqualBursts) {
    // burst = {2, 2, 2}: ожидания 0, 2, 4 -> (0+2+4)/3 = 2.0
    const int burst[] = {2, 2, 2};
    EXPECT_DOUBLE_EQ(fcfs_avg_waiting(burst, 3), 2.0);
}

TEST(FcfsAvgWaiting, MixedBursts) {
    // burst = {5, 3, 8, 6}: ожидания 0, 5, 8, 16 -> (0+5+8+16)/4 = 29/4 = 7.25
    const int burst[] = {5, 3, 8, 6};
    EXPECT_DOUBLE_EQ(fcfs_avg_waiting(burst, 4), 7.25);
}

TEST(FcfsAvgWaiting, OrderMattersForWaitingSum) {
    // burst = {1, 100}: ожидания 0, 1 -> (0+1)/2 = 0.5
    // (короткий впереди -> маленькое среднее ожидание — на этом строится SJF)
    const int burst[] = {1, 100};
    EXPECT_DOUBLE_EQ(fcfs_avg_waiting(burst, 2), 0.5);
}

// ---------- Round-Robin: порядок исполнения по квантам ----------

TEST(RoundRobinOrder, QuantumCoversAllBecomesFcfs) {
    // Если квант >= любой длительности, каждый процесс отрабатывает за один заход:
    // порядок просто 0, 1.
    const int burst[] = {4, 4};
    int order[16] = {0};
    int len = -1;
    round_robin_order(burst, 2, 4, order, &len);

    ASSERT_EQ(len, 2);
    const int expected[] = {0, 1};
    for (int i = 0; i < len; ++i) EXPECT_EQ(order[i], expected[i]);
}

TEST(RoundRobinOrder, SingleProcessSplitByQuantum) {
    // Один процесс длиной 3 при кванте 1 исполняется тремя кусками: 0, 0, 0.
    const int burst[] = {3};
    int order[16] = {0};
    int len = -1;
    round_robin_order(burst, 1, 1, order, &len);

    ASSERT_EQ(len, 3);
    const int expected[] = {0, 0, 0};
    for (int i = 0; i < len; ++i) EXPECT_EQ(order[i], expected[i]);
}

TEST(RoundRobinOrder, ClassicRotation) {
    // burst = {5, 3, 1}, quantum = 2.
    // Остатки и круги:
    //   P0:5->3 [0], P1:3->1 [1], P2:1->0 [2],
    //   P0:3->1 [0], P1:1->0 [1], P0:1->0 [0]
    // Итог: 0,1,2,0,1,0  (длина 6)
    const int burst[] = {5, 3, 1};
    int order[32] = {0};
    int len = -1;
    round_robin_order(burst, 3, 2, order, &len);

    ASSERT_EQ(len, 6);
    const int expected[] = {0, 1, 2, 0, 1, 0};
    for (int i = 0; i < len; ++i) EXPECT_EQ(order[i], expected[i]);
}

TEST(RoundRobinOrder, QuantumOneRoundRobin) {
    // burst = {2, 1, 2}, quantum = 1.
    //   круг1: P0:2->1 [0], P1:1->0 [1], P2:2->1 [2]
    //   круг2: P0:1->0 [0],            P2:1->0 [2]
    // Итог: 0,1,2,0,2  (длина 5)
    const int burst[] = {2, 1, 2};
    int order[32] = {0};
    int len = -1;
    round_robin_order(burst, 3, 1, order, &len);

    ASSERT_EQ(len, 5);
    const int expected[] = {0, 1, 2, 0, 2};
    for (int i = 0; i < len; ++i) EXPECT_EQ(order[i], expected[i]);
}
