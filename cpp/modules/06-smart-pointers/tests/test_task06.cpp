#include <gtest/gtest.h>
#include "task06.hpp"
#include <memory>
#include <string>
#include <random>
#include <vector>
#include <algorithm>
#include <numeric>
#include <limits>
#include <utility>

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

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Эти тесты прогоняют сотни случайных входов с ФИКСИРОВАННЫМ сидом mt19937 и
// проверяют ИНВАРИАНТЫ (round-trip, перестановка, оракул std::, границы), а не
// конкретные примеры. Сид фиксирован => CI детерминирован и не «флакает».

// ---------------------------------------------------------------------------
// make_unique_int: владеет переданным значением; разные вызовы — разные адреса.

TEST(SmartPointersProps, OwnsExactValueAndDistinctStorage) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(-1000000, 1000000);
    for (int iter = 0; iter < 400; ++iter) {
        int v = dist(rng);
        auto p = make_unique_int(v);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(*p, v);                 // владеет ровно тем значением

        int w = dist(rng);
        auto q = make_unique_int(w);
        ASSERT_NE(q, nullptr);
        // Два независимых владельца: отдельные блоки памяти.
        EXPECT_NE(p.get(), q.get());
        // Запись через один указатель не задевает другой.
        *p = v + 1;
        EXPECT_EQ(*q, w);
    }
}

TEST(SmartPointersProps, EdgeExtremeValues) {
    for (int v : {std::numeric_limits<int>::min(),
                  std::numeric_limits<int>::max(), 0, -1, 1}) {
        auto p = make_unique_int(v);
        ASSERT_NE(p, nullptr);
        EXPECT_EQ(*p, v);
    }
}

// ---------------------------------------------------------------------------
// IntList: round-trip и инвариант порядка push_front.
// Свойство: to_vector() == reverse(последовательности вставок); size() == n;
//           contains(x) <=> x присутствует во входе.

TEST(IntListProps, PushFrontReversesAndRoundTrips) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> lenDist(0, 60);
    std::uniform_int_distribution<int> valDist(-500, 500);

    for (int iter = 0; iter < 300; ++iter) {
        int n = lenDist(rng);
        std::vector<int> inserted;
        inserted.reserve(n);
        IntList list;
        for (int i = 0; i < n; ++i) {
            int v = valDist(rng);
            inserted.push_back(v);
            list.push_front(v);
        }
        // size совпадает с числом вставок.
        EXPECT_EQ(list.size(), static_cast<std::size_t>(n));

        // push_front => to_vector — это reverse порядка вставки.
        std::vector<int> expected(inserted.rbegin(), inserted.rend());
        EXPECT_EQ(list.to_vector(), expected);

        // to_vector — перестановка вставленного мультимножества.
        std::vector<int> sortedIn = inserted;
        std::vector<int> sortedOut = list.to_vector();
        std::sort(sortedIn.begin(), sortedIn.end());
        std::sort(sortedOut.begin(), sortedOut.end());
        EXPECT_EQ(sortedIn, sortedOut);
    }
}

TEST(IntListProps, ContainsMatchesPresenceOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> lenDist(0, 40);
    std::uniform_int_distribution<int> valDist(-30, 30);   // тесная область => дубликаты и попадания

    for (int iter = 0; iter < 300; ++iter) {
        int n = lenDist(rng);
        std::vector<int> inserted;
        IntList list;
        for (int i = 0; i < n; ++i) {
            int v = valDist(rng);
            inserted.push_back(v);
            list.push_front(v);
        }
        // Оракул присутствия: std::find по тому же мультимножеству.
        for (int probe = -35; probe <= 35; ++probe) {
            bool oracle =
                std::find(inserted.begin(), inserted.end(), probe) != inserted.end();
            EXPECT_EQ(list.contains(probe), oracle) << "probe=" << probe;
        }
    }
}

TEST(IntListProps, EmptyAndSingleEdge) {
    IntList empty;
    EXPECT_EQ(empty.size(), 0u);
    EXPECT_TRUE(empty.to_vector().empty());
    EXPECT_FALSE(empty.contains(0));

    IntList one;
    one.push_front(123);
    EXPECT_EQ(one.size(), 1u);
    EXPECT_TRUE(one.contains(123));
    EXPECT_FALSE(one.contains(124));
    std::vector<int> expected = {123};
    EXPECT_EQ(one.to_vector(), expected);
}

// ---------------------------------------------------------------------------
// TreeNode: владение детьми (shared) + слабая ссылка на родителя.
// Свойства: subtree_sum == сумма всех значений в поддереве (независимый оракул);
//           child(i)->parent() == узел; корень видит use_count == 1 (нет цикла);
//           child_count() == числу добавленных детей.

TEST(TreeProps, SubtreeSumEqualsManualSum) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> valDist(-50, 50);

    for (int iter = 0; iter < 250; ++iter) {
        // Строим дерево «вручную» и параллельно копим ожидаемую сумму поддерева.
        std::uniform_int_distribution<int> shapeDist(0, 4);
        std::uniform_int_distribution<int> depthDist(0, 3);

        int rootVal = valDist(rng);
        auto root = make_tree_root(rootVal);
        long expectedSum = rootVal;
        std::size_t expectedDirectChildren = 0;

        int directKids = shapeDist(rng);
        for (int i = 0; i < directKids; ++i) {
            int cVal = valDist(rng);
            auto child = root->add_child(cVal);
            ++expectedDirectChildren;
            expectedSum += cVal;

            // Каждый прямой ребёнок видит root как родителя (weak->lock).
            EXPECT_EQ(child->parent(), root);

            int grandKids = shapeDist(rng);
            for (int g = 0; g < grandKids; ++g) {
                int gVal = valDist(rng);
                auto grand = child->add_child(gVal);
                expectedSum += gVal;
                EXPECT_EQ(grand->parent(), child);
            }
        }

        EXPECT_EQ(root->child_count(), expectedDirectChildren);
        EXPECT_EQ(static_cast<long>(root->subtree_sum()), expectedSum);

        // Локальный инвариант: subtree_sum родителя >= ... нет, значения знаковые;
        // зато сумма поддерева ребёнка — часть суммы корня:
        long childSums = 0;
        for (std::size_t i = 0; i < root->child_count(); ++i) {
            childSums += root->child(i)->subtree_sum();
        }
        EXPECT_EQ(static_cast<long>(root->subtree_sum()), rootVal + childSums);
    }
}

TEST(TreeProps, ParentIsWeakNoSelfCount) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> kidDist(0, 25);
    std::uniform_int_distribution<int> valDist(-100, 100);

    for (int iter = 0; iter < 300; ++iter) {
        auto root = make_tree_root(valDist(rng));
        int kids = kidDist(rng);
        std::vector<std::shared_ptr<TreeNode>> handles;  // держим детей живыми
        for (int i = 0; i < kids; ++i) {
            handles.push_back(root->add_child(valDist(rng)));
        }
        // Сколько бы детей ни было, родитель хранится через weak_ptr =>
        // единственный внешний владелец root — переменная root.
        EXPECT_EQ(root.use_count(), 1L);
        EXPECT_EQ(root->child_count(), static_cast<std::size_t>(kids));

        // Каждый ребёнок указывает на того же родителя.
        for (const auto& h : handles) {
            EXPECT_EQ(h->parent(), root);
        }
    }
}

TEST(TreeProps, ParentLockNullAfterRootDies) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> valDist(-100, 100);

    for (int iter = 0; iter < 200; ++iter) {
        std::shared_ptr<TreeNode> child;
        {
            auto root = make_tree_root(valDist(rng));
            child = root->add_child(valDist(rng));
            ASSERT_NE(child->parent(), nullptr);   // пока root жив
        } // root уничтожен
        EXPECT_EQ(child->parent(), nullptr);       // weak lock => nullptr
    }
}

// ---------------------------------------------------------------------------
// Counter (Pimpl): value() == start + сумма всех инкрементов/add;
//                  перемещение переносит состояние без копирования.

TEST(PimplProps, ValueTracksAppliedDeltas) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> startDist(-1000, 1000);
    std::uniform_int_distribution<int> opsDist(0, 50);
    std::uniform_int_distribution<int> deltaDist(-100, 100);
    std::uniform_int_distribution<int> coin(0, 1);

    for (int iter = 0; iter < 400; ++iter) {
        long start = startDist(rng);
        Counter c(static_cast<int>(start));
        long expected = start;

        int ops = opsDist(rng);
        for (int i = 0; i < ops; ++i) {
            if (coin(rng) == 0) {
                c.increment();
                expected += 1;
            } else {
                int d = deltaDist(rng);
                c.add(d);
                expected += d;
            }
        }
        EXPECT_EQ(static_cast<long>(c.value()), expected);
    }
}

TEST(PimplProps, MovePreservesStateAndContinues) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> startDist(-500, 500);
    std::uniform_int_distribution<int> addDist(-50, 50);

    for (int iter = 0; iter < 300; ++iter) {
        int start = startDist(rng);
        int d = addDist(rng);
        Counter a(start);
        a.add(d);
        int before = a.value();
        EXPECT_EQ(before, start + d);

        // move-конструирование переносит ровно то же значение.
        Counter b(std::move(a));
        EXPECT_EQ(b.value(), before);
        b.increment();
        EXPECT_EQ(b.value(), before + 1);

        // move-присваивание тоже переносит состояние.
        Counter target(0);
        target = std::move(b);
        EXPECT_EQ(target.value(), before + 1);
    }
}

TEST(PimplProps, DefaultAndExtremeStart) {
    Counter d;
    EXPECT_EQ(d.value(), 0);

    Counter c(0);
    c.add(std::numeric_limits<int>::max());
    EXPECT_EQ(c.value(), std::numeric_limits<int>::max());
    // -1 откатывает на единицу от максимума.
    c.add(-1);
    EXPECT_EQ(c.value(), std::numeric_limits<int>::max() - 1);
}

// ---------------------------------------------------------------------------
// SharedResource: read() стабилен; use_count() == числу живых владельцев блока.
// Оракул для use_count — мы сами считаем количество живых копий.

TEST(SharedResProps, UseCountMatchesLiveOwners) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> nDist(0, 30);
    std::uniform_int_distribution<int> charDist('a', 'z');
    std::uniform_int_distribution<int> lenDist(0, 12);

    for (int iter = 0; iter < 250; ++iter) {
        // случайная строка
        int len = lenDist(rng);
        std::string payload;
        payload.reserve(len);
        for (int i = 0; i < len; ++i)
            payload.push_back(static_cast<char>(charDist(rng)));

        SharedResource base(payload);
        EXPECT_EQ(base.use_count(), 1L);
        EXPECT_EQ(base.read(), payload);

        int extra = nDist(rng);
        std::vector<SharedResource> owners;   // живые дополнительные владельцы
        owners.reserve(extra);
        for (int i = 0; i < extra; ++i) {
            owners.push_back(base.share());
            // После i+1 share-копий всего владельцев: 1 (base) + (i+1).
            long expected = 1L + static_cast<long>(i + 1);
            EXPECT_EQ(base.use_count(), expected);
            EXPECT_EQ(owners.back().use_count(), expected);
            // Все видят одни и те же данные (общий блок).
            EXPECT_EQ(owners.back().read(), payload);
        }

        long total = 1L + static_cast<long>(extra);
        EXPECT_EQ(base.use_count(), total);

        // По мере уничтожения копий счётчик строго убывает к 1.
        for (int i = extra; i > 0; --i) {
            owners.pop_back();
            EXPECT_EQ(base.use_count(), static_cast<long>(i));
        }
        EXPECT_EQ(base.use_count(), 1L);
        EXPECT_EQ(base.read(), payload);   // данные пережили уход совладельцев
    }
}

// ---------------------------------------------------------------------------
// Задание 6. maybe_at: безопасный доступ к элементу вектора через optional.

TEST(MaybeAt, ValidIndex) {
    std::vector<int> v = {10, 20, 30};
    auto r = maybe_at(v, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 20);
}

TEST(MaybeAt, FirstAndLastIndex) {
    std::vector<int> v = {7, 8, 9};
    ASSERT_TRUE(maybe_at(v, 0).has_value());
    EXPECT_EQ(*maybe_at(v, 0), 7);
    ASSERT_TRUE(maybe_at(v, 2).has_value());
    EXPECT_EQ(*maybe_at(v, 2), 9);
}

TEST(MaybeAt, OutOfBoundsReturnsNullopt) {
    std::vector<int> v = {1, 2, 3};
    EXPECT_FALSE(maybe_at(v, 3).has_value());
    EXPECT_FALSE(maybe_at(v, 100).has_value());
    EXPECT_EQ(maybe_at(v, 3), std::nullopt);
}

TEST(MaybeAt, EmptyVectorAlwaysNullopt) {
    std::vector<int> v;
    EXPECT_FALSE(maybe_at(v, 0).has_value());
    EXPECT_EQ(maybe_at(v, 0), std::nullopt);
}

TEST(MaybeAt, SingleElement) {
    std::vector<int> v = {42};
    ASSERT_TRUE(maybe_at(v, 0).has_value());
    EXPECT_EQ(*maybe_at(v, 0), 42);
    EXPECT_FALSE(maybe_at(v, 1).has_value());
}

// Seeded property: для случайных векторов и индексов
//   has_value() == (i < size)   и   *result == v[i] когда в границах.
TEST(MaybeAtProps, HasValueIffInBoundsAndValueCorrect) {
    std::mt19937 rng(0xDEADBEEFu);
    std::uniform_int_distribution<int>       lenDist(0, 50);
    std::uniform_int_distribution<int>       valDist(-1000, 1000);
    std::uniform_int_distribution<std::size_t> idxDist(0, 60);

    for (int iter = 0; iter < 500; ++iter) {
        int len = lenDist(rng);
        std::vector<int> v;
        v.reserve(len);
        for (int k = 0; k < len; ++k) v.push_back(valDist(rng));

        std::size_t i = idxDist(rng);
        auto result = maybe_at(v, i);

        // Инвариант: has_value совпадает с оракулом (i < v.size())
        bool expected_present = (i < v.size());
        EXPECT_EQ(result.has_value(), expected_present)
            << "v.size()=" << v.size() << " i=" << i;

        // Инвариант: значение совпадает с прямым обращением
        if (expected_present) {
            EXPECT_EQ(*result, v[i])
                << "v.size()=" << v.size() << " i=" << i;
        } else {
            EXPECT_EQ(result, std::nullopt);
        }
    }
}

TEST(SharedResProps, ShareIsIdempotentOnData) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> charDist('A', 'Z');
    std::uniform_int_distribution<int> lenDist(0, 16);

    for (int iter = 0; iter < 200; ++iter) {
        int len = lenDist(rng);
        std::string payload;
        for (int i = 0; i < len; ++i)
            payload.push_back(static_cast<char>(charDist(rng)));

        SharedResource r(payload);
        // share() не меняет содержимое — round-trip данных через цепочку share.
        SharedResource a = r.share();
        SharedResource b = a.share();
        SharedResource c = b.share();
        EXPECT_EQ(r.read(), payload);
        EXPECT_EQ(a.read(), payload);
        EXPECT_EQ(b.read(), payload);
        EXPECT_EQ(c.read(), payload);
        // 4 владельца одного блока.
        EXPECT_EQ(r.use_count(), 4L);
    }
}
