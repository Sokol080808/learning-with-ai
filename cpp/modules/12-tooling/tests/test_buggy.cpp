#include <gtest/gtest.h>
#include "buggy.hpp"

#include <climits>     // INT_MAX
#include <stdexcept>   // std::out_of_range
#include <random>      // std::mt19937, распределения для property-тестов
#include <vector>      // std::vector
#include <string>      // std::string
#include <algorithm>   // std::max_element, std::is_permutation, std::sort
#include <cstdint>     // std::int64_t
#include <cmath>       // std::sqrt

TEST(Buggy, SumFirstN) {
    EXPECT_EQ(sum_first_n({1, 2, 3, 4, 5}, 3), 6);   // 1+2+3
    EXPECT_EQ(sum_first_n({10, 20}, 0), 0);
    EXPECT_EQ(sum_first_n({5}, 1), 5);
}

TEST(Buggy, Repeat) {
    EXPECT_EQ(repeat("ab", 3), "ababab");
    EXPECT_EQ(repeat("x", 1), "x");
    EXPECT_EQ(repeat("y", 0), "");
}

TEST(Buggy, MaxIn) {
    EXPECT_EQ(max_in({3, 7, 1}), 7);
    EXPECT_EQ(max_in({-5, -2, -9}), -2);  // все отрицательные
}

TEST(Buggy, HasDuplicate) {
    EXPECT_FALSE(has_duplicate({1, 2, 3}));
    EXPECT_TRUE(has_duplicate({1, 2, 2}));
    EXPECT_FALSE(has_duplicate({}));
}

TEST(Buggy, Average) {
    EXPECT_DOUBLE_EQ(average({1, 2, 3, 4}), 2.5);
    EXPECT_DOUBLE_EQ(average({2, 4}), 3.0);
    EXPECT_DOUBLE_EQ(average({5}), 5.0);
}

// ===== Новые задания (посложнее) =====

// Задание 6. checked_at: корректный доступ в пределах границ.
TEST(CheckedAt, ReturnsElementInRange) {
    std::vector<int> v{10, 20, 30};
    EXPECT_EQ(checked_at(v, 0), 10);
    EXPECT_EQ(checked_at(v, 1), 20);
    EXPECT_EQ(checked_at(v, 2), 30);
}

// Задание 6. checked_at: выход за границу — именно std::out_of_range (НЕ logic_error).
TEST(CheckedAt, ThrowsOutOfRange) {
    std::vector<int> v{1, 2, 3};
    EXPECT_THROW(checked_at(v, 3), std::out_of_range);   // i == size
    EXPECT_THROW(checked_at(v, 100), std::out_of_range); // далеко за границей
}

// Задание 6. checked_at: на пустом векторе любой индекс вне границ.
TEST(CheckedAt, EmptyVectorAlwaysThrows) {
    std::vector<int> empty;
    EXPECT_THROW(checked_at(empty, 0), std::out_of_range);
}

// Задание 7. int_sqrt: точные квадраты.
TEST(IntSqrt, PerfectSquares) {
    EXPECT_EQ(int_sqrt(0), 0);
    EXPECT_EQ(int_sqrt(1), 1);
    EXPECT_EQ(int_sqrt(4), 2);
    EXPECT_EQ(int_sqrt(9), 3);
    EXPECT_EQ(int_sqrt(16), 4);
    EXPECT_EQ(int_sqrt(144), 12);
}

// Задание 7. int_sqrt: округление вниз для неполных квадратов.
TEST(IntSqrt, FloorsNonSquares) {
    EXPECT_EQ(int_sqrt(2), 1);
    EXPECT_EQ(int_sqrt(3), 1);
    EXPECT_EQ(int_sqrt(8), 2);
    EXPECT_EQ(int_sqrt(15), 3);
    EXPECT_EQ(int_sqrt(99), 9);
    EXPECT_EQ(int_sqrt(120), 10);  // 10*10=100 <= 120 < 121=11*11
}

// Задание 7. int_sqrt: большие значения у границы int (проверяет отсутствие переполнения r*r).
TEST(IntSqrt, LargeNoOverflow) {
    EXPECT_EQ(int_sqrt(2147395600), 46340);          // 46340^2 = 2147395600
    EXPECT_EQ(int_sqrt(INT_MAX), 46340);             // 46341^2 переполнил бы int
    EXPECT_EQ(int_sqrt(1000000), 1000);
}

// Задание 8. midpoint_int: обычные случаи и округление вниз.
TEST(MidpointInt, BasicCases) {
    EXPECT_EQ(midpoint_int(2, 4), 3);
    EXPECT_EQ(midpoint_int(1, 4), 2);   // 2.5 -> вниз к 2
    EXPECT_EQ(midpoint_int(0, 0), 0);
    EXPECT_EQ(midpoint_int(3, 3), 3);
    EXPECT_EQ(midpoint_int(0, 10), 5);
}

// Задание 8. midpoint_int: отрицательные и смешанные знаки.
TEST(MidpointInt, NegativeAndMixed) {
    EXPECT_EQ(midpoint_int(-4, -2), -3);
    EXPECT_EQ(midpoint_int(-2, 2), 0);
    EXPECT_EQ(midpoint_int(-3, 0), -2);  // -1.5 -> вниз к меньшему концу: -2
}

// Задание 8. midpoint_int: у границы INT_MAX, где (a+b)/2 переполняется (UB).
TEST(MidpointInt, NoOverflowNearMax) {
    EXPECT_EQ(midpoint_int(INT_MAX - 1, INT_MAX), INT_MAX - 1);
    EXPECT_EQ(midpoint_int(INT_MAX, INT_MAX), INT_MAX);
    EXPECT_EQ(midpoint_int(2000000000, 2000000000), 2000000000);
    EXPECT_EQ(midpoint_int(1000000000, 2000000000), 1500000000);
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо фиксированных примеров проверяем ИНВАРИАНТЫ на сотнях случайных
// входов с ЖЁСТКО зафиксированным сидом (CI никогда не флакает), а также явные
// крайние случаи. Где есть готовый «оракул» (независимая медленная формула,
// std::-функция или прямой пересчёт) — сверяем результат с ним.

// --- sum_first_n: сверка с независимым пересчётом и инкрементальная аддитивность ---
TEST(BuggyProps, SumFirstNMatchesNaiveOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len_dist(0, 60);
    std::uniform_int_distribution<int> val_dist(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        const int len = len_dist(rng);
        std::vector<int> v(static_cast<std::size_t>(len));
        for (int& x : v) x = val_dist(rng);

        std::uniform_int_distribution<int> n_dist(0, len);
        const int n = n_dist(rng);  // 0 <= n <= len: без выхода за границу

        // Оракул: честная сумма первых n элементов в int64, затем сужаем.
        std::int64_t oracle = 0;
        for (int i = 0; i < n; ++i) oracle += v[static_cast<std::size_t>(i)];

        EXPECT_EQ(sum_first_n(v, n), static_cast<int>(oracle))
            << "len=" << len << " n=" << n;

        // Инвариант-аддитивность: sum(n) == sum(n-1) + v[n-1].
        if (n >= 1) {
            EXPECT_EQ(sum_first_n(v, n), sum_first_n(v, n - 1) + v[static_cast<std::size_t>(n - 1)]);
        }
    }
}

TEST(BuggyProps, SumFirstNEdgeCases) {
    EXPECT_EQ(sum_first_n({}, 0), 0);                 // пустой вектор, n=0
    EXPECT_EQ(sum_first_n({42}, 0), 0);               // n=0 всегда 0
    EXPECT_EQ(sum_first_n({7, 8, 9}, 3), 24);         // n == size
    EXPECT_EQ(sum_first_n({-5, -5, -5, -5}, 4), -20); // все отрицательные
}

// --- repeat: длина, идемпотентность по конкатенации, дистрибутивность ---
TEST(BuggyProps, RepeatLengthAndConcatInvariant) {
    std::mt19937 rng(0x1234ABCDu);
    std::uniform_int_distribution<int> times_dist(0, 40);
    std::uniform_int_distribution<int> slen_dist(0, 6);
    std::uniform_int_distribution<int> ch_dist('a', 'z');

    for (int iter = 0; iter < 400; ++iter) {
        const int slen = slen_dist(rng);
        std::string s(static_cast<std::size_t>(slen), 'x');
        for (char& c : s) c = static_cast<char>(ch_dist(rng));

        const int times = times_dist(rng);
        const std::string r = repeat(s, times);

        // Инвариант 1: длина ровно s.size() * times.
        EXPECT_EQ(r.size(), s.size() * static_cast<std::size_t>(times))
            << "s='" << s << "' times=" << times;

        // Инвариант 2: каждый блок длины s.size() равен s (результат — конкатенация копий).
        for (int k = 0; k < times; ++k) {
            EXPECT_EQ(r.substr(static_cast<std::size_t>(k) * s.size(), s.size()), s);
        }

        // Инвариант 3 (дистрибутивность): repeat(s, a+b) == repeat(s,a) + repeat(s,b).
        std::uniform_int_distribution<int> split_dist(0, times);
        const int a = split_dist(rng);
        const int b = times - a;
        EXPECT_EQ(repeat(s, times), repeat(s, a) + repeat(s, b));
    }
}

TEST(BuggyProps, RepeatEdgeCases) {
    EXPECT_EQ(repeat("", 5), "");        // пустая строка повторяется в пустую
    EXPECT_EQ(repeat("ab", 0), "");      // times=0 -> пусто
    EXPECT_EQ(repeat("", 0), "");        // обе границы
    EXPECT_EQ(repeat("z", 1), "z");      // одна копия
}

// --- max_in: сверка с std::max_element + инвариант принадлежности ---
TEST(BuggyProps, MaxInMatchesStdMaxElement) {
    std::mt19937 rng(0xBADF00Du);
    std::uniform_int_distribution<int> len_dist(1, 80);   // вектор непустой по контракту
    std::uniform_int_distribution<int> val_dist(-100000, 100000);

    for (int iter = 0; iter < 400; ++iter) {
        const int len = len_dist(rng);
        std::vector<int> v(static_cast<std::size_t>(len));
        for (int& x : v) x = val_dist(rng);

        const int got = max_in(v);
        const int oracle = *std::max_element(v.begin(), v.end());

        EXPECT_EQ(got, oracle);  // сверка с оракулом std::

        // Инвариант: максимум >= любого элемента И присутствует в векторе.
        bool present = false;
        for (int x : v) {
            EXPECT_GE(got, x);
            if (x == got) present = true;
        }
        EXPECT_TRUE(present);
    }
}

TEST(BuggyProps, MaxInEdgeCases) {
    EXPECT_EQ(max_in({INT_MAX}), INT_MAX);            // одиночный максимум int
    EXPECT_EQ(max_in({INT_MIN}), INT_MIN);            // одиночный минимум int
    EXPECT_EQ(max_in({-3, -3, -3}), -3);              // все равны (дубликаты)
    EXPECT_EQ(max_in({INT_MIN, INT_MAX}), INT_MAX);   // крайние значения вместе
    EXPECT_EQ(max_in({5}), 5);                        // размер 1
}

// --- has_duplicate: сверка с эталоном через сортировку (O(n log n)) ---
TEST(BuggyProps, HasDuplicateMatchesSortOracle) {
    std::mt19937 rng(0xFEEDBEEFu);
    std::uniform_int_distribution<int> len_dist(0, 40);

    for (int iter = 0; iter < 400; ++iter) {
        const int len = len_dist(rng);
        // Узкий диапазон значений повышает шанс коллизий, чтобы покрыть обе ветки.
        std::uniform_int_distribution<int> val_dist(0, len > 0 ? len : 1);

        std::vector<int> v(static_cast<std::size_t>(len));
        for (int& x : v) x = val_dist(rng);

        // Оракул: отсортировать копию и поискать соседние равные.
        std::vector<int> sorted = v;
        std::sort(sorted.begin(), sorted.end());
        bool oracle = false;
        for (std::size_t i = 1; i < sorted.size(); ++i) {
            if (sorted[i] == sorted[i - 1]) { oracle = true; break; }
        }

        EXPECT_EQ(has_duplicate(v), oracle) << "len=" << len;
    }
}

TEST(BuggyProps, HasDuplicateConstructedInvariants) {
    std::mt19937 rng(0x0D15EA5Eu);
    std::uniform_int_distribution<int> len_dist(0, 50);
    std::uniform_int_distribution<int> val_dist(-1000000, 1000000);

    for (int iter = 0; iter < 300; ++iter) {
        const int len = len_dist(rng);

        // Строим вектор из ГАРАНТИРОВАННО различных значений (шаг по уникальному ряду).
        std::vector<int> uniq;
        uniq.reserve(static_cast<std::size_t>(len));
        int next = val_dist(rng);
        for (int i = 0; i < len; ++i) { uniq.push_back(next); next += 1 + (i % 7); }

        // Инвариант: набор различных значений не содержит дубликатов.
        EXPECT_FALSE(has_duplicate(uniq)) << "len=" << len;

        // Добавим копию случайного элемента -> дубликат обязан появиться (при len>=1).
        if (len >= 1) {
            std::uniform_int_distribution<int> idx(0, len - 1);
            std::vector<int> with_dup = uniq;
            with_dup.push_back(uniq[static_cast<std::size_t>(idx(rng))]);
            EXPECT_TRUE(has_duplicate(with_dup));
        }
    }
}

// --- average: лежит между min и max, и точно совпадает с честным пересчётом ---
TEST(BuggyProps, AverageBoundedAndExact) {
    std::mt19937 rng(0xABCDEF01u);
    std::uniform_int_distribution<int> len_dist(1, 80);
    std::uniform_int_distribution<int> val_dist(-1000000, 1000000);

    for (int iter = 0; iter < 400; ++iter) {
        const int len = len_dist(rng);
        std::vector<int> v(static_cast<std::size_t>(len));
        for (int& x : v) x = val_dist(rng);

        const double avg = average(v);

        int lo = v[0], hi = v[0];
        std::int64_t sum = 0;
        for (int x : v) { sum += x; if (x < lo) lo = x; if (x > hi) hi = x; }

        // Инвариант 1: min <= average <= max.
        EXPECT_GE(avg, static_cast<double>(lo));
        EXPECT_LE(avg, static_cast<double>(hi));

        // Инвариант 2 (оракул): точное значение = sum / n как дробное деление.
        const double oracle = static_cast<double>(sum) / static_cast<double>(len);
        EXPECT_DOUBLE_EQ(avg, oracle);

        // Инвариант 3: среднее*n восстанавливает сумму (с допуском на double).
        EXPECT_NEAR(avg * static_cast<double>(len), static_cast<double>(sum), 1e-6);
    }
}

TEST(BuggyProps, AverageEdgeCases) {
    EXPECT_DOUBLE_EQ(average({0}), 0.0);                 // один нуль
    EXPECT_DOUBLE_EQ(average({7}), 7.0);                 // размер 1
    EXPECT_DOUBLE_EQ(average({-4, 4}), 0.0);             // симметрия вокруг нуля
    EXPECT_DOUBLE_EQ(average({1, 2}), 1.5);              // дробный результат
    // Большие значения: наивная int-сумма переполнилась бы, но реализация в long long.
    EXPECT_DOUBLE_EQ(average({2000000000, 2000000000}), 2000000000.0);
}

// --- checked_at: в границах == v[i]; вне границ == именно std::out_of_range ---
TEST(BuggyProps, CheckedAtInRangeMatchesIndexing) {
    std::mt19937 rng(0x5EED1234u);
    std::uniform_int_distribution<int> len_dist(1, 60);
    std::uniform_int_distribution<int> val_dist(-1000000, 1000000);

    for (int iter = 0; iter < 400; ++iter) {
        const int len = len_dist(rng);
        std::vector<int> v(static_cast<std::size_t>(len));
        for (int& x : v) x = val_dist(rng);

        // В границах: checked_at(v, i) == v[i] для случайного валидного i.
        std::uniform_int_distribution<std::size_t> idx(0, static_cast<std::size_t>(len) - 1);
        const std::size_t i = idx(rng);
        EXPECT_EQ(checked_at(v, i), v[i]);

        // Вне границ: i == size и i = size + смещение — оба бросают std::out_of_range.
        std::uniform_int_distribution<std::size_t> over(0, 1000);
        EXPECT_THROW(checked_at(v, static_cast<std::size_t>(len)), std::out_of_range);
        EXPECT_THROW(checked_at(v, static_cast<std::size_t>(len) + over(rng)), std::out_of_range);
    }
}

TEST(BuggyProps, CheckedAtEdgeCases) {
    std::vector<int> empty;
    EXPECT_THROW(checked_at(empty, 0), std::out_of_range);  // пустой: индекс 0 уже вне границ
    std::vector<int> one{99};
    EXPECT_EQ(checked_at(one, 0), 99);                       // единственный валидный индекс
    EXPECT_THROW(checked_at(one, 1), std::out_of_range);     // i == size
    // Очень большой индекс не должен «обернуться» и случайно попасть в границы.
    EXPECT_THROW(checked_at(one, static_cast<std::size_t>(-1)), std::out_of_range);
}

// --- int_sqrt: определяющий инвариант r*r <= n < (r+1)*(r+1) + сверка со std::sqrt ---
TEST(BuggyProps, IntSqrtDefiningInvariant) {
    std::mt19937 rng(0x71777777u);
    std::uniform_int_distribution<int> n_dist(0, INT_MAX);

    for (int iter = 0; iter < 500; ++iter) {
        const int n = n_dist(rng);
        const int r = int_sqrt(n);

        // Базовый инвариант: r >= 0.
        EXPECT_GE(r, 0);

        // Определяющие неравенства (в int64, чтобы не переполниться у границы).
        const std::int64_t rr = static_cast<std::int64_t>(r) * r;
        const std::int64_t r1 = static_cast<std::int64_t>(r + 1) * (r + 1);
        EXPECT_LE(rr, static_cast<std::int64_t>(n));    // r*r <= n
        EXPECT_GT(r1, static_cast<std::int64_t>(n));    // (r+1)^2 > n  => r максимально

        // Сверка с оракулом std::sqrt (с поправкой на округление double у границы).
        int approx = static_cast<int>(std::sqrt(static_cast<double>(n)));
        while (static_cast<std::int64_t>(approx) * approx > n) --approx;
        while (static_cast<std::int64_t>(approx + 1) * (approx + 1) <= n) ++approx;
        EXPECT_EQ(r, approx) << "n=" << n;
    }
}

TEST(BuggyProps, IntSqrtPerfectSquaresRoundTrip) {
    std::mt19937 rng(0x9A9A9A9Au);
    // r до 46340: r*r ещё помещается в int (46341^2 > INT_MAX).
    std::uniform_int_distribution<int> r_dist(0, 46340);

    for (int iter = 0; iter < 400; ++iter) {
        const int r = r_dist(rng);
        const int sq = r * r;

        // Round-trip: на точном квадрате int_sqrt возвращает исходный корень.
        EXPECT_EQ(int_sqrt(sq), r);

        // На sq-1 (если есть) корень строго меньше r.
        if (sq >= 1) EXPECT_EQ(int_sqrt(sq - 1), r - 1);

        // Монотонность: int_sqrt не убывает при росте аргумента.
        if (r >= 1) {
            const int prev = (r - 1) * (r - 1);
            EXPECT_LE(int_sqrt(prev), int_sqrt(sq));
        }
    }
}

TEST(BuggyProps, IntSqrtEdgeCases) {
    EXPECT_EQ(int_sqrt(0), 0);
    EXPECT_EQ(int_sqrt(1), 1);
    EXPECT_EQ(int_sqrt(2), 1);
    EXPECT_EQ(int_sqrt(INT_MAX), 46340);              // у самой границы, без переполнения r*r
    EXPECT_EQ(int_sqrt(2147395600), 46340);           // 46340^2 ровно
    EXPECT_EQ(int_sqrt(2147395599), 46339);           // на единицу меньше точного квадрата
}

// --- midpoint_int: корректность без переполнения + симметрия и границы ---
// ВАЖНО: контракт функции — a <= b, и реализация a + (b - a)/2 безопасна только
// пока (b - a) помещается в int. Сам пролёт INT_MIN..INT_MAX даёт переполнение
// (b - a) и это UB вызывающего кода, а не задача функции — поэтому генерируем
// пары с ОГРАНИЧЕННЫМ разбросом (b - a в [0, INT_MAX]), как и обещано в .hpp.
TEST(BuggyProps, MidpointIntMatchesInt64Oracle) {
    std::mt19937 rng(0xDEADC0DEu);
    std::uniform_int_distribution<int> base_dist(INT_MIN, INT_MAX);
    std::uniform_int_distribution<int> span_dist(0, INT_MAX);  // b - a гарантированно влезает в int

    for (int iter = 0; iter < 500; ++iter) {
        const int a = base_dist(rng);
        const int span = span_dist(rng);
        // b = a + span, но без выхода за INT_MAX: усекаем span до доступного запаса.
        const std::int64_t room = static_cast<std::int64_t>(INT_MAX) - a;
        const int eff_span = static_cast<int>(std::min<std::int64_t>(span, room));
        const int b = a + eff_span;  // a <= b, и (b - a) = eff_span <= INT_MAX

        const int got = midpoint_int(a, b);

        // Оракул: считаем в int64 «к меньшему концу» (floor-деление при a<=b совпадает).
        const std::int64_t oracle = static_cast<std::int64_t>(a) +
                                    (static_cast<std::int64_t>(b) - a) / 2;
        EXPECT_EQ(static_cast<std::int64_t>(got), oracle) << "a=" << a << " b=" << b;

        // Инвариант: середина лежит в [a, b].
        EXPECT_GE(got, a);
        EXPECT_LE(got, b);

        // Инвариант: расстояние до концов отличается не больше чем на 1.
        const std::int64_t dl = static_cast<std::int64_t>(got) - a;
        const std::int64_t dr = static_cast<std::int64_t>(b) - got;
        EXPECT_LE(dl - dr, 1);
        EXPECT_GE(dl - dr, -1);
    }
}

TEST(BuggyProps, MidpointIntEdgeCases) {
    EXPECT_EQ(midpoint_int(INT_MIN, INT_MIN), INT_MIN);        // обе границы снизу
    EXPECT_EQ(midpoint_int(INT_MAX, INT_MAX), INT_MAX);        // обе границы сверху
    EXPECT_EQ(midpoint_int(0, INT_MAX), INT_MAX / 2);          // полупролёт, (b-a) ещё влезает в int
    EXPECT_EQ(midpoint_int(0, 1), 0);                          // округление вниз
    EXPECT_EQ(midpoint_int(-1, 0), -1);                        // -0.5 -> вниз к -1
    EXPECT_EQ(midpoint_int(INT_MIN, INT_MIN + 1), INT_MIN);    // соседние у нижней границы
    // Нижняя половина без переполнения (b - a) = INT_MAX, влезает в int.
    EXPECT_EQ(midpoint_int(INT_MIN, -1), INT_MIN / 2 - 1);
}
