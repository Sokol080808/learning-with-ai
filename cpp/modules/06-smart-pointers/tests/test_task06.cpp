#include <gtest/gtest.h>
#include "task06.hpp"
#include <memory>
#include <string>

TEST(SmartPointers, MakeUniqueInt) {
    auto p = make_unique_int(42);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 42);
}

TEST(IntList, PushFrontOrder) {
    IntList list;
    list.push_front(1);
    list.push_front(2);
    list.push_front(3);
    // последний добавленный — первым: 3, 2, 1
    std::vector<int> expected = {3, 2, 1};
    EXPECT_EQ(list.to_vector(), expected);
}

TEST(IntList, Size) {
    IntList list;
    EXPECT_EQ(list.size(), 0u);
    list.push_front(10);
    list.push_front(20);
    EXPECT_EQ(list.size(), 2u);
}

TEST(IntList, Contains) {
    IntList list;
    list.push_front(5);
    list.push_front(7);
    EXPECT_TRUE(list.contains(5));
    EXPECT_TRUE(list.contains(7));
    EXPECT_FALSE(list.contains(99));
}

TEST(IntList, EmptyToVector) {
    IntList list;
    EXPECT_TRUE(list.to_vector().empty());
}

// ---------------------------------------------------------------------------
// Задание 3. Дерево: shared_ptr на детей, weak_ptr на родителя.

TEST(Tree, RootHasNoParent) {
    auto root = make_tree_root(1);
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(root->value(), 1);
    EXPECT_EQ(root->parent(), nullptr);
    EXPECT_EQ(root->child_count(), 0u);
}

TEST(Tree, AddChildLinksBothWays) {
    auto root = make_tree_root(10);
    auto a = root->add_child(20);
    auto b = root->add_child(30);

    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->value(), 20);
    EXPECT_EQ(b->value(), 30);

    EXPECT_EQ(root->child_count(), 2u);
    EXPECT_EQ(root->child(0), a);
    EXPECT_EQ(root->child(1), b);

    // Ребёнок видит родителя через weak_ptr -> lock().
    EXPECT_EQ(a->parent(), root);
    EXPECT_EQ(b->parent(), root);
}

TEST(Tree, ParentLinkIsWeakNoCycleLeak) {
    // Родитель НЕ должен удерживать лишнюю ссылку на самого себя через детей.
    // Если бы parent был shared_ptr, use_count корня раздулся бы числом детей.
    auto root = make_tree_root(0);
    root->add_child(1);
    root->add_child(2);
    root->add_child(3);
    // Единственный внешний владелец корня — переменная root => use_count == 1.
    EXPECT_EQ(root.use_count(), 1L);
}

TEST(Tree, SubtreeSumRecursive) {
    auto root = make_tree_root(1);
    auto a = root->add_child(2);
    auto b = root->add_child(3);
    a->add_child(4);
    b->add_child(5);
    b->add_child(6);
    // 1 + 2 + 3 + 4 + 5 + 6
    EXPECT_EQ(root->subtree_sum(), 21);
    // Сумма поддерева b: 3 + 5 + 6
    EXPECT_EQ(b->subtree_sum(), 14);
}

TEST(Tree, ChildKeepsParentAliveOnlyWeakly) {
    // Когда корень умирает, повышение weak_ptr ребёнком должно дать nullptr.
    std::shared_ptr<TreeNode> child;
    {
        auto root = make_tree_root(7);
        child = root->add_child(8);
        ASSERT_NE(child->parent(), nullptr);   // пока root жив — родитель доступен
    } // root уничтожен
    // Ребёнок жив (его держит child), но родителя больше нет: lock() => nullptr.
    EXPECT_EQ(child->parent(), nullptr);
}

// ---------------------------------------------------------------------------
// Задание 4. Pimpl на unique_ptr.

TEST(Pimpl, StartsAtGivenValue) {
    Counter c(5);
    EXPECT_EQ(c.value(), 5);
}

TEST(Pimpl, DefaultStartsAtZero) {
    Counter c;
    EXPECT_EQ(c.value(), 0);
}

TEST(Pimpl, IncrementAndAdd) {
    Counter c(0);
    c.increment();
    c.increment();
    c.add(10);
    EXPECT_EQ(c.value(), 12);
}

TEST(Pimpl, MoveTransfersState) {
    Counter a(3);
    a.add(4);                 // a == 7
    Counter b(std::move(a));  // перемещение
    EXPECT_EQ(b.value(), 7);
    b.increment();
    EXPECT_EQ(b.value(), 8);
}

// ---------------------------------------------------------------------------
// Задание 5. Общий ресурс и use_count.

TEST(SharedRes, SingleOwnerCountIsOne) {
    SharedResource r("hello");
    EXPECT_EQ(r.use_count(), 1L);
    EXPECT_EQ(r.read(), "hello");
}

TEST(SharedRes, ShareIncrementsUseCount) {
    SharedResource r("data");
    SharedResource s = r.share();
    // Теперь два владельца одного блока.
    EXPECT_EQ(r.use_count(), 2L);
    EXPECT_EQ(s.use_count(), 2L);
    EXPECT_EQ(s.read(), "data");
}

TEST(SharedRes, CountDropsWhenSharerDies) {
    SharedResource r("x");
    {
        SharedResource s = r.share();
        EXPECT_EQ(r.use_count(), 2L);
    } // s уничтожен — счётчик падает
    EXPECT_EQ(r.use_count(), 1L);
}

TEST(SharedRes, ThreeOwnersSeeSameData) {
    SharedResource r("shared");
    SharedResource s1 = r.share();
    SharedResource s2 = r.share();
    EXPECT_EQ(r.use_count(), 3L);
    EXPECT_EQ(s1.read(), "shared");
    EXPECT_EQ(s2.read(), "shared");
}
