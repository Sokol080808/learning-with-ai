// Эти тесты трогать не нужно — это эталон поведения.
#include <gtest/gtest.h>

#include <numeric>
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
