#include <gtest/gtest.h>
#include <vector>
#include <list>
#include <string>
#include <array>
#include <random>
#include <numeric>
#include <algorithm>
#include <ranges>
#include <climits>
#include <cstdint>
#include "concepts_ranges.hpp"

using namespace m17;

// ---- вспомогательные типы для проверки концептов -------------------

// Складывается: число с operator+ (int подойдёт, но возьмём и свой).
struct Money {
    int cents = 0;
    friend Money operator+(const Money& a, const Money& b) { return {a.cents + b.cents}; }
    friend bool operator==(const Money&, const Money&) = default;
};

// НЕ складывается: нет operator+.
struct NoPlus {
    int x = 0;
};

// =====================================================================
// Задание 1а — концепт Addable
// =====================================================================
TEST(Addable, TrueForArithmeticAndUserPlus) {
    EXPECT_TRUE(Addable<int>);
    EXPECT_TRUE(Addable<double>);
    EXPECT_TRUE(Addable<std::string>);  // строки складываются через +
    EXPECT_TRUE(Addable<Money>);
}

TEST(Addable, FalseWhenNoPlus) {
    EXPECT_FALSE(Addable<NoPlus>);
}

// =====================================================================
// Задание 1б — концепт Container
// =====================================================================
TEST(Container, TrueForStandardContainers) {
    EXPECT_TRUE(Container<std::vector<int>>);
    EXPECT_TRUE(Container<std::list<double>>);
    EXPECT_TRUE((Container<std::array<int, 3>>));
    EXPECT_TRUE(Container<std::string>);
}

TEST(Container, FalseForScalars) {
    EXPECT_FALSE(Container<int>);
    EXPECT_FALSE(Container<double>);
    EXPECT_FALSE(Container<Money>);
}

// =====================================================================
// Задание 2 — sum_container
// =====================================================================
TEST(SumContainer, SumsInts) {
    std::vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(sum_container(v), 15);
}

TEST(SumContainer, SumsDoubles) {
    std::vector<double> v{0.5, 0.25, 0.25};
    EXPECT_DOUBLE_EQ(sum_container(v), 1.0);
}

TEST(SumContainer, EmptyIsZeroValue) {
    std::vector<int> v{};
    EXPECT_EQ(sum_container(v), 0);
}

TEST(SumContainer, WorksOnList) {
    std::list<int> l{10, 20, 30};
    EXPECT_EQ(sum_container(l), 60);
}

TEST(SumContainer, SumsUserType) {
    std::vector<Money> v{{100}, {250}, {50}};
    EXPECT_EQ(sum_container(v), Money{400});
}

// =====================================================================
// Задание 3 — even_times_two_take (ленивый конвейер)
// =====================================================================
TEST(Pipeline, EvenThenDoubleThenTake) {
    std::vector<int> xs{1, 2, 3, 4, 5, 6, 7, 8};
    // чётные: 2,4,6,8 -> удвоить: 4,8,12,16 -> взять 2: {4,8}
    EXPECT_EQ(even_times_two_take(xs, 2), (std::vector<int>{4, 8}));
}

TEST(Pipeline, TakeMoreThanAvailable) {
    std::vector<int> xs{2, 4, 6};
    // чётные все -> удвоить: 4,8,12 -> взять 10: всё
    EXPECT_EQ(even_times_two_take(xs, 10), (std::vector<int>{4, 8, 12}));
}

TEST(Pipeline, NoEvensGivesEmpty) {
    std::vector<int> xs{1, 3, 5, 7};
    EXPECT_TRUE(even_times_two_take(xs, 3).empty());
}

TEST(Pipeline, TakeZeroGivesEmpty) {
    std::vector<int> xs{2, 4, 6, 8};
    EXPECT_TRUE(even_times_two_take(xs, 0).empty());
}

TEST(Pipeline, OrderIsFilterThenTransform) {
    std::vector<int> xs{2, 3, 4, 5, 6};
    // фильтр по ИСХОДНЫМ числам (2,4,6), потом *2 = 4,8,12
    EXPECT_EQ(even_times_two_take(xs, 100), (std::vector<int>{4, 8, 12}));
}

// =====================================================================
// Задание 4 — Sortable + sorted_copy
// =====================================================================
TEST(Sortable, TrueForRandomAccessComparable) {
    EXPECT_TRUE(Sortable<std::vector<int>>);
    EXPECT_TRUE((Sortable<std::array<int, 4>>));
}

TEST(Sortable, FalseForListNoRandomAccess) {
    // std::list — двунаправленный, не random-access -> не Sortable
    EXPECT_FALSE(Sortable<std::list<int>>);
}

TEST(Sortable, FalseWhenElementNotComparable) {
    EXPECT_FALSE(Sortable<std::vector<NoPlus>>);  // у NoPlus нет operator<
}

TEST(SortedCopy, SortsAscending) {
    std::vector<int> v{5, 1, 4, 2, 3};
    EXPECT_EQ(sorted_copy(v), (std::vector<int>{1, 2, 3, 4, 5}));
}

TEST(SortedCopy, DoesNotMutateInput) {
    std::vector<int> v{3, 1, 2};
    auto out = sorted_copy(v);
    EXPECT_EQ(v, (std::vector<int>{3, 1, 2}));  // исходник не тронут
    EXPECT_EQ(out, (std::vector<int>{1, 2, 3}));
}

TEST(SortedCopy, EmptyStaysEmpty) {
    std::vector<int> v{};
    EXPECT_TRUE(sorted_copy(v).empty());
}

// =====================================================================
// Задание 5 — take_n (свой адаптер)
// =====================================================================
TEST(TakeN, FirstThree) {
    std::vector<int> v{10, 20, 30, 40, 50};
    EXPECT_EQ(take_n(v, 3), (std::vector<int>{10, 20, 30}));
}

TEST(TakeN, MoreThanSizeReturnsAll) {
    std::vector<int> v{1, 2};
    EXPECT_EQ(take_n(v, 100), (std::vector<int>{1, 2}));
}

TEST(TakeN, ZeroIsEmpty) {
    std::vector<int> v{1, 2, 3};
    EXPECT_TRUE(take_n(v, 0).empty());
}

TEST(TakeN, WorksOnList) {
    std::list<int> l{7, 8, 9, 10};
    EXPECT_EQ(take_n(l, 2), (std::vector<int>{7, 8}));
}

TEST(TakeN, WorksOnStrings) {
    std::vector<std::string> v{"a", "b", "c"};
    EXPECT_EQ(take_n(v, 2), (std::vector<std::string>{"a", "b"}));
}

// =====================================================================
// РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом)
// =====================================================================
// Все тесты ниже используют один фиксированный сид std::mt19937, поэтому
// детерминированы и не «флакают» в CI. Они проверяют ИНВАРИАНТЫ (оракул
// против std::, перестановка, упорядоченность, префикс, коммутативность,
// идемпотентность границ) на множестве сгенерированных входов плюс явные
// крайние случаи. Каждый набор имён уникален и не пересекается с тестами
// выше.

namespace {

// Свой тип со СВОИМИ operator+ и operator< — проверяем, что концепты и
// функции работают не только для встроенных арифметических типов.
struct Pt {
    int v = 0;
    friend Pt operator+(const Pt& a, const Pt& b) { return {a.v + b.v}; }
    friend bool operator<(const Pt& a, const Pt& b) { return a.v < b.v; }
    friend bool operator==(const Pt&, const Pt&) = default;
};

}  // namespace

// ---------------------------------------------------------------------
// Концепты — таблица истинности на расширенном наборе типов.
// Концепты вычисляются на этапе компиляции, поэтому RNG здесь не нужен;
// проверяем границы и пользовательские типы.
// ---------------------------------------------------------------------
TEST(ConceptProps, AddableMatchesPlusAvailability) {
    // Есть operator+ -> Addable истинен.
    EXPECT_TRUE(Addable<int>);
    EXPECT_TRUE(Addable<long>);
    EXPECT_TRUE(Addable<unsigned>);
    EXPECT_TRUE(Addable<float>);
    EXPECT_TRUE(Addable<Pt>);
    EXPECT_TRUE(Addable<std::string>);
    // Нет operator+ -> Addable ложен.
    EXPECT_FALSE(Addable<NoPlus>);
    EXPECT_FALSE(Addable<std::vector<int>>);  // у vector нет operator+
}

TEST(ConceptProps, ContainerMatchesIterability) {
    // Имеют begin/end и разыменование -> Container истинен.
    EXPECT_TRUE(Container<std::vector<Pt>>);
    EXPECT_TRUE(Container<std::list<int>>);
    EXPECT_TRUE((Container<std::array<double, 5>>));
    EXPECT_TRUE(Container<std::string>);
    EXPECT_TRUE(Container<std::vector<std::string>>);
    // Скаляры/пользовательские без итераторов -> ложен.
    EXPECT_FALSE(Container<int>);
    EXPECT_FALSE(Container<Pt>);
    EXPECT_FALSE(Container<NoPlus>);
}

TEST(ConceptProps, SortableRequiresRandomAccessAndLess) {
    EXPECT_TRUE(Sortable<std::vector<int>>);
    EXPECT_TRUE(Sortable<std::vector<Pt>>);   // у Pt есть operator<
    EXPECT_TRUE((Sortable<std::array<long, 7>>));
    EXPECT_FALSE(Sortable<std::list<int>>);          // не random-access
    EXPECT_FALSE(Sortable<std::vector<NoPlus>>);     // нет operator<
}

// ---------------------------------------------------------------------
// sum_container — оракул std::accumulate + коммутативность.
// ---------------------------------------------------------------------
TEST(SumContainerProps, MatchesAccumulateOracleAndIsOrderIndependent) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> sizeDist(0, 40);
    // Узкий диапазон значений, чтобы сумма гарантированно влезала в int.
    std::uniform_int_distribution<int> valDist(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> v(static_cast<std::size_t>(sizeDist(rng)));
        for (int& x : v) x = valDist(rng);

        const int oracle = std::accumulate(v.begin(), v.end(), 0);
        EXPECT_EQ(sum_container(v), oracle);

        // Сумма не зависит от порядка элементов (коммутативность сложения).
        std::vector<int> shuffled = v;
        std::shuffle(shuffled.begin(), shuffled.end(), rng);
        EXPECT_EQ(sum_container(shuffled), oracle);

        // Пустой контейнер из тех же элементов даёт Value{} == 0.
        if (v.empty()) EXPECT_EQ(sum_container(v), 0);
    }
}

TEST(SumContainerProps, WorksOnListAndUserType) {
    std::mt19937 rng(0x5EEDu);
    std::uniform_int_distribution<int> sizeDist(0, 30);
    std::uniform_int_distribution<int> valDist(-500, 500);

    for (int iter = 0; iter < 200; ++iter) {
        const std::size_t n = static_cast<std::size_t>(sizeDist(rng));
        std::list<int> l;
        std::vector<Pt> pts;
        int oracle = 0;
        for (std::size_t i = 0; i < n; ++i) {
            int x = valDist(rng);
            l.push_back(x);
            pts.push_back(Pt{x});
            oracle += x;
        }
        EXPECT_EQ(sum_container(l), oracle);
        EXPECT_EQ(sum_container(pts), Pt{oracle});
    }
}

TEST(SumContainerProps, EdgeSingletonAndExtremes) {
    std::vector<int> one{42};
    EXPECT_EQ(sum_container(one), 42);

    std::vector<int> empty{};
    EXPECT_EQ(sum_container(empty), 0);

    // Один элемент-экстремум: сумма равна самому элементу, без переполнения.
    std::vector<int> hi{INT_MAX};
    EXPECT_EQ(sum_container(hi), INT_MAX);
    std::vector<int> lo{INT_MIN};
    EXPECT_EQ(sum_container(lo), INT_MIN);
}

// ---------------------------------------------------------------------
// even_times_two_take — оракул через ручной фильтр/трансформ/take.
// ---------------------------------------------------------------------
TEST(PipelineProps, MatchesManualOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> sizeDist(0, 50);
    std::uniform_int_distribution<int> valDist(-50, 50);
    std::uniform_int_distribution<std::size_t> kDist(0, 60);

    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> xs(static_cast<std::size_t>(sizeDist(rng)));
        for (int& x : xs) x = valDist(rng);
        const std::size_t k = kDist(rng);

        // Эталон: фильтр чётности по ИСХОДНЫМ, потом *2, потом первые k.
        std::vector<int> oracle;
        for (int x : xs) {
            if (x % 2 == 0) {
                if (oracle.size() == k) break;
                oracle.push_back(x * 2);
            }
        }
        std::vector<int> got = even_times_two_take(xs, k);
        EXPECT_EQ(got, oracle);

        // Инварианты результата.
        EXPECT_LE(got.size(), k);                       // не больше k
        for (int y : got) EXPECT_EQ(y % 2, 0);          // y = 2*чётное -> чётно
        for (int y : got) EXPECT_EQ(y % 4, 0);          // 2*(чётное) делится на 4
    }
}

TEST(PipelineProps, TakeIsMonotoneAndPrefix) {
    std::mt19937 rng(0xABCDEFu);
    std::uniform_int_distribution<int> sizeDist(0, 40);
    std::uniform_int_distribution<int> valDist(-30, 30);

    for (int iter = 0; iter < 200; ++iter) {
        std::vector<int> xs(static_cast<std::size_t>(sizeDist(rng)));
        for (int& x : xs) x = valDist(rng);

        // «Всё, что есть»: огромный k материализует полный конвейер.
        std::vector<int> full = even_times_two_take(xs, 1000000);

        // take(k) — это префикс полного результата для любого k <= size.
        std::uniform_int_distribution<std::size_t> kDist(0, full.size());
        const std::size_t k = kDist(rng);
        std::vector<int> taken = even_times_two_take(xs, k);

        EXPECT_EQ(taken.size(), k);
        EXPECT_TRUE(std::equal(taken.begin(), taken.end(), full.begin()));

        // take(0) пуст; take(size) == full.
        EXPECT_TRUE(even_times_two_take(xs, 0).empty());
        EXPECT_EQ(even_times_two_take(xs, full.size()), full);
    }
}

TEST(PipelineProps, EdgeNoEvensAndEmpty) {
    std::vector<int> empty{};
    EXPECT_TRUE(even_times_two_take(empty, 5).empty());

    std::vector<int> odds{1, 3, 5, 7, 9, -11};
    EXPECT_TRUE(even_times_two_take(odds, 100).empty());

    // Ноль чётный: попадает в фильтр и удваивается в 0.
    std::vector<int> withZero{0, 1, 0};
    EXPECT_EQ(even_times_two_take(withZero, 100), (std::vector<int>{0, 0}));
}

// ---------------------------------------------------------------------
// sorted_copy — упорядоченность + перестановка входа + оракул std::sort.
// ---------------------------------------------------------------------
TEST(SortedCopyProps, OrderedPermutationMatchingStdSort) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> sizeDist(0, 60);
    std::uniform_int_distribution<int> valDist(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> v(static_cast<std::size_t>(sizeDist(rng)));
        for (int& x : v) x = valDist(rng);

        std::vector<int> original = v;            // копия для проверки неизменности
        std::vector<int> out = sorted_copy(v);

        // 1) Вход не изменён.
        EXPECT_EQ(v, original);
        // 2) Размер сохранён.
        EXPECT_EQ(out.size(), original.size());
        // 3) Результат отсортирован по возрастанию.
        EXPECT_TRUE(std::is_sorted(out.begin(), out.end()));
        // 4) Результат — перестановка входа.
        EXPECT_TRUE(std::is_permutation(out.begin(), out.end(),
                                        original.begin(), original.end()));
        // 5) Совпадает с оракулом std::sort.
        std::vector<int> oracle = original;
        std::sort(oracle.begin(), oracle.end());
        EXPECT_EQ(out, oracle);
    }
}

TEST(SortedCopyProps, IdempotentAndUserType) {
    std::mt19937 rng(0xFACEu);
    std::uniform_int_distribution<int> sizeDist(0, 40);
    std::uniform_int_distribution<int> valDist(-100, 100);

    for (int iter = 0; iter < 200; ++iter) {
        std::vector<Pt> v(static_cast<std::size_t>(sizeDist(rng)));
        for (Pt& p : v) p = Pt{valDist(rng)};

        std::vector<Pt> once = sorted_copy(v);
        // Идемпотентность: сортировка уже отсортированного ничего не меняет.
        std::vector<Pt> twice = sorted_copy(once);
        EXPECT_EQ(once, twice);
        EXPECT_TRUE(std::is_sorted(once.begin(), once.end(),
                                   [](const Pt& a, const Pt& b) { return a < b; }));
        EXPECT_TRUE(std::is_permutation(once.begin(), once.end(), v.begin(), v.end()));
    }
}

TEST(SortedCopyProps, EdgeEmptySingleAndDuplicates) {
    std::vector<int> empty{};
    EXPECT_TRUE(sorted_copy(empty).empty());

    std::vector<int> single{7};
    EXPECT_EQ(sorted_copy(single), (std::vector<int>{7}));

    // Все одинаковые: остаются на месте, размер сохранён.
    std::vector<int> dups{4, 4, 4, 4};
    EXPECT_EQ(sorted_copy(dups), (std::vector<int>{4, 4, 4, 4}));

    // Экстремумы упорядочиваются корректно.
    std::vector<int> ext{INT_MAX, INT_MIN, 0};
    EXPECT_EQ(sorted_copy(ext), (std::vector<int>{INT_MIN, 0, INT_MAX}));
}

// ---------------------------------------------------------------------
// take_n — размер == min(n, len), результат — префикс входа, оракул views.
// ---------------------------------------------------------------------
TEST(TakeNProps, SizeAndPrefixInvariants) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> sizeDist(0, 50);
    std::uniform_int_distribution<int> valDist(-1000, 1000);
    std::uniform_int_distribution<std::size_t> nDist(0, 70);

    for (int iter = 0; iter < 400; ++iter) {
        std::vector<int> v(static_cast<std::size_t>(sizeDist(rng)));
        for (int& x : v) x = valDist(rng);
        const std::size_t n = nDist(rng);

        std::vector<int> out = take_n(v, n);

        // 1) Размер ровно min(n, len) — не выходим за end().
        const std::size_t expected = std::min(n, v.size());
        EXPECT_EQ(out.size(), expected);
        // 2) Результат — точный префикс входа.
        EXPECT_TRUE(std::equal(out.begin(), out.end(), v.begin()));
        // 3) Оракул через std::views::take.
        std::vector<int> oracle;
        for (int x : v | std::views::take(n)) oracle.push_back(x);
        EXPECT_EQ(out, oracle);
    }
}

TEST(TakeNProps, FullAndOverTakeReturnWholeInput) {
    std::mt19937 rng(0xDEADu);
    std::uniform_int_distribution<int> sizeDist(0, 40);
    std::uniform_int_distribution<int> valDist(-200, 200);

    for (int iter = 0; iter < 200; ++iter) {
        std::vector<int> v(static_cast<std::size_t>(sizeDist(rng)));
        for (int& x : v) x = valDist(rng);

        // take_n(v, len) и take_n(v, >len) возвращают весь вход.
        EXPECT_EQ(take_n(v, v.size()), v);
        EXPECT_EQ(take_n(v, v.size() + 5), v);
        // take_n(v, 0) пуст.
        EXPECT_TRUE(take_n(v, 0).empty());

        // Работает и по std::list (не random-access диапазон).
        std::list<int> l(v.begin(), v.end());
        std::uniform_int_distribution<std::size_t> nDist(0, v.size() + 3);
        const std::size_t n = nDist(rng);
        std::vector<int> fromList = take_n(l, n);
        std::vector<int> prefix(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(std::min(n, v.size())));
        EXPECT_EQ(fromList, prefix);
    }
}

TEST(TakeNProps, EdgeEmptyAndStrings) {
    std::vector<int> empty{};
    EXPECT_TRUE(take_n(empty, 0).empty());
    EXPECT_TRUE(take_n(empty, 100).empty());

    std::vector<std::string> v{"alpha", "beta", "gamma", "delta"};
    EXPECT_EQ(take_n(v, 2), (std::vector<std::string>{"alpha", "beta"}));
    EXPECT_EQ(take_n(v, 0), (std::vector<std::string>{}));
    EXPECT_EQ(take_n(v, 99), v);
}

// =====================================================================
// Задание 6 — sort_by_age + youngest_name (проекции std::ranges)
// =====================================================================

TEST(SortByAge, BasicOrder) {
    std::vector<Person> people{{"Зина", 30}, {"Алёша", 22}, {"Маша", 25}};
    auto out = sort_by_age(people);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0].age, 22);
    EXPECT_EQ(out[1].age, 25);
    EXPECT_EQ(out[2].age, 30);
}

TEST(SortByAge, DoesNotMutateInput) {
    std::vector<Person> orig{{"А", 5}, {"Б", 3}, {"В", 9}};
    std::vector<Person> copy = orig;
    sort_by_age(orig);      // принимает по значению — оригинал НЕ изменяется
    EXPECT_EQ(orig, copy);  // вектор orig остался в исходном порядке
}

TEST(SortByAge, SingleElement) {
    std::vector<Person> v{{"Один", 42}};
    auto out = sort_by_age(v);
    EXPECT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].name, "Один");
}

TEST(SortByAge, EmptyVector) {
    std::vector<Person> v{};
    EXPECT_TRUE(sort_by_age(v).empty());
}

TEST(SortByAge, AllSameAge) {
    std::vector<Person> v{{"А", 10}, {"Б", 10}, {"В", 10}};
    auto out = sort_by_age(v);
    ASSERT_EQ(out.size(), 3u);
    for (const auto& p : out) EXPECT_EQ(p.age, 10);
}

TEST(YounguestName, FindsYoungest) {
    std::vector<Person> people{{"Зина", 30}, {"Алёша", 22}, {"Маша", 25}};
    EXPECT_EQ(youngest_name(people), "Алёша");
}

TEST(YounguestName, EmptyReturnsEmptyString) {
    std::vector<Person> v{};
    EXPECT_EQ(youngest_name(v), "");
}

TEST(YounguestName, SingleElement) {
    std::vector<Person> v{{"Один", 7}};
    EXPECT_EQ(youngest_name(v), "Один");
}

TEST(YounguestName, TieReturnFirst) {
    // min_element возвращает первый минимум при равных значениях
    std::vector<Person> v{{"А", 5}, {"Б", 5}, {"В", 10}};
    EXPECT_EQ(youngest_name(v), "А");
}

// ---------------------------------------------------------------------
// Задание 6 — property/seeded тесты (std::mt19937, сид фиксирован)
// ---------------------------------------------------------------------

TEST(ProjectionProps, SortByAgeMatchesManualSort) {
    std::mt19937 rng(0x17C0FFEEu);
    std::uniform_int_distribution<int> sizeDist(0, 40);
    std::uniform_int_distribution<int> ageDist(1, 100);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(sizeDist(rng));
        // Используем уникальные имена-индексы, чтобы youngest_name -> find_if
        // не путал одноимённых Person из разных позиций.
        std::vector<Person> people(n);
        for (std::size_t i = 0; i < n; ++i) {
            people[i].name = "P" + std::to_string(i);
            people[i].age  = ageDist(rng);
        }

        std::vector<Person> out = sort_by_age(people);

        // 1) Размер сохранён
        EXPECT_EQ(out.size(), people.size());
        // 2) Результат отсортирован по age
        for (std::size_t i = 1; i < out.size(); ++i)
            EXPECT_LE(out[i - 1].age, out[i].age);
        // 3) Мультисет по age совпадает с входом (перестановка)
        std::vector<int> inAges, outAges;
        for (const auto& p : people) inAges.push_back(p.age);
        for (const auto& p : out)    outAges.push_back(p.age);
        std::sort(inAges.begin(),  inAges.end());
        std::sort(outAges.begin(), outAges.end());
        EXPECT_EQ(inAges, outAges);
        // 4) Для непустых: youngest_name возвращает имя Person с минимальным age
        if (!people.empty()) {
            const int min_age = out.front().age;
            std::string yn = youngest_name(people);
            // Находим Person с этим именем во входе (имена уникальны)
            auto found = std::ranges::find_if(people,
                [&](const Person& p) { return p.name == yn; });
            ASSERT_NE(found, people.end());
            EXPECT_EQ(found->age, min_age);
        }
    }
}

TEST(ProjectionProps, YounguestNameMatchesMinElementOracle) {
    std::mt19937 rng(0x17A9EDu);
    std::uniform_int_distribution<int> sizeDist(1, 30);
    std::uniform_int_distribution<int> ageDist(0, 60);

    for (int iter = 0; iter < 200; ++iter) {
        const std::size_t n = static_cast<std::size_t>(sizeDist(rng));
        std::vector<Person> people(n);
        for (std::size_t i = 0; i < n; ++i) {
            people[i].name = "P" + std::to_string(i);
            people[i].age  = ageDist(rng);
        }

        // Оракул: std::min_element по age вручную
        auto oracle_it = std::min_element(people.begin(), people.end(),
            [](const Person& a, const Person& b) { return a.age < b.age; });
        const int min_age = oracle_it->age;

        std::string yn = youngest_name(people);
        // Найти Person с полученным именем и проверить, что age == min_age
        auto it = std::ranges::find_if(people,
            [&](const Person& p) { return p.name == yn; });
        ASSERT_NE(it, people.end()) << "youngest_name вернула имя, которого нет во входе";
        EXPECT_EQ(it->age, min_age);
    }
}
