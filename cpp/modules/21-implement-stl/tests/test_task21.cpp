#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <numeric>
#include <random>
#include <deque>
#include <list>
#include <algorithm>
#include <cstddef>
#include "task21.hpp"

// =====================================================================
// Задание 1. Vector<T>
// =====================================================================
TEST(Vector, StartsEmpty) {
    Vector<int> v;
    EXPECT_EQ(v.size(), 0u);
    EXPECT_TRUE(v.empty());
}

TEST(Vector, PushBackGrowsSize) {
    Vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    EXPECT_EQ(v.size(), 3u);
    EXPECT_FALSE(v.empty());
    EXPECT_EQ(v[0], 10);
    EXPECT_EQ(v[1], 20);
    EXPECT_EQ(v[2], 30);
}

TEST(Vector, CapacityDoublesByPolicy) {
    Vector<int> v;
    EXPECT_EQ(v.capacity(), 0u);
    v.push_back(1);
    EXPECT_EQ(v.capacity(), 1u);   // 0 -> 1
    v.push_back(2);
    EXPECT_EQ(v.capacity(), 2u);   // 1 -> 2
    v.push_back(3);
    EXPECT_EQ(v.capacity(), 4u);   // 2 -> 4
    v.push_back(4);
    EXPECT_EQ(v.capacity(), 4u);   // ещё помещается
    v.push_back(5);
    EXPECT_EQ(v.capacity(), 8u);   // 4 -> 8
    EXPECT_EQ(v.size(), 5u);
}

TEST(Vector, CapacityAlwaysAtLeastSize) {
    Vector<int> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
        EXPECT_GE(v.capacity(), v.size());
    }
    EXPECT_EQ(v.size(), 100u);
    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(v[static_cast<std::size_t>(i)], i);
}

TEST(Vector, PopBackShrinksSizeNotCapacity) {
    Vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    std::size_t cap_before = v.capacity();
    v.pop_back();
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v.capacity(), cap_before);  // pop не уменьшает capacity
    EXPECT_EQ(v[1], 2);
}

TEST(Vector, AtThrowsOutOfRange) {
    Vector<int> v;
    v.push_back(7);
    EXPECT_EQ(v.at(0), 7);
    EXPECT_THROW(v.at(1), std::out_of_range);
    EXPECT_THROW(v.at(100), std::out_of_range);
}

TEST(Vector, AtThrowsOnEmpty) {
    Vector<int> v;
    EXPECT_THROW(v.at(0), std::out_of_range);
}

TEST(Vector, IteratorsRangeFor) {
    Vector<int> v;
    for (int i = 1; i <= 5; ++i) v.push_back(i);
    int sum = 0;
    for (int x : v) sum += x;
    EXPECT_EQ(sum, 15);
    EXPECT_EQ(static_cast<std::size_t>(v.end() - v.begin()), v.size());
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 15);
}

TEST(Vector, WorksForStrings) {
    Vector<std::string> v;
    v.push_back("a");
    v.push_back("bb");
    v.push_back("ccc");
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[2], "ccc");
    EXPECT_EQ(v.at(1), "bb");
}

// =====================================================================
// Задание 2. SmallVector<T, N>
// =====================================================================
TEST(SmallVector, StaysInlineUntilFull) {
    SmallVector<int, 3> sv;
    EXPECT_EQ(sv.size(), 0u);
    EXPECT_EQ(sv.capacity(), 3u);
    EXPECT_FALSE(sv.on_heap());

    sv.push_back(1);
    sv.push_back(2);
    sv.push_back(3);
    EXPECT_EQ(sv.size(), 3u);
    EXPECT_FALSE(sv.on_heap());     // ещё во встроенном буфере
    EXPECT_EQ(sv.capacity(), 3u);
    EXPECT_EQ(sv[0], 1);
    EXPECT_EQ(sv[2], 3);
}

TEST(SmallVector, MovesToHeapOnOverflow) {
    SmallVector<int, 2> sv;
    sv.push_back(10);
    sv.push_back(20);
    EXPECT_FALSE(sv.on_heap());
    sv.push_back(30);               // переполнили встроенный буфер
    EXPECT_TRUE(sv.on_heap());
    EXPECT_GT(sv.capacity(), 2u);
    EXPECT_EQ(sv.size(), 3u);
    EXPECT_EQ(sv[0], 10);
    EXPECT_EQ(sv[1], 20);
    EXPECT_EQ(sv[2], 30);
}

TEST(SmallVector, KeepsDataAcrossManyPushes) {
    SmallVector<int, 4> sv;
    for (int i = 0; i < 50; ++i) sv.push_back(i * i);
    EXPECT_EQ(sv.size(), 50u);
    EXPECT_TRUE(sv.on_heap());
    for (int i = 0; i < 50; ++i)
        EXPECT_EQ(sv[static_cast<std::size_t>(i)], i * i);
}

TEST(SmallVector, PopBack) {
    SmallVector<int, 2> sv;
    sv.push_back(1);
    sv.push_back(2);
    sv.push_back(3);
    EXPECT_EQ(sv.size(), 3u);
    sv.pop_back();
    EXPECT_EQ(sv.size(), 2u);
    EXPECT_EQ(sv[1], 2);
}

// =====================================================================
// Задание 3. IntrusiveList<T>
// =====================================================================
TEST(IntrusiveList, PushBackAndSize) {
    IntrusiveList<int> lst;
    EXPECT_EQ(lst.size(), 0u);
    EXPECT_TRUE(lst.empty());

    Node<int> a{1}, b{2}, c{3};
    lst.push_back(&a);
    lst.push_back(&b);
    lst.push_back(&c);
    EXPECT_EQ(lst.size(), 3u);
    EXPECT_FALSE(lst.empty());
    EXPECT_EQ(lst.front(), 1);
    EXPECT_EQ(lst.back(), 3);
}

TEST(IntrusiveList, EraseMiddle) {
    IntrusiveList<int> lst;
    Node<int> a{1}, b{2}, c{3};
    lst.push_back(&a);
    lst.push_back(&b);
    lst.push_back(&c);
    lst.erase(&b);
    EXPECT_EQ(lst.size(), 2u);
    EXPECT_EQ(lst.front(), 1);
    EXPECT_EQ(lst.back(), 3);
}

TEST(IntrusiveList, EraseHeadAndTail) {
    IntrusiveList<int> lst;
    Node<int> a{10}, b{20}, c{30};
    lst.push_back(&a);
    lst.push_back(&b);
    lst.push_back(&c);

    lst.erase(&a);                 // снимаем голову
    EXPECT_EQ(lst.front(), 20);
    EXPECT_EQ(lst.size(), 2u);

    lst.erase(&c);                 // снимаем хвост
    EXPECT_EQ(lst.back(), 20);
    EXPECT_EQ(lst.front(), 20);
    EXPECT_EQ(lst.size(), 1u);

    lst.erase(&b);                 // снимаем последний
    EXPECT_EQ(lst.size(), 0u);
    EXPECT_TRUE(lst.empty());
}

TEST(IntrusiveList, ReinsertAfterErase) {
    IntrusiveList<int> lst;
    Node<int> a{1}, b{2};
    lst.push_back(&a);
    lst.push_back(&b);
    lst.erase(&a);
    lst.push_back(&a);             // вернули в конец
    EXPECT_EQ(lst.size(), 2u);
    EXPECT_EQ(lst.front(), 2);
    EXPECT_EQ(lst.back(), 1);
}

// =====================================================================
// Задание 6. IntrusiveList::iterator (forward) + Vector Rule of 5
// =====================================================================

// --- 6a. Forward-итератор IntrusiveList: range-for и std::distance. ---
TEST(IntrusiveListIterator, DistanceEqualsSize) {
    IntrusiveList<int> lst;
    Node<int> a{1}, b{2}, c{3}, d{4};
    lst.push_back(&a);
    lst.push_back(&b);
    lst.push_back(&c);
    lst.push_back(&d);
    // std::distance ходит по итератору, опираясь на iterator_category.
    EXPECT_EQ(static_cast<std::size_t>(std::distance(lst.begin(), lst.end())),
              lst.size());
}

TEST(IntrusiveListIterator, RangeForVisitsInOrder) {
    IntrusiveList<int> lst;
    Node<int> a{10}, b{20}, c{30};
    lst.push_back(&a);
    lst.push_back(&b);
    lst.push_back(&c);
    int sum = 0;
    std::vector<int> seen;
    for (int x : lst) {          // требует begin()/end() и operator*/++/!=
        sum += x;
        seen.push_back(x);
    }
    EXPECT_EQ(sum, 60);
    EXPECT_EQ(seen, (std::vector<int>{10, 20, 30}));
}

TEST(IntrusiveListIterator, EmptyListBeginEqualsEnd) {
    IntrusiveList<int> lst;
    EXPECT_EQ(lst.begin(), lst.end());
    EXPECT_EQ(std::distance(lst.begin(), lst.end()), 0);
}

TEST(IntrusiveListIterator, HasForwardIteratorTraits) {
    using It = IntrusiveList<int>::iterator;
    static_assert(std::is_same_v<It::iterator_category,
                                 std::forward_iterator_tag>,
                  "category must be forward_iterator_tag");
    static_assert(std::is_same_v<It::value_type, int>);
    static_assert(std::is_same_v<It::reference, int&>);
    static_assert(std::is_same_v<It::pointer, int*>);
    static_assert(std::is_same_v<It::difference_type, std::ptrdiff_t>);
    // iterator_traits должен видеть те же типы через сам итератор.
    static_assert(std::is_same_v<std::iterator_traits<It>::value_type, int>);
    SUCCEED();
}

TEST(IntrusiveListIterator, ArrowOperatorReachesMembers) {
    IntrusiveList<std::string> lst;
    Node<std::string> a{"hi"}, b{"world"};
    lst.push_back(&a);
    lst.push_back(&b);
    auto it = lst.begin();
    EXPECT_EQ(it->size(), 2u);   // "hi"
    ++it;
    EXPECT_EQ(it->size(), 5u);   // "world"
}

// --- 6b. Vector Rule of 5: глубокая копия + перемещение. ---
TEST(VectorRuleOf5, CopyIsDeepAndIndependent) {
    Vector<int> a;
    for (int i = 0; i < 5; ++i) a.push_back(i);
    Vector<int> b = a;                 // copy-конструктор
    EXPECT_EQ(b.size(), a.size());
    for (std::size_t i = 0; i < a.size(); ++i) EXPECT_EQ(a[i], b[i]);
    // Меняем копию — оригинал не должен пострадать (глубокая копия).
    b[0] = 999;
    b.push_back(42);
    EXPECT_EQ(a[0], 0);
    EXPECT_EQ(a.size(), 5u);
    EXPECT_EQ(b[0], 999);
    EXPECT_EQ(b.size(), 6u);
}

TEST(VectorRuleOf5, MoveTransfersAndEmptiesSource) {
    Vector<int> a;
    for (int i = 0; i < 4; ++i) a.push_back(i * 10);
    Vector<int> b = std::move(a);      // move-конструктор
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(b[3], 30);
    // Источник остаётся в валидном пустом состоянии.
    EXPECT_EQ(a.size(), 0u);
    EXPECT_EQ(a.capacity(), 0u);
    EXPECT_TRUE(a.empty());
    // ...и пригоден для повторного использования.
    a.push_back(7);
    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(a[0], 7);
}

TEST(VectorRuleOf5, CopyAssignReplacesContents) {
    Vector<int> a;
    for (int i = 0; i < 3; ++i) a.push_back(i);
    Vector<int> b;
    for (int i = 0; i < 10; ++i) b.push_back(-1);
    b = a;                              // copy-присваивание
    EXPECT_EQ(b.size(), 3u);
    for (std::size_t i = 0; i < a.size(); ++i) EXPECT_EQ(b[i], a[i]);
    b[0] = 555;                         // независимость после присваивания
    EXPECT_EQ(a[0], 0);
}

TEST(VectorRuleOf5, MoveAssignTransfersAndEmptiesSource) {
    Vector<int> a;
    for (int i = 0; i < 6; ++i) a.push_back(i);
    Vector<int> b;
    b.push_back(100);
    b = std::move(a);                   // move-присваивание
    EXPECT_EQ(b.size(), 6u);
    EXPECT_EQ(b[5], 5);
    EXPECT_EQ(a.size(), 0u);
    EXPECT_TRUE(a.empty());
}

TEST(VectorRuleOf5, SelfCopyAssignIsSafe) {
    Vector<int> a;
    for (int i = 0; i < 4; ++i) a.push_back(i);
    Vector<int>& ref = a;
    a = ref;                            // самоприсваивание не должно ломать
    EXPECT_EQ(a.size(), 4u);
    for (int i = 0; i < 4; ++i) EXPECT_EQ(a[static_cast<std::size_t>(i)], i);
}

TEST(VectorRuleOf5, WorksWithStringsDeepCopy) {
    Vector<std::string> a;
    a.push_back("alpha");
    a.push_back("beta");
    Vector<std::string> b = a;
    b[0] = "GAMMA";
    EXPECT_EQ(a[0], "alpha");
    EXPECT_EQ(b[0], "GAMMA");
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(b.size(), 2u);
}

TEST(VectorRuleOf5, NoexceptMoveEnablesContainerOptimizations) {
    // Move-конструктор/присваивание не должны бросать — это включает
    // move-if-noexcept в реальных контейнерах.
    static_assert(std::is_nothrow_move_constructible_v<Vector<int>>,
                  "Vector move-ctor must be noexcept");
    static_assert(std::is_nothrow_move_assignable_v<Vector<int>>,
                  "Vector move-assign must be noexcept");
    SUCCEED();
}

// =====================================================================
// Задание 4. LRUCache<K, V>
// =====================================================================
TEST(LRUCache, RejectsZeroCapacity) {
    EXPECT_THROW((LRUCache<int, int>(0)), std::out_of_range);
}

TEST(LRUCache, PutGetBasic) {
    LRUCache<int, std::string> c(2);
    c.put(1, "one");
    c.put(2, "two");
    EXPECT_EQ(c.size(), 2u);
    EXPECT_TRUE(c.contains(1));

    auto g = c.get(1);
    ASSERT_TRUE(g.has_value());
    EXPECT_EQ(*g, "one");

    auto miss = c.get(99);
    EXPECT_FALSE(miss.has_value());
}

TEST(LRUCache, EvictsLeastRecentlyUsed) {
    LRUCache<int, int> c(2);
    c.put(1, 100);
    c.put(2, 200);
    // Обращаемся к 1 -> теперь 2 самый давний.
    EXPECT_EQ(*c.get(1), 100);
    c.put(3, 300);                 // переполнение -> вытесняется 2
    EXPECT_FALSE(c.contains(2));
    EXPECT_TRUE(c.contains(1));
    EXPECT_TRUE(c.contains(3));
    EXPECT_EQ(c.size(), 2u);
}

TEST(LRUCache, PutUpdatesValueAndRecency) {
    LRUCache<int, int> c(2);
    c.put(1, 10);
    c.put(2, 20);
    c.put(1, 11);                  // обновляем значение и делаем 1 свежим
    EXPECT_EQ(c.size(), 2u);
    EXPECT_EQ(*c.get(1), 11);
    c.put(3, 30);                  // вытесняется 2 (самый давний)
    EXPECT_FALSE(c.contains(2));
    EXPECT_TRUE(c.contains(1));
}

TEST(LRUCache, GetMakesKeyFresh) {
    LRUCache<int, int> c(3);
    c.put(1, 1);
    c.put(2, 2);
    c.put(3, 3);
    EXPECT_EQ(*c.get(1), 1);       // 1 снова свежий; теперь давний — 2
    c.put(4, 4);                   // вытесняется 2
    EXPECT_FALSE(c.contains(2));
    EXPECT_TRUE(c.contains(1));
    EXPECT_TRUE(c.contains(3));
    EXPECT_TRUE(c.contains(4));
}

// =====================================================================
// Задание 5. RingBuffer<T, N>
// =====================================================================
TEST(RingBuffer, BasicPushPopFifo) {
    RingBuffer<int, 4> rb;
    EXPECT_TRUE(rb.empty());
    EXPECT_FALSE(rb.full());
    EXPECT_EQ(rb.capacity(), 4u);

    EXPECT_TRUE(rb.try_push(1));
    EXPECT_TRUE(rb.try_push(2));
    EXPECT_TRUE(rb.try_push(3));
    EXPECT_EQ(rb.size(), 3u);

    auto a = rb.try_pop();
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, 1);              // FIFO
    auto b = rb.try_pop();
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*b, 2);
    EXPECT_EQ(rb.size(), 1u);
}

TEST(RingBuffer, TryPushReturnsFalseWhenFull) {
    RingBuffer<int, 2> rb;
    EXPECT_TRUE(rb.try_push(1));
    EXPECT_TRUE(rb.try_push(2));
    EXPECT_TRUE(rb.full());
    EXPECT_FALSE(rb.try_push(3));  // не пишет, возвращает false
    EXPECT_EQ(rb.size(), 2u);
}

TEST(RingBuffer, PushThrowsWhenFull) {
    RingBuffer<int, 2> rb;
    rb.push(1);
    rb.push(2);
    EXPECT_THROW(rb.push(3), std::out_of_range);
}

TEST(RingBuffer, TryPopEmptyIsNullopt) {
    RingBuffer<int, 2> rb;
    EXPECT_FALSE(rb.try_pop().has_value());
}

TEST(RingBuffer, WrapsAround) {
    RingBuffer<int, 3> rb;
    // Заполняем, частично опустошаем, снова заполняем — индексы должны
    // «завернуться» по кругу и значения остаться корректными FIFO.
    rb.try_push(1);
    rb.try_push(2);
    rb.try_push(3);
    EXPECT_EQ(*rb.try_pop(), 1);
    EXPECT_EQ(*rb.try_pop(), 2);
    rb.try_push(4);
    rb.try_push(5);
    EXPECT_TRUE(rb.full());
    EXPECT_EQ(*rb.try_pop(), 3);
    EXPECT_EQ(*rb.try_pop(), 4);
    EXPECT_EQ(*rb.try_pop(), 5);
    EXPECT_TRUE(rb.empty());
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо новых фиксированных примеров мы прогоняем сотни случайных
// (но воспроизводимых) сценариев и на каждом шаге проверяем ИНВАРИАНТ —
// свойство, которое обязано выполняться при любой корректной реализации,
// сверяясь с эталоном из <vector>/<deque>/<list>/std-структур. Сид жёстко
// зафиксирован, поэтому CI никогда не «мигает».

// ---------------------------------------------------------------------
// Задание 1. Vector<T> — оракул против std::vector<int>.
// ---------------------------------------------------------------------
TEST(VectorProps, MatchesStdVectorUnderRandomOps) {
    std::mt19937 rng(0xC0FFEEu);
    for (int trial = 0; trial < 300; ++trial) {
        Vector<int>            mine;
        std::vector<int>       oracle;
        std::uniform_int_distribution<int> op(0, 3);
        std::uniform_int_distribution<int> val(-1000, 1000);

        int steps = static_cast<int>(rng() % 60u);
        for (int s = 0; s < steps; ++s) {
            int choice = op(rng);
            if (choice == 3 && !oracle.empty()) {
                // pop_back: оба контейнера сжимаются согласованно.
                mine.pop_back();
                oracle.pop_back();
            } else {
                int x = val(rng);
                mine.push_back(x);
                oracle.push_back(x);
            }
            // Инвариант размера и поэлементного совпадения.
            ASSERT_EQ(mine.size(), oracle.size());
            ASSERT_EQ(mine.empty(), oracle.empty());
            for (std::size_t i = 0; i < oracle.size(); ++i)
                ASSERT_EQ(mine[i], oracle[i]);
            // capacity никогда не меньше size.
            ASSERT_GE(mine.capacity(), mine.size());
        }
        // Итераторы дают тот же диапазон, что и std::accumulate по оракулу.
        long long sum_mine =
            std::accumulate(mine.begin(), mine.end(), 0LL);
        long long sum_orcl =
            std::accumulate(oracle.begin(), oracle.end(), 0LL);
        ASSERT_EQ(sum_mine, sum_orcl);
        ASSERT_EQ(static_cast<std::size_t>(mine.end() - mine.begin()),
                  mine.size());
    }
}

TEST(VectorProps, CapacityGrowthPolicyDoublesExactly) {
    // Свойство: ёмкость следует строго политике 0->1, иначе ×2, и растёт
    // только тогда, когда size её настигает. Перевыделение строго увеличивает.
    std::mt19937 rng(0x1234u);
    for (int trial = 0; trial < 200; ++trial) {
        Vector<int> v;
        std::size_t expected_cap = 0;
        int n = 1 + static_cast<int>(rng() % 200u);
        for (int i = 0; i < n; ++i) {
            std::size_t cap_before = v.capacity();
            v.push_back(i);
            if (v.size() > cap_before) {
                // Произошёл рост — проверяем точную политику.
                expected_cap = (cap_before == 0) ? 1 : cap_before * 2;
                ASSERT_EQ(v.capacity(), expected_cap);
                ASSERT_GT(v.capacity(), cap_before);
            } else {
                ASSERT_EQ(v.capacity(), cap_before);  // роста не было
            }
            ASSERT_GE(v.capacity(), v.size());
        }
    }
}

TEST(VectorProps, AtThrowsExactlyOutsideBounds) {
    // Граничное свойство at(): валиден ровно для [0, size), иначе out_of_range.
    std::mt19937 rng(0xBEEFu);
    for (int trial = 0; trial < 200; ++trial) {
        Vector<int> v;
        int n = static_cast<int>(rng() % 30u);
        for (int i = 0; i < n; ++i) v.push_back(i * 7);
        // Внутри границ — без исключения и верное значение.
        for (int i = 0; i < n; ++i)
            ASSERT_EQ(v.at(static_cast<std::size_t>(i)), i * 7);
        // На границе и за ней — out_of_range.
        EXPECT_THROW(v.at(static_cast<std::size_t>(n)), std::out_of_range);
        std::size_t far = static_cast<std::size_t>(n) +
                          (rng() % 50u) + 1u;
        EXPECT_THROW(v.at(far), std::out_of_range);
    }
}

TEST(VectorProps, EmptyEdgeCases) {
    Vector<int> v;
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), 0u);
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.begin(), v.end());
    EXPECT_THROW(v.at(0), std::out_of_range);
    // Размер 1 — минимальный непустой случай.
    v.push_back(42);
    EXPECT_EQ(v.size(), 1u);
    EXPECT_EQ(v.capacity(), 1u);
    EXPECT_EQ(v.at(0), 42);
    v.pop_back();
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.begin(), v.end());
}

// ---------------------------------------------------------------------
// Задание 2. SmallVector<T, N> — оракул против std::vector + проверка SBO.
// ---------------------------------------------------------------------
TEST(SmallVectorProps, MatchesStdVectorAndSboFlag) {
    std::mt19937 rng(0xABCDEFu);
    constexpr std::size_t N = 4;
    for (int trial = 0; trial < 300; ++trial) {
        SmallVector<int, N> mine;
        std::vector<int>    oracle;
        std::uniform_int_distribution<int> op(0, 3);
        std::uniform_int_distribution<int> val(-500, 500);

        int steps = static_cast<int>(rng() % 50u);
        for (int s = 0; s < steps; ++s) {
            int choice = op(rng);
            if (choice == 3 && !oracle.empty()) {
                mine.pop_back();
                oracle.pop_back();
            } else {
                int x = val(rng);
                mine.push_back(x);
                oracle.push_back(x);
            }
            ASSERT_EQ(mine.size(), oracle.size());
            for (std::size_t i = 0; i < oracle.size(); ++i)
                ASSERT_EQ(mine[i], oracle[i]);
            // SBO: данные ушли в кучу => мы когда-то превышали N.
            // Если on_heap, то capacity > N; если нет — capacity == N.
            if (mine.on_heap())
                ASSERT_GT(mine.capacity(), N);
            else
                ASSERT_EQ(mine.capacity(), N);
        }
    }
}

TEST(SmallVectorProps, StaysInlineExactlyUpToN) {
    // Пока вставленных элементов <= N, данные обязаны быть во встроенном
    // буфере; ровно на (N+1)-м — уезжают в кучу.
    std::mt19937 rng(0x5151u);
    constexpr std::size_t N = 5;
    for (int trial = 0; trial < 200; ++trial) {
        SmallVector<int, N> sv;
        int n = static_cast<int>(rng() % 40u);
        for (int i = 0; i < n; ++i) {
            sv.push_back(i);
            if (sv.size() <= N) {
                ASSERT_FALSE(sv.on_heap());
                ASSERT_EQ(sv.capacity(), N);
            } else {
                ASSERT_TRUE(sv.on_heap());
                ASSERT_GT(sv.capacity(), N);
            }
        }
    }
}

// ---------------------------------------------------------------------
// Задание 3. IntrusiveList<T> — оракул против std::list, узлы живут в deque.
// (deque не инвалидирует элементы при росте, поэтому адреса узлов стабильны.)
// ---------------------------------------------------------------------
TEST(IntrusiveListProps, MatchesStdListUnderRandomInsertErase) {
    std::mt19937 rng(0xD00Du);
    for (int trial = 0; trial < 200; ++trial) {
        // Заранее аллоцируем пул узлов в deque, чтобы адреса не менялись.
        std::deque<Node<int>> pool;
        IntrusiveList<int>    mine;
        std::list<int>        oracle;
        // Параллельно держим список «живых» указателей в порядке списка.
        std::vector<Node<int>*> live_order;
        std::uniform_int_distribution<int> val(-1000, 1000);

        int steps = static_cast<int>(rng() % 60u);
        for (int s = 0; s < steps; ++s) {
            bool do_erase = (rng() % 3u == 0u) && !live_order.empty();
            if (do_erase) {
                std::size_t idx = rng() % live_order.size();
                Node<int>* victim = live_order[idx];
                mine.erase(victim);
                auto oit = oracle.begin();
                std::advance(oit, static_cast<std::ptrdiff_t>(idx));
                oracle.erase(oit);
                live_order.erase(live_order.begin() +
                                 static_cast<std::ptrdiff_t>(idx));
            } else {
                int x = val(rng);
                pool.emplace_back(x);
                Node<int>* node = &pool.back();
                mine.push_back(node);
                oracle.push_back(x);
                live_order.push_back(node);
            }
            // Инварианты: размер совпадает; front/back совпадают, если непусто.
            ASSERT_EQ(mine.size(), oracle.size());
            ASSERT_EQ(mine.empty(), oracle.empty());
            if (!oracle.empty()) {
                ASSERT_EQ(mine.front(), oracle.front());
                ASSERT_EQ(mine.back(),  oracle.back());
            }
            // Значения узлов в нашем порядке совпадают с эталоном поэлементно.
            auto oit = oracle.begin();
            for (Node<int>* p : live_order) {
                ASSERT_EQ(p->value, *oit);
                ++oit;
            }
        }
    }
}

TEST(IntrusiveListProps, EraseThenReinsertGoesToTail) {
    // Свойство: erase разрывает связи O(1), а повторный push_back ставит
    // узел в хвост; порядок остаётся консистентным со std::list.
    std::mt19937 rng(0x7E57u);
    for (int trial = 0; trial < 200; ++trial) {
        std::deque<Node<int>> pool;
        IntrusiveList<int>    mine;
        std::list<int>        oracle;
        std::vector<Node<int>*> live;
        int n = 1 + static_cast<int>(rng() % 10u);
        for (int i = 0; i < n; ++i) {
            pool.emplace_back(i);
            mine.push_back(&pool.back());
            oracle.push_back(i);
            live.push_back(&pool.back());
        }
        // Вынимаем случайный узел и сразу возвращаем в хвост.
        std::size_t idx = rng() % live.size();
        Node<int>* node = live[idx];
        int v = node->value;
        mine.erase(node);
        auto oit = oracle.begin();
        std::advance(oit, static_cast<std::ptrdiff_t>(idx));
        oracle.erase(oit);
        live.erase(live.begin() + static_cast<std::ptrdiff_t>(idx));

        mine.push_back(node);
        oracle.push_back(v);
        live.push_back(node);

        ASSERT_EQ(mine.size(), oracle.size());
        ASSERT_EQ(mine.front(), oracle.front());
        ASSERT_EQ(mine.back(),  oracle.back());
        auto check = oracle.begin();
        for (Node<int>* p : live) {
            ASSERT_EQ(p->value, *check);
            ++check;
        }
    }
}

// ---------------------------------------------------------------------
// Задание 6a. IntrusiveList::iterator — обход через итератор сверяем с
// прямым обходом узлов: distance == size, сумма по range-for == сумма пула.
// ---------------------------------------------------------------------
TEST(IntrusiveListIteratorProps, TraversalMatchesOracle) {
    std::mt19937 rng(0x17E7u);
    for (int trial = 0; trial < 200; ++trial) {
        std::deque<Node<int>> pool;          // стабильные адреса узлов
        IntrusiveList<int>    mine;
        std::vector<int>      oracle;         // эталонный порядок значений
        std::uniform_int_distribution<int> val(-1000, 1000);

        int n = static_cast<int>(rng() % 40u);
        for (int i = 0; i < n; ++i) {
            int x = val(rng);
            pool.emplace_back(x);
            mine.push_back(&pool.back());
            oracle.push_back(x);
        }
        // distance по forward-итератору == размеру.
        ASSERT_EQ(static_cast<std::size_t>(
                      std::distance(mine.begin(), mine.end())),
                  mine.size());
        ASSERT_EQ(mine.size(), oracle.size());
        // range-for выдаёт ровно те же значения в том же порядке.
        std::vector<int> collected;
        for (int x : mine) collected.push_back(x);
        ASSERT_EQ(collected, oracle);
        // И std::accumulate по итераторам совпадает с эталоном.
        long long sum_mine =
            std::accumulate(mine.begin(), mine.end(), 0LL);
        long long sum_orcl =
            std::accumulate(oracle.begin(), oracle.end(), 0LL);
        ASSERT_EQ(sum_mine, sum_orcl);
    }
}

// ---------------------------------------------------------------------
// Задание 6b. Vector Rule of 5 — копия/перемещение сохраняют содержимое
// (round-trip против std::vector), а источник move обнуляется.
// ---------------------------------------------------------------------
TEST(VectorRuleOf5Props, CopyAndMovePreserveContents) {
    std::mt19937 rng(0x5A1Eu);
    for (int trial = 0; trial < 300; ++trial) {
        Vector<int>      src;
        std::vector<int> oracle;
        int n = static_cast<int>(rng() % 60u);
        std::uniform_int_distribution<int> val(-1000, 1000);
        for (int i = 0; i < n; ++i) {
            int x = val(rng);
            src.push_back(x);
            oracle.push_back(x);
        }

        // 1) Глубокая копия совпадает с оракулом поэлементно.
        Vector<int> copy = src;
        ASSERT_EQ(copy.size(), oracle.size());
        for (std::size_t i = 0; i < oracle.size(); ++i)
            ASSERT_EQ(copy[i], oracle[i]);

        // 2) Независимость: мутация копии не трогает оригинал.
        if (!copy.empty()) {
            copy[0] = copy[0] + 1;
            ASSERT_EQ(static_cast<std::size_t>(src.size()), oracle.size());
            for (std::size_t i = 0; i < oracle.size(); ++i)
                ASSERT_EQ(src[i], oracle[i]);
        }

        // 3) Move переносит содержимое и обнуляет источник.
        Vector<int> moved = std::move(src);
        ASSERT_EQ(moved.size(), oracle.size());
        for (std::size_t i = 0; i < oracle.size(); ++i)
            ASSERT_EQ(moved[i], oracle[i]);
        ASSERT_EQ(src.size(), 0u);
        ASSERT_EQ(src.capacity(), 0u);
        ASSERT_TRUE(src.empty());

        // 4) Move-присваивание в уже занятый вектор — round-trip сохраняется.
        Vector<int> dst;
        dst.push_back(-7);
        dst = std::move(moved);
        ASSERT_EQ(dst.size(), oracle.size());
        for (std::size_t i = 0; i < oracle.size(); ++i)
            ASSERT_EQ(dst[i], oracle[i]);
    }
}

// ---------------------------------------------------------------------
// Задание 4. LRUCache<K,V> — оракул-симулятор LRU.
// ---------------------------------------------------------------------
namespace {
// Простая эталонная модель LRU: список ключей от свежего (front) к давнему
// (back), плюс карта значений. Меняет O(n), но для теста это неважно.
struct LruOracle {
    std::size_t cap;
    std::list<int> order;                 // front = свежий
    std::unordered_map<int, int> values;  // key -> value

    explicit LruOracle(std::size_t c) : cap(c) {}

    void touch(int k) {
        order.remove(k);
        order.push_front(k);
    }
    void put(int k, int v) {
        if (values.count(k)) {
            values[k] = v;
            touch(k);
            return;
        }
        order.push_front(k);
        values[k] = v;
        if (values.size() > cap) {
            int victim = order.back();
            order.pop_back();
            values.erase(victim);
        }
    }
    bool get(int k, int& out) {
        auto it = values.find(k);
        if (it == values.end()) return false;
        out = it->second;
        touch(k);
        return true;
    }
    bool contains(int k) const { return values.count(k) != 0; }
    std::size_t size() const { return values.size(); }
};
} // namespace

TEST(LRUCacheProps, MatchesLruOracleUnderRandomTraffic) {
    std::mt19937 rng(0xCAFEu);
    for (int trial = 0; trial < 200; ++trial) {
        std::size_t cap = 1 + (rng() % 6u);
        LRUCache<int, int> mine(cap);
        LruOracle          oracle(cap);
        std::uniform_int_distribution<int> key(0, 9);  // малый домен => коллизии
        std::uniform_int_distribution<int> val(-100, 100);

        int steps = 30 + static_cast<int>(rng() % 70u);
        for (int s = 0; s < steps; ++s) {
            int k = key(rng);
            if (rng() % 2u == 0u) {
                int v = val(rng);
                mine.put(k, v);
                oracle.put(k, v);
            } else {
                auto g = mine.get(k);
                int ov;
                bool ok = oracle.get(k, ov);
                ASSERT_EQ(g.has_value(), ok);
                if (ok) ASSERT_EQ(*g, ov);
            }
            // Инварианты после каждого шага.
            ASSERT_EQ(mine.size(), oracle.size());
            ASSERT_LE(mine.size(), cap);             // никогда не превышаем cap
            ASSERT_EQ(mine.capacity(), cap);
            // Присутствие ключей совпадает по всему домену.
            for (int probe = 0; probe <= 9; ++probe)
                ASSERT_EQ(mine.contains(probe), oracle.contains(probe))
                    << "key=" << probe;
        }
    }
}

TEST(LRUCacheProps, CapacityOneAlwaysHoldsLastKey) {
    // Граничный случай cap==1: после любой вставки кэш содержит ровно
    // последний вставленный ключ и ничего больше.
    std::mt19937 rng(0x0001u);
    for (int trial = 0; trial < 200; ++trial) {
        LRUCache<int, int> c(1);
        int last_key = -1;
        int steps = 1 + static_cast<int>(rng() % 30u);
        for (int s = 0; s < steps; ++s) {
            int k = static_cast<int>(rng() % 5u);
            int v = static_cast<int>(rng() % 1000u);
            c.put(k, v);
            last_key = k;
            ASSERT_EQ(c.size(), 1u);
            ASSERT_TRUE(c.contains(last_key));
            auto g = c.get(last_key);
            ASSERT_TRUE(g.has_value());
            ASSERT_EQ(*g, v);
        }
    }
}

TEST(LRUCacheProps, ZeroCapacityThrowsOutOfRange) {
    EXPECT_THROW((LRUCache<int, int>(0)), std::out_of_range);
    EXPECT_THROW((LRUCache<std::string, int>(0)), std::out_of_range);
}

// ---------------------------------------------------------------------
// Задание 5. RingBuffer<T,N> — оракул против std::deque c границей N (FIFO).
// ---------------------------------------------------------------------
TEST(RingBufferProps, MatchesBoundedDequeFifo) {
    std::mt19937 rng(0xF1F0u);
    constexpr std::size_t N = 6;
    for (int trial = 0; trial < 300; ++trial) {
        RingBuffer<int, N> mine;
        std::deque<int>    oracle;     // эталон FIFO с ручной границей N
        std::uniform_int_distribution<int> val(-1000, 1000);

        int steps = static_cast<int>(rng() % 80u);
        for (int s = 0; s < steps; ++s) {
            if (rng() % 2u == 0u) {
                // push-сторона: try_push должен совпасть с «есть ли место».
                int x = val(rng);
                bool space = oracle.size() < N;
                bool pushed = mine.try_push(x);
                ASSERT_EQ(pushed, space);
                if (space) oracle.push_back(x);
            } else {
                // pop-сторона: try_pop совпадает с «непусто» и значением FIFO.
                auto popped = mine.try_pop();
                if (oracle.empty()) {
                    ASSERT_FALSE(popped.has_value());
                } else {
                    ASSERT_TRUE(popped.has_value());
                    ASSERT_EQ(*popped, oracle.front());
                    oracle.pop_front();
                }
            }
            // Инварианты состояния.
            ASSERT_EQ(mine.size(), oracle.size());
            ASSERT_EQ(mine.empty(), oracle.empty());
            ASSERT_EQ(mine.full(), oracle.size() == N);
            ASSERT_EQ(mine.capacity(), N);
            ASSERT_LE(mine.size(), N);
        }
    }
}

TEST(RingBufferProps, PushThrowsExactlyWhenFull) {
    // Свойство: push бросает out_of_range тогда и только тогда, когда full().
    std::mt19937 rng(0x9090u);
    constexpr std::size_t N = 4;
    for (int trial = 0; trial < 200; ++trial) {
        RingBuffer<int, N> rb;
        std::size_t pushed = 0;
        int n = static_cast<int>(rng() % 12u);
        for (int i = 0; i < n; ++i) {
            if (pushed < N) {
                rb.push(i);            // место есть — не бросает
                ++pushed;
                ASSERT_EQ(rb.size(), pushed);
            } else {
                ASSERT_TRUE(rb.full());
                EXPECT_THROW(rb.push(i), std::out_of_range);
                ASSERT_EQ(rb.size(), N);   // отказ не изменил содержимое
            }
        }
    }
}

TEST(RingBufferProps, WrapAroundPreservesFifoOrder) {
    // Многократно переполняем и опустошаем буфер, проверяя, что «заворот»
    // индексов не ломает FIFO-порядок относительно эталонного deque.
    std::mt19937 rng(0x33CCu);
    constexpr std::size_t N = 5;
    for (int trial = 0; trial < 200; ++trial) {
        RingBuffer<int, N> rb;
        std::deque<int>    oracle;
        int rounds = 3 + static_cast<int>(rng() % 5u);
        int counter = 0;
        for (int r = 0; r < rounds; ++r) {
            // Заполняем до отказа произвольным числом элементов.
            int to_push = static_cast<int>(rng() % (N + 2u));
            for (int i = 0; i < to_push; ++i) {
                int x = counter++;
                if (oracle.size() < N) {
                    ASSERT_TRUE(rb.try_push(x));
                    oracle.push_back(x);
                } else {
                    ASSERT_FALSE(rb.try_push(x));
                }
            }
            // Снимаем произвольное число — проверяем FIFO.
            int to_pop = static_cast<int>(rng() % (N + 2u));
            for (int i = 0; i < to_pop; ++i) {
                auto got = rb.try_pop();
                if (oracle.empty()) {
                    ASSERT_FALSE(got.has_value());
                } else {
                    ASSERT_TRUE(got.has_value());
                    ASSERT_EQ(*got, oracle.front());
                    oracle.pop_front();
                }
            }
            ASSERT_EQ(rb.size(), oracle.size());
        }
        // Досушиваем остаток — порядок до самого конца совпадает.
        while (!oracle.empty()) {
            auto got = rb.try_pop();
            ASSERT_TRUE(got.has_value());
            ASSERT_EQ(*got, oracle.front());
            oracle.pop_front();
        }
        ASSERT_FALSE(rb.try_pop().has_value());  // пустой -> nullopt
    }
}
