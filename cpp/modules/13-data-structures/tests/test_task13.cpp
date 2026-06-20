#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <deque>
#include <stdexcept>
#include <random>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <limits>
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

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо точечных примеров проверяем ИНВАРИАНТЫ на сотнях случайных
// входов с фиксированным сидом mt19937 — чтобы CI никогда не флакал.
// Везде, где возможно, сверяемся с оракулом из <algorithm>/std::set.

// ── Algo: insertion_sort ───────────────────────────────────────────────────
// Инварианты: результат отсортирован, является перестановкой входа,
// совпадает с std::sort и сортировка идемпотентна.
TEST(SortProps, MatchesStdSortIsPermutationAndOrdered) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 60);
    std::uniform_int_distribution<int> val(-100, 100);  // намеренно узкий → дубликаты
    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> in(len(rng));
        for (int& x : in) x = val(rng);

        std::vector<int> got = insertion_sort(in);

        // 1) длина сохранена
        ASSERT_EQ(got.size(), in.size());
        // 2) отсортировано по возрастанию
        EXPECT_TRUE(std::is_sorted(got.begin(), got.end()));
        // 3) перестановка входа (мультимножество совпадает)
        EXPECT_TRUE(std::is_permutation(got.begin(), got.end(), in.begin()));
        // 4) совпадает с оракулом std::sort
        std::vector<int> oracle = in;
        std::sort(oracle.begin(), oracle.end());
        EXPECT_EQ(got, oracle);
        // 5) идемпотентность: сортировка отсортированного ничего не меняет
        EXPECT_EQ(insertion_sort(got), got);
    }
}

TEST(SortProps, ExtremeValuesAndConstantRuns) {
    const int lo = std::numeric_limits<int>::min();
    const int hi = std::numeric_limits<int>::max();
    EXPECT_EQ(insertion_sort({hi, lo, 0}), (std::vector<int>{lo, 0, hi}));
    EXPECT_EQ(insertion_sort({7, 7, 7, 7}), (std::vector<int>{7, 7, 7, 7}));
    EXPECT_EQ(insertion_sort({lo, lo}), (std::vector<int>{lo, lo}));
    // одиночный элемент и пустой
    EXPECT_EQ(insertion_sort({hi}), (std::vector<int>{hi}));
    EXPECT_EQ(insertion_sort({}), (std::vector<int>{}));
}

// ── Algo: binary_search_idx ────────────────────────────────────────────────
// Инварианты: найденный индекс указывает ровно на target; для отсутствующего
// возвращается -1; согласованность с std::binary_search как оракулом.
TEST(BinarySearchProps, FoundPointsToTargetAbsentIsMinusOne) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 50);
    std::uniform_int_distribution<int> val(-40, 40);
    for (int iter = 0; iter < 400; ++iter) {
        // строим отсортированный массив (с возможными дубликатами)
        std::vector<int> v(len(rng));
        for (int& x : v) x = val(rng);
        std::sort(v.begin(), v.end());

        // пробуем искать значения из диапазона (часть присутствует, часть — нет)
        for (int t = -45; t <= 45; ++t) {
            int idx = binary_search_idx(v, t);
            bool present = std::binary_search(v.begin(), v.end(), t);
            if (present) {
                ASSERT_GE(idx, 0);
                ASSERT_LT(idx, static_cast<int>(v.size()));
                EXPECT_EQ(v[idx], t);  // указывает именно на target
            } else {
                EXPECT_EQ(idx, -1);
            }
        }
    }
}

TEST(BinarySearchProps, EveryElementIsFindable) {
    std::mt19937 rng(0x13371337u);
    std::uniform_int_distribution<int> len(1, 60);
    std::uniform_int_distribution<int> val(-1000, 1000);
    for (int iter = 0; iter < 300; ++iter) {
        std::vector<int> v(len(rng));
        for (int& x : v) x = val(rng);
        std::sort(v.begin(), v.end());
        // каждый реально присутствующий элемент находится, и индекс корректен
        for (std::size_t k = 0; k < v.size(); ++k) {
            int idx = binary_search_idx(v, v[k]);
            ASSERT_GE(idx, 0);
            EXPECT_EQ(v[idx], v[k]);
        }
    }
    // граничные случаи
    EXPECT_EQ(binary_search_idx({}, 0), -1);
    EXPECT_EQ(binary_search_idx({5}, 5), 0);
    EXPECT_EQ(binary_search_idx({5}, 4), -1);
}

// ── Algo: two_sum ──────────────────────────────────────────────────────────
// Инварианты: при наличии ответа индексы валидны, i<j и сумма == target;
// при nullopt — никакой пары с такой суммой не существует (брутфорс-оракул).
TEST(TwoSumProps, ReturnedPairIsValidAndNulloptMeansNoPair) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 40);
    std::uniform_int_distribution<int> val(-30, 30);
    std::uniform_int_distribution<int> tgt(-60, 60);
    for (int iter = 0; iter < 500; ++iter) {
        std::vector<int> v(len(rng));
        for (int& x : v) x = val(rng);
        int target = tgt(rng);

        auto r = two_sum(v, target);

        // брутфорс-оракул: существует ли вообще валидная пара i<j
        bool oracle_exists = false;
        for (std::size_t i = 0; i < v.size() && !oracle_exists; ++i)
            for (std::size_t j = i + 1; j < v.size(); ++j)
                if (v[i] + v[j] == target) { oracle_exists = true; break; }

        EXPECT_EQ(r.has_value(), oracle_exists);
        if (r.has_value()) {
            int i = r->first, j = r->second;
            ASSERT_GE(i, 0);
            ASSERT_LT(j, static_cast<int>(v.size()));
            EXPECT_LT(i, j);                       // i < j по контракту
            EXPECT_EQ(v[i] + v[j], target);        // сумма действительно равна target
        }
    }
}

// ── Stack<T> (vector-based LIFO) ───────────────────────────────────────────
// Инвариант: серия push, затем серия pop возвращает элементы в обратном
// порядке (LIFO round-trip), а size трекается точно.
TEST(StackProps, LifoRoundTrip) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 100);
    std::uniform_int_distribution<int> val(-1000, 1000);
    for (int iter = 0; iter < 300; ++iter) {
        std::vector<int> pushed(len(rng));
        for (int& x : pushed) x = val(rng);

        Stack<int> s;
        for (int x : pushed) s.push(x);
        ASSERT_EQ(s.size(), pushed.size());
        EXPECT_EQ(s.empty(), pushed.empty());

        // снимаем — должны получить обратный порядок
        std::vector<int> popped;
        while (!s.empty()) {
            popped.push_back(s.top());
            s.pop();
        }
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(s.size(), 0u);
        std::reverse(popped.begin(), popped.end());
        EXPECT_EQ(popped, pushed);  // LIFO: pop-порядок развёрнут == push-порядок
    }
}

// ── ArrayStack<T> (бросает std::out_of_range на пустом) ────────────────────
// Инвариант: LIFO round-trip как у Stack, плюс после полного слива
// top()/pop() кидают именно std::out_of_range.
TEST(ArrayStackProps, LifoRoundTripAndThrowsWhenDrained) {
    std::mt19937 rng(0xBADC0DEu);
    std::uniform_int_distribution<int> len(0, 80);
    std::uniform_int_distribution<int> val(-500, 500);
    for (int iter = 0; iter < 300; ++iter) {
        std::vector<int> pushed(len(rng));
        for (int& x : pushed) x = val(rng);

        ArrayStack<int> s;
        for (int x : pushed) s.push(x);
        ASSERT_EQ(s.size(), pushed.size());

        std::vector<int> popped;
        while (!s.empty()) {
            popped.push_back(s.top());
            s.pop();
        }
        std::reverse(popped.begin(), popped.end());
        EXPECT_EQ(popped, pushed);

        // полностью слитый — оба метода бросают конкретный тип
        EXPECT_THROW(s.top(), std::out_of_range);
        EXPECT_THROW(s.pop(), std::out_of_range);
    }
}

// ── SinglyLinkedList<T> ─────────────────────────────────────────────────────
// Инварианты: to_vector совпадает с порядком push_back; reverse эквивалентен
// std::reverse; двойной reverse — тождество; size трекается.
TEST(SinglyLinkedListProps, PushBackRoundTripAndReverseInvolution) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 100);
    std::uniform_int_distribution<int> val(-1000, 1000);
    for (int iter = 0; iter < 300; ++iter) {
        std::vector<int> seq(len(rng));
        for (int& x : seq) x = val(rng);

        SinglyLinkedList<int> l;
        for (int x : seq) l.push_back(x);

        // round-trip: построили push_back → читаем тот же порядок
        ASSERT_EQ(l.size(), seq.size());
        EXPECT_EQ(l.empty(), seq.empty());
        EXPECT_EQ(l.to_vector(), seq);

        // reverse эквивалентен std::reverse оракула
        l.reverse();
        std::vector<int> rev = seq;
        std::reverse(rev.begin(), rev.end());
        EXPECT_EQ(l.to_vector(), rev);
        EXPECT_EQ(l.size(), seq.size());  // размер не меняется

        // двойной reverse — инволюция (тождество)
        l.reverse();
        EXPECT_EQ(l.to_vector(), seq);
    }
}

TEST(SinglyLinkedListProps, PushFrontMirrorsReverseOfBack) {
    std::mt19937 rng(0xFEEDFACEu);
    std::uniform_int_distribution<int> len(0, 80);
    std::uniform_int_distribution<int> val(-200, 200);
    for (int iter = 0; iter < 300; ++iter) {
        std::vector<int> seq(len(rng));
        for (int& x : seq) x = val(rng);

        // push_front по очереди → порядок обратный последовательности вставки
        SinglyLinkedList<int> lf;
        for (int x : seq) lf.push_front(x);
        std::vector<int> expected = seq;
        std::reverse(expected.begin(), expected.end());
        EXPECT_EQ(lf.to_vector(), expected);
        EXPECT_EQ(lf.size(), seq.size());
    }
}

// ── BST (по int, дубликаты игнорируются) ────────────────────────────────────
// Инварианты: inorder == отсортированному множеству уникальных значений;
// size == числу уникальных; contains согласован с членством; вставка
// идемпотентна (повтор не меняет ни size, ни inorder).
TEST(BSTProps, InorderEqualsSortedUniqueAndContainsMatchesSet) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> len(0, 80);
    std::uniform_int_distribution<int> val(-50, 50);  // узко → много дубликатов
    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> ins(len(rng));
        for (int& x : ins) x = val(rng);

        BST t;
        for (int x : ins) t.insert(x);

        // оракул: std::set даёт отсортированное множество уникальных
        std::set<int> oracle(ins.begin(), ins.end());
        std::vector<int> expected(oracle.begin(), oracle.end());

        std::vector<int> walk = t.inorder();          // материализуем один раз
        EXPECT_EQ(t.size(), oracle.size());           // size == число уникальных
        EXPECT_EQ(walk, expected);                    // inorder отсортирован и уникален
        EXPECT_TRUE(std::is_sorted(walk.begin(), walk.end()));

        // contains согласован с членством по всему диапазону
        for (int q = -55; q <= 55; ++q)
            EXPECT_EQ(t.contains(q), oracle.count(q) != 0);
    }
}

TEST(BSTProps, InsertIsIdempotent) {
    std::mt19937 rng(0xABCDEF01u);
    std::uniform_int_distribution<int> len(1, 60);
    std::uniform_int_distribution<int> val(-30, 30);
    for (int iter = 0; iter < 300; ++iter) {
        std::vector<int> ins(len(rng));
        for (int& x : ins) x = val(rng);

        BST t;
        for (int x : ins) t.insert(x);
        std::size_t sz = t.size();
        std::vector<int> snapshot = t.inorder();

        // повторная вставка уже присутствующих значений ничего не меняет
        for (int x : ins) t.insert(x);
        EXPECT_EQ(t.size(), sz);
        EXPECT_EQ(t.inorder(), snapshot);
    }
}

TEST(BSTProps, ExtremeValues) {
    const int lo = std::numeric_limits<int>::min();
    const int hi = std::numeric_limits<int>::max();
    BST t;
    t.insert(hi);
    t.insert(lo);
    t.insert(0);
    t.insert(hi);  // дубликат экстремума
    EXPECT_EQ(t.size(), 3u);
    EXPECT_EQ(t.inorder(), (std::vector<int>{lo, 0, hi}));
    EXPECT_TRUE(t.contains(lo));
    EXPECT_TRUE(t.contains(hi));
    EXPECT_FALSE(t.contains(1));
}

// ===== Задание 8. Queue<T> на кольцевом буфере ================================

TEST(Queue, EmptyOnStart) {
    Queue<int> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0u);
}

TEST(Queue, FifoRoundTrip) {
    // FIFO: первым вошёл — первым вышел
    Queue<int> q;
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.front(), 1);
    q.dequeue();
    EXPECT_EQ(q.front(), 2);
    EXPECT_EQ(q.size(), 2u);
    q.dequeue();
    EXPECT_EQ(q.front(), 3);
    q.dequeue();
    EXPECT_TRUE(q.empty());
}

TEST(Queue, DequeueOnEmptyThrows) {
    Queue<int> q;
    EXPECT_THROW(q.dequeue(), std::underflow_error);
}

TEST(Queue, FrontOnEmptyThrows) {
    Queue<int> q;
    EXPECT_THROW(q.front(), std::underflow_error);
}

TEST(Queue, ThrowsAfterDraining) {
    Queue<int> q;
    q.enqueue(42);
    q.dequeue();
    EXPECT_THROW(q.front(), std::underflow_error);
    EXPECT_THROW(q.dequeue(), std::underflow_error);
}

TEST(Queue, OverflowThrows) {
    // CAPACITY=4 для этого теста, чтобы не ждать 64 вставки
    Queue<int, 4> q;
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    q.enqueue(4);
    EXPECT_THROW(q.enqueue(5), std::overflow_error);
}

TEST(Queue, WrapAroundRingBuffer) {
    // Проверяем «обёртку» кольца: enqueue/dequeue чередуем, чтобы
    // индексы перевалили за конец массива и завернулись на начало.
    Queue<int, 4> q;
    q.enqueue(10);
    q.enqueue(20);
    q.dequeue();   // head_ сдвинулся на 1
    q.enqueue(30);
    q.enqueue(40); // tail_ теперь = 3
    q.dequeue();   // head_ = 2
    q.enqueue(50); // tail_ обернётся на 0 (= 4 % 4)
    // Теперь в очереди: 30, 40, 50
    EXPECT_EQ(q.size(), 3u);
    EXPECT_EQ(q.front(), 30);
    q.dequeue();
    EXPECT_EQ(q.front(), 40);
    q.dequeue();
    EXPECT_EQ(q.front(), 50);
    q.dequeue();
    EXPECT_TRUE(q.empty());
}

TEST(Queue, WorksForStrings) {
    Queue<std::string> q;
    q.enqueue("hello");
    q.enqueue("world");
    EXPECT_EQ(q.front(), "hello");
    q.dequeue();
    EXPECT_EQ(q.front(), "world");
}

// ── Queue: seeded property — FIFO enqueue/dequeue порядок ─────────────────
// Инвариант: серия enqueue, затем серия dequeue возвращает элементы в том
// же порядке (FIFO), size трекается точно; front всегда == первому вставленному.
TEST(QueueProps, FifoOrderAndSizeTracking) {
    std::mt19937 rng(0xC0FFEEu);
    // Используем маленькую ёмкость, чтобы проверить границы кольца
    constexpr std::size_t CAP = 32;
    std::uniform_int_distribution<int> len(0, static_cast<int>(CAP));
    std::uniform_int_distribution<int> val(-500, 500);
    for (int iter = 0; iter < 400; ++iter) {
        int n = len(rng);
        std::vector<int> pushed(n);
        for (int& x : pushed) x = val(rng);

        Queue<int, CAP> q;
        for (int x : pushed) q.enqueue(x);
        ASSERT_EQ(q.size(), static_cast<std::size_t>(n));
        EXPECT_EQ(q.empty(), pushed.empty());

        // front должен указывать на первый вставленный элемент
        if (!pushed.empty())
            EXPECT_EQ(q.front(), pushed[0]);

        // dequeue по очереди — должен вернуть исходный порядок
        std::vector<int> got;
        while (!q.empty()) {
            got.push_back(q.front());
            q.dequeue();
        }
        EXPECT_EQ(got, pushed);  // FIFO: порядок сохранён
        EXPECT_TRUE(q.empty());
        EXPECT_EQ(q.size(), 0u);
    }
}

// ── Queue: property — interleaved enqueue/dequeue (имитирует «скользящее окно») ──
// Проверяем, что кольцо корректно работает при многократном wrap-around.
TEST(QueueProps, InterleavedEnqueueDequeueWithWrapAround) {
    std::mt19937 rng(0xDEADBEEFu);
    std::uniform_int_distribution<int> val(-200, 200);
    constexpr std::size_t CAP = 8;
    Queue<int, CAP> q;
    std::deque<int> oracle;

    for (int step = 0; step < 2000; ++step) {
        // Если очередь полна — только dequeue; если пуста — только enqueue;
        // иначе случайный выбор.
        bool do_enqueue;
        if (q.size() == CAP) {
            do_enqueue = false;
        } else if (q.empty()) {
            do_enqueue = true;
        } else {
            do_enqueue = (rng() % 2 == 0);
        }

        if (do_enqueue) {
            int v = val(rng);
            q.enqueue(v);
            oracle.push_back(v);
        } else {
            EXPECT_EQ(q.front(), oracle.front());
            q.dequeue();
            oracle.pop_front();
        }
        EXPECT_EQ(q.size(), oracle.size());
        if (!oracle.empty())
            EXPECT_EQ(q.front(), oracle.front());
    }
}

// ===== Задание 9. merge_sort ===================================================

TEST(MergeSort, BasicCases) {
    EXPECT_EQ(merge_sort({}), (std::vector<int>{}));
    EXPECT_EQ(merge_sort({42}), (std::vector<int>{42}));
    EXPECT_EQ(merge_sort({2, 1}), (std::vector<int>{1, 2}));
    EXPECT_EQ(merge_sort({3, 1, 4, 1, 5, 9, 2, 6}),
              (std::vector<int>{1, 1, 2, 3, 4, 5, 6, 9}));
    EXPECT_EQ(merge_sort({5, 4, 3, 2, 1}), (std::vector<int>{1, 2, 3, 4, 5}));
    EXPECT_EQ(merge_sort({1, 2, 3, 4, 5}), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(MergeSort, AllEqual) {
    EXPECT_EQ(merge_sort({7, 7, 7, 7}), (std::vector<int>{7, 7, 7, 7}));
}

TEST(MergeSort, NegativesAndExtremes) {
    const int lo = std::numeric_limits<int>::min();
    const int hi = std::numeric_limits<int>::max();
    EXPECT_EQ(merge_sort({hi, lo, 0}), (std::vector<int>{lo, 0, hi}));
    EXPECT_EQ(merge_sort({-5, -1, -3, -2}), (std::vector<int>{-5, -3, -2, -1}));
}

// ── merge_sort: seeded property — сортированность + перестановка + == std::sort ──
// Инварианты: результат отсортирован, является перестановкой входа,
// совпадает с оракулом std::sort, идемпотентен.
TEST(MergeSortProps, MatchesStdSortIsPermutationAndOrdered) {
    std::mt19937 rng(0xABCDEF42u);
    std::uniform_int_distribution<int> len(0, 200);
    std::uniform_int_distribution<int> val(-100, 100);  // узкий → дубликаты
    for (int iter = 0; iter < 500; ++iter) {
        std::vector<int> in(len(rng));
        for (int& x : in) x = val(rng);

        std::vector<int> got = merge_sort(in);

        ASSERT_EQ(got.size(), in.size());
        // 1) отсортирован
        EXPECT_TRUE(std::is_sorted(got.begin(), got.end()));
        // 2) перестановка
        EXPECT_TRUE(std::is_permutation(got.begin(), got.end(), in.begin()));
        // 3) совпадает с оракулом
        std::vector<int> oracle = in;
        std::sort(oracle.begin(), oracle.end());
        EXPECT_EQ(got, oracle);
        // 4) идемпотентность
        EXPECT_EQ(merge_sort(got), got);
    }
}
