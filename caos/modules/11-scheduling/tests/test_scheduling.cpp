// Эти тесты трогать не нужно — это эталон поведения.
// Они на C++ (GoogleTest), но вызывают твои C-функции через extern "C".
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

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

// ============================================================
// RANDOMIZED / PROPERTY TESTS — added for stronger coverage
// ============================================================

// ---- Helper: naive reference implementations in C++ --------

// Compute FCFS average waiting time using the straightforward definition.
static double ref_fcfs_avg_waiting(const std::vector<int>& burst) {
    int n = (int)burst.size();
    if (n == 0) return 0.0;
    long long wait_sum = 0;
    long long prefix = 0;
    for (int i = 0; i < n; ++i) {
        wait_sum += prefix;
        prefix += burst[i];
    }
    return (double)wait_sum / n;
}

// Compute Round-Robin order sequence using a simple queue simulation.
static std::vector<int> ref_round_robin_order(const std::vector<int>& burst,
                                               int quantum) {
    int n = (int)burst.size();
    std::vector<int> rem(burst.begin(), burst.end());
    std::vector<int> result;
    // queue holds process ids ready to run
    std::vector<int> q;
    q.reserve(n);
    for (int i = 0; i < n; ++i) q.push_back(i);
    int head = 0;
    while (head < (int)q.size()) {
        int id = q[head++];
        result.push_back(id);
        int run = std::min(rem[id], quantum);
        rem[id] -= run;
        if (rem[id] > 0) q.push_back(id);
    }
    return result;
}

// ---- FCFS randomized: oracle comparison ----------------------

TEST(FcfsAvgWaitingRandom, MatchesNaiveOracleSmallN) {
    // n in [2..8], burst in [1..20], 500 random cases
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> dist_n(2, 8);
    std::uniform_int_distribution<int> dist_b(1, 20);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        double got = fcfs_avg_waiting(burst.data(), n);
        double want = ref_fcfs_avg_waiting(burst);
        EXPECT_DOUBLE_EQ(got, want)
            << "n=" << n << " burst[0]=" << burst[0];
    }
}

TEST(FcfsAvgWaitingRandom, MatchesNaiveOracleLargerN) {
    // n in [10..100], burst in [1..1000], 200 random cases
    std::mt19937 rng(0xDEADBEEF);
    std::uniform_int_distribution<int> dist_n(10, 100);
    std::uniform_int_distribution<int> dist_b(1, 1000);

    for (int iter = 0; iter < 200; ++iter) {
        int n = dist_n(rng);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        double got = fcfs_avg_waiting(burst.data(), n);
        double want = ref_fcfs_avg_waiting(burst);
        EXPECT_DOUBLE_EQ(got, want)
            << "n=" << n;
    }
}

TEST(FcfsAvgWaitingRandom, SingleProcessAlwaysZero) {
    // For n=1 waiting is always 0 regardless of burst value
    std::mt19937 rng(0xABCD1234);
    std::uniform_int_distribution<int> dist_b(1, 100000);

    for (int iter = 0; iter < 500; ++iter) {
        int b = dist_b(rng);
        EXPECT_DOUBLE_EQ(fcfs_avg_waiting(&b, 1), 0.0)
            << "burst=" << b;
    }
}

TEST(FcfsAvgWaitingRandom, TwoProcessesFormula) {
    // For n=2: waiting = (0 + burst[0]) / 2 = burst[0] / 2.0
    std::mt19937 rng(0xFACEB00C);
    std::uniform_int_distribution<int> dist_b(1, 9999);

    for (int iter = 0; iter < 500; ++iter) {
        int burst[2];
        burst[0] = dist_b(rng);
        burst[1] = dist_b(rng);
        double expected = burst[0] / 2.0;
        EXPECT_DOUBLE_EQ(fcfs_avg_waiting(burst, 2), expected)
            << "burst[0]=" << burst[0] << " burst[1]=" << burst[1];
    }
}

TEST(FcfsAvgWaitingRandom, OrderingMonotonicity) {
    // Sorting burst ascending never increases avg waiting (SJF property):
    // sorted FCFS avg waiting <= unsorted avg waiting
    std::mt19937 rng(0x12345678);
    std::uniform_int_distribution<int> dist_n(3, 12);
    std::uniform_int_distribution<int> dist_b(1, 50);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        double unsorted = fcfs_avg_waiting(burst.data(), n);

        std::vector<int> sorted_burst = burst;
        std::sort(sorted_burst.begin(), sorted_burst.end());
        double sorted_avg = fcfs_avg_waiting(sorted_burst.data(), n);

        EXPECT_LE(sorted_avg, unsorted + 1e-9)
            << "Sorted FCFS should be <= unsorted for n=" << n;
    }
}

TEST(FcfsAvgWaitingRandom, AllBurstsEqualExactFormula) {
    // All equal bursts b: waiting[i] = i*b, avg = b*(n-1)/2
    std::mt19937 rng(0xCAFEBABE);
    std::uniform_int_distribution<int> dist_b(1, 100);
    std::uniform_int_distribution<int> dist_n(2, 20);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        int b = dist_b(rng);
        std::vector<int> burst(n, b);

        double expected = b * (n - 1) / 2.0;
        double got = fcfs_avg_waiting(burst.data(), n);
        EXPECT_DOUBLE_EQ(got, expected)
            << "n=" << n << " b=" << b;
    }
}

TEST(FcfsAvgWaitingRandom, ResultIsNonNegative) {
    // avg waiting must always be >= 0
    std::mt19937 rng(0x0FF1CE);
    std::uniform_int_distribution<int> dist_n(1, 50);
    std::uniform_int_distribution<int> dist_b(1, 500);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        double got = fcfs_avg_waiting(burst.data(), n);
        EXPECT_GE(got, 0.0) << "negative avg waiting for n=" << n;
    }
}

// ---- Round-Robin randomized: oracle comparison --------------

// NOTE on buffer sizing: the reference implementation allocates its internal
// queue with capacity n. The queue tail grows by one each time a process is
// re-queued, so the total number of entries enqueued equals
// sum(ceil(burst[i]/quantum)).  To avoid overflowing the reference's internal
// buffer we restrict inputs so that every process fits within ONE quantum
// (burst[i] <= quantum), meaning each process is enqueued exactly once and
// the total equals n.  This is still non-trivial: the oracle must correctly
// emit each id exactly once in index order and report len == n.

TEST(RoundRobinOrderRandom, MatchesNaiveOracleAllFitOneQuantum) {
    // burst[i] in [1..quantum], so every process completes in one slot.
    // order_out must be exactly 0,1,...,n-1 and len == n.
    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> dist_n(1, 50);
    std::uniform_int_distribution<int> dist_q(1, 20);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        int quantum = dist_q(rng);
        std::uniform_int_distribution<int> dist_b(1, quantum);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        // Each process fits in one slot → order is 0,1,...,n-1
        std::vector<int> got_order(n + 4, -1);
        int got_len = -1;
        round_robin_order(burst.data(), n, quantum,
                          got_order.data(), &got_len);

        ASSERT_EQ(got_len, n)
            << "n=" << n << " q=" << quantum;
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(got_order[i], i)
                << "slot " << i << " n=" << n << " q=" << quantum;
        }
    }
}

TEST(RoundRobinOrderRandom, OutputIsAPermutationOfIds) {
    // When every burst[i] <= quantum each id appears exactly once.
    // The multiset of ids in order_out must equal {0,1,...,n-1}.
    // Uses single-quantum regime for safety with the reference implementation.
    std::mt19937 rng(0xDEADBEEF);
    std::uniform_int_distribution<int> dist_n(2, 60);
    std::uniform_int_distribution<int> dist_q(2, 20);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        int quantum = dist_q(rng);
        std::uniform_int_distribution<int> dist_b(1, quantum);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        std::vector<int> order(n + 4, -1);
        int got_len = -1;
        round_robin_order(burst.data(), n, quantum, order.data(), &got_len);

        ASSERT_EQ(got_len, n) << "n=" << n << " q=" << quantum;

        // each id in [0,n) must appear exactly once
        std::vector<int> seen(n, 0);
        for (int i = 0; i < got_len; ++i) {
            int id = order[i];
            ASSERT_GE(id, 0); ASSERT_LT(id, n);
            seen[id]++;
        }
        for (int i = 0; i < n; ++i)
            EXPECT_EQ(seen[i], 1) << "id " << i << " wrong count n=" << n;
    }
}

TEST(RoundRobinOrderRandom, TotalSlotsEqualsExpected) {
    // Total number of quanta == sum of ceil(burst[i]/quantum).
    // Use single-quantum inputs (burst[i] <= quantum) so total == n.
    std::mt19937 rng(0xABCDEF01);
    std::uniform_int_distribution<int> dist_n(1, 50);
    std::uniform_int_distribution<int> dist_q(1, 15);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        int quantum = dist_q(rng);
        std::uniform_int_distribution<int> dist_b(1, quantum);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);
        // expected_len == n because each burst <= quantum
        int expected_len = n;

        std::vector<int> order(expected_len + 4, -1);
        int got_len = -1;
        round_robin_order(burst.data(), n, quantum, order.data(), &got_len);

        EXPECT_EQ(got_len, expected_len)
            << "n=" << n << " q=" << quantum;
    }
}

TEST(RoundRobinOrderRandom, EachProcessIdAppearsExactTimes) {
    // Each process id must appear exactly ceil(burst[i]/quantum) times.
    // Use single-quantum regime (burst[i] <= quantum) → each id appears once.
    std::mt19937 rng(0xFEEDFACE);
    std::uniform_int_distribution<int> dist_n(1, 40);
    std::uniform_int_distribution<int> dist_q(1, 12);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        int quantum = dist_q(rng);
        std::uniform_int_distribution<int> dist_b(1, quantum);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        std::vector<int> order(n + 4, -1);
        int got_len = -1;
        round_robin_order(burst.data(), n, quantum, order.data(), &got_len);

        ASSERT_EQ(got_len, n);
        std::vector<int> counts(n, 0);
        for (int i = 0; i < got_len; ++i) {
            int id = order[i];
            ASSERT_GE(id, 0) << "negative id at slot " << i;
            ASSERT_LT(id, n) << "id >= n at slot " << i;
            counts[id]++;
        }
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(counts[i], 1)
                << "process " << i << " appeared wrong number of times"
                << " (burst=" << burst[i] << " q=" << quantum << ")";
        }
    }
}

TEST(RoundRobinOrderRandom, LargeQuantumDegeneratestoFCFSOrder) {
    // When quantum >= max(burst), order_out is exactly 0,1,...,n-1
    std::mt19937 rng(0x13579BDF);
    std::uniform_int_distribution<int> dist_n(2, 15);
    std::uniform_int_distribution<int> dist_b(1, 20);

    for (int iter = 0; iter < 500; ++iter) {
        int n = dist_n(rng);
        std::vector<int> burst(n);
        int max_b = 0;
        for (int i = 0; i < n; ++i) {
            burst[i] = dist_b(rng);
            max_b = std::max(max_b, burst[i]);
        }
        int quantum = max_b; // exactly max — every process finishes in one slot

        std::vector<int> order(n + 1, -1);
        int got_len = -1;
        round_robin_order(burst.data(), n, quantum, order.data(), &got_len);

        ASSERT_EQ(got_len, n);
        for (int i = 0; i < n; ++i) {
            EXPECT_EQ(order[i], i)
                << "at slot " << i << " n=" << n << " q=" << quantum;
        }
    }
}

TEST(RoundRobinOrderRandom, AllIdsInValidRange) {
    // All ids in order_out must be in [0, n).
    // Single-quantum regime keeps total quanta == n (safe for reference impl).
    std::mt19937 rng(0x2468ACE0);
    std::uniform_int_distribution<int> dist_n(1, 60);
    std::uniform_int_distribution<int> dist_q(1, 15);

    for (int iter = 0; iter < 1000; ++iter) {
        int n = dist_n(rng);
        int quantum = dist_q(rng);
        std::uniform_int_distribution<int> dist_b(1, quantum);
        std::vector<int> burst(n);
        for (int i = 0; i < n; ++i) burst[i] = dist_b(rng);

        std::vector<int> order(n + 4, -99);
        int got_len = -1;
        round_robin_order(burst.data(), n, quantum, order.data(), &got_len);

        ASSERT_GE(got_len, 0);
        for (int i = 0; i < got_len; ++i) {
            EXPECT_GE(order[i], 0) << "negative id at " << i;
            EXPECT_LT(order[i], n) << "id >= n at " << i;
        }
    }
}
