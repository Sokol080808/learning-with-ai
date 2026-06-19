#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <stdexcept>
#include "task13.hpp"

TEST(Stack, PushTopPopSize) {
    Stack<int> s;
    EXPECT_TRUE(s.empty());
    s.push(1);
    s.push(2);
    s.push(3);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.top(), 3);
    s.pop();
    EXPECT_EQ(s.top(), 2);
    EXPECT_EQ(s.size(), 2u);
    EXPECT_FALSE(s.empty());
}

TEST(Stack, WorksForStrings) {
    Stack<std::string> s;
    s.push("a");
    s.push("b");
    EXPECT_EQ(s.top(), "b");
}

TEST(Algo, BinarySearch) {
    std::vector<int> v{1, 3, 5, 7, 9, 11};
    EXPECT_EQ(binary_search_idx(v, 1), 0);
    EXPECT_EQ(binary_search_idx(v, 7), 3);
    EXPECT_EQ(binary_search_idx(v, 11), 5);
    EXPECT_EQ(binary_search_idx(v, 4), -1);
    EXPECT_EQ(binary_search_idx({}, 1), -1);
}

TEST(Algo, InsertionSort) {
    EXPECT_EQ(insertion_sort({3, 1, 4, 1, 5, 9, 2, 6}),
              (std::vector<int>{1, 1, 2, 3, 4, 5, 6, 9}));
    EXPECT_EQ(insertion_sort({}), (std::vector<int>{}));
    EXPECT_EQ(insertion_sort({42}), (std::vector<int>{42}));
    EXPECT_EQ(insertion_sort({5, 4, 3, 2, 1}), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(Algo, TwoSum) {
    auto r = two_sum({2, 7, 11, 15}, 9);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->first, 0);
    EXPECT_EQ(r->second, 1);

    auto r2 = two_sum({3, 2, 4}, 6);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->first, 1);
    EXPECT_EQ(r2->second, 2);

    EXPECT_FALSE(two_sum({1, 2, 3}, 100).has_value());
}

// ── Задание 5. Односвязный список ──────────────────────────────────────────

TEST(SinglyLinkedList, EmptyOnStart) {
    SinglyLinkedList<int> l;
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.size(), 0u);
    EXPECT_EQ(l.to_vector(), (std::vector<int>{}));
}

TEST(SinglyLinkedList, PushFrontOrder) {
    SinglyLinkedList<int> l;
    l.push_front(1);
    l.push_front(2);
    l.push_front(3);
    EXPECT_EQ(l.size(), 3u);
    EXPECT_FALSE(l.empty());
    // push_front кладёт в голову → порядок обратный вставке
    EXPECT_EQ(l.to_vector(), (std::vector<int>{3, 2, 1}));
}

TEST(SinglyLinkedList, PushBackOrder) {
    SinglyLinkedList<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    EXPECT_EQ(l.size(), 3u);
    // push_back добавляет в хвост → порядок совпадает с вставкой
    EXPECT_EQ(l.to_vector(), (std::vector<int>{1, 2, 3}));
}

TEST(SinglyLinkedList, MixedPushFrontAndBack) {
    SinglyLinkedList<int> l;
    l.push_back(2);   // [2]
    l.push_front(1);  // [1, 2]
    l.push_back(3);   // [1, 2, 3]
    l.push_front(0);  // [0, 1, 2, 3]
    EXPECT_EQ(l.to_vector(), (std::vector<int>{0, 1, 2, 3}));
    EXPECT_EQ(l.size(), 4u);
}

TEST(SinglyLinkedList, Reverse) {
    SinglyLinkedList<int> l;
    for (int x : {1, 2, 3, 4, 5}) l.push_back(x);
    l.reverse();
    EXPECT_EQ(l.to_vector(), (std::vector<int>{5, 4, 3, 2, 1}));
    EXPECT_EQ(l.size(), 5u);
    // после reverse список остаётся рабочим
    l.push_front(6);
    l.push_back(0);
    EXPECT_EQ(l.to_vector(), (std::vector<int>{6, 5, 4, 3, 2, 1, 0}));
}

TEST(SinglyLinkedList, ReverseEdgeCases) {
    SinglyLinkedList<int> empty;
    empty.reverse();
    EXPECT_EQ(empty.to_vector(), (std::vector<int>{}));

    SinglyLinkedList<int> one;
    one.push_back(42);
    one.reverse();
    EXPECT_EQ(one.to_vector(), (std::vector<int>{42}));
}

TEST(SinglyLinkedList, WorksForStrings) {
    SinglyLinkedList<std::string> l;
    l.push_back("a");
    l.push_back("b");
    l.push_front("z");
    EXPECT_EQ(l.to_vector(), (std::vector<std::string>{"z", "a", "b"}));
    l.reverse();
    EXPECT_EQ(l.to_vector(), (std::vector<std::string>{"b", "a", "z"}));
}

// ── Задание 6. Стек на массиве ─────────────────────────────────────────────

TEST(ArrayStack, PushTopPopSize) {
    ArrayStack<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
    s.push(10);
    s.push(20);
    EXPECT_EQ(s.size(), 2u);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.top(), 20);
    s.pop();
    EXPECT_EQ(s.top(), 10);
    EXPECT_EQ(s.size(), 1u);
    s.pop();
    EXPECT_TRUE(s.empty());
}

TEST(ArrayStack, PopOnEmptyThrows) {
    ArrayStack<int> s;
    EXPECT_THROW(s.pop(), std::out_of_range);
}

TEST(ArrayStack, TopOnEmptyThrows) {
    ArrayStack<int> s;
    EXPECT_THROW(s.top(), std::out_of_range);
}

TEST(ArrayStack, ThrowsAfterDraining) {
    ArrayStack<int> s;
    s.push(1);
    s.pop();
    EXPECT_THROW(s.top(), std::out_of_range);
    EXPECT_THROW(s.pop(), std::out_of_range);
}

TEST(ArrayStack, WorksForStrings) {
    ArrayStack<std::string> s;
    s.push("x");
    s.push("y");
    EXPECT_EQ(s.top(), "y");
    s.pop();
    EXPECT_EQ(s.top(), "x");
}

// ── Задание 7. BST ─────────────────────────────────────────────────────────

TEST(BST, EmptyOnStart) {
    BST t;
    EXPECT_EQ(t.size(), 0u);
    EXPECT_FALSE(t.contains(5));
    EXPECT_EQ(t.inorder(), (std::vector<int>{}));
}

TEST(BST, InsertAndContains) {
    BST t;
    t.insert(5);
    t.insert(3);
    t.insert(8);
    t.insert(1);
    EXPECT_TRUE(t.contains(5));
    EXPECT_TRUE(t.contains(3));
    EXPECT_TRUE(t.contains(8));
    EXPECT_TRUE(t.contains(1));
    EXPECT_FALSE(t.contains(4));
    EXPECT_FALSE(t.contains(100));
    EXPECT_EQ(t.size(), 4u);
}

TEST(BST, InorderIsSorted) {
    BST t;
    for (int x : {5, 3, 8, 1, 4, 7, 9, 2, 6}) t.insert(x);
    EXPECT_EQ(t.inorder(),
              (std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST(BST, DuplicatesIgnored) {
    BST t;
    t.insert(5);
    t.insert(5);
    t.insert(3);
    t.insert(3);
    t.insert(5);
    EXPECT_EQ(t.size(), 2u);
    EXPECT_EQ(t.inorder(), (std::vector<int>{3, 5}));
}

TEST(BST, NegativesAndSingleNode) {
    BST t;
    t.insert(0);
    t.insert(-10);
    t.insert(-5);
    t.insert(10);
    EXPECT_TRUE(t.contains(-10));
    EXPECT_TRUE(t.contains(-5));
    EXPECT_FALSE(t.contains(-7));
    EXPECT_EQ(t.inorder(), (std::vector<int>{-10, -5, 0, 10}));
}
