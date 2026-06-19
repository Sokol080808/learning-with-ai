#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <numeric>
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
