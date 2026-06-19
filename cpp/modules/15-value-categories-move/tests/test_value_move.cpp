#include <gtest/gtest.h>

#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "value_move.hpp"

// =============================================================================
// Тесты модуля 15. На заготовках они КРАСНЫЕ — реализуй TODO в include/value_move.hpp.
// =============================================================================

// -----------------------------------------------------------------------------
// Задание 1. my_move
// -----------------------------------------------------------------------------

// Тип результата my_move обязан быть rvalue-ссылкой (xvalue), как у std::move.
TEST(MyMove, ReturnTypeIsRvalueReference) {
    int x = 0;
    static_assert(std::is_same_v<decltype(vm::my_move(x)), int&&>,
                  "my_move(lvalue) должен иметь тип int&&");

    const int cx = 0;
    static_assert(std::is_same_v<decltype(vm::my_move(cx)), const int&&>,
                  "const сохраняется: my_move(const int&) -> const int&&");

    static_assert(std::is_same_v<decltype(vm::my_move(42)), int&&>,
                  "my_move(rvalue) тоже int&&");
    SUCCEED();
}

// my_move должен ссылаться на ТОТ ЖЕ объект (менять только категорию), а не на
// какой-то посторонний. Проверяем по адресу: &my_move(x) == &x.
TEST(MyMove, RefersToSameObject) {
    int x = 7;
    int&& r = vm::my_move(x);
    EXPECT_EQ(&r, &x);
    EXPECT_EQ(r, 7);
}

// my_move(x) должен реально включать перемещение при инициализации:
// строку из x «уведут», и x опустеет (типичное поведение move у std::string).
TEST(MyMove, EnablesMoveFromNamedObject) {
    std::string src = "a fairly long string that will not fit in SSO buffer 123456";
    std::string dst = vm::my_move(src);  // ожидаем move, а не copy
    EXPECT_EQ(dst, "a fairly long string that will not fit in SSO buffer 123456");
    EXPECT_TRUE(src.empty());  // у перемещённой строки тело «украдено»
}

// -----------------------------------------------------------------------------
// Задание 2. ScopedResource (move-only)
// -----------------------------------------------------------------------------

TEST(ScopedResource, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<vm::ScopedResource>,
                  "ScopedResource нельзя копировать");
    static_assert(!std::is_copy_assignable_v<vm::ScopedResource>,
                  "ScopedResource нельзя копировать-присваивать");
    static_assert(std::is_move_constructible_v<vm::ScopedResource>,
                  "ScopedResource должен перемещаться");
    static_assert(std::is_nothrow_move_constructible_v<vm::ScopedResource>,
                  "move-конструктор обязан быть noexcept");
    static_assert(std::is_nothrow_move_assignable_v<vm::ScopedResource>,
                  "move-присваивание обязано быть noexcept");
    SUCCEED();
}

TEST(ScopedResource, ConstructAndValid) {
    vm::ScopedResource r{100};
    EXPECT_EQ(r.get(), 100);
    EXPECT_TRUE(r.valid());

    vm::ScopedResource empty;
    EXPECT_EQ(empty.get(), vm::ScopedResource::kEmpty);
    EXPECT_FALSE(empty.valid());
}

TEST(ScopedResource, MoveConstructorTransfersAndClears) {
    vm::ScopedResource a{55};
    vm::ScopedResource b{std::move(a)};
    EXPECT_EQ(b.get(), 55);      // хэндл переехал в b
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(a.get(), vm::ScopedResource::kEmpty);  // источник опустел
    EXPECT_FALSE(a.valid());
}

TEST(ScopedResource, MoveAssignmentTransfersAndClears) {
    vm::ScopedResource a{7};
    vm::ScopedResource b{9};
    b = std::move(a);
    EXPECT_EQ(b.get(), 7);
    EXPECT_EQ(a.get(), vm::ScopedResource::kEmpty);
}

TEST(ScopedResource, SelfMoveAssignmentKeepsHandle) {
    vm::ScopedResource a{321};
    auto& ref = a;
    a = std::move(ref);  // защита от самоприсваивания
    EXPECT_EQ(a.get(), 321);
}

TEST(ScopedResource, ReleaseReturnsHandleAndEmpties) {
    vm::ScopedResource a{77};
    int h = a.release();
    EXPECT_EQ(h, 77);
    EXPECT_EQ(a.get(), vm::ScopedResource::kEmpty);
    EXPECT_FALSE(a.valid());
}

TEST(ScopedResource, WorksInsideVector) {
    // Контейнер при росте переместит элементы (move noexcept) — без копий.
    std::vector<vm::ScopedResource> v;
    v.push_back(vm::ScopedResource{1});
    v.push_back(vm::ScopedResource{2});
    v.push_back(vm::ScopedResource{3});
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(v[0].get(), 1);
    EXPECT_EQ(v[2].get(), 3);
}

// -----------------------------------------------------------------------------
// Задание 3. CopyMoveCounter
// -----------------------------------------------------------------------------

TEST(CopyMoveCounter, CopyConstructionCounts) {
    vm::CopyMoveCounter::reset();
    vm::CopyMoveCounter a;
    vm::CopyMoveCounter b = a;  // копирование
    (void)b;
    EXPECT_EQ(vm::CopyMoveCounter::copies(), 1);
    EXPECT_EQ(vm::CopyMoveCounter::moves(), 0);
}

TEST(CopyMoveCounter, MoveConstructionCounts) {
    vm::CopyMoveCounter::reset();
    vm::CopyMoveCounter a;
    vm::CopyMoveCounter b = std::move(a);  // перемещение
    (void)b;
    EXPECT_EQ(vm::CopyMoveCounter::copies(), 0);
    EXPECT_EQ(vm::CopyMoveCounter::moves(), 1);
}

TEST(CopyMoveCounter, AssignmentCounts) {
    vm::CopyMoveCounter::reset();
    vm::CopyMoveCounter a, b;
    b = a;             // copy-assign
    EXPECT_EQ(vm::CopyMoveCounter::copies(), 1);
    b = std::move(a);  // move-assign
    EXPECT_EQ(vm::CopyMoveCounter::moves(), 1);
}

// Главный сценарий-демонстрация: передача по значению.
// Принимаем по значению. Передаём lvalue -> ровно одна КОПИЯ.
static void take_by_value(vm::CopyMoveCounter /*c*/) {}

TEST(CopyMoveCounter, PassByValueWithLvalueCopiesOnce) {
    vm::CopyMoveCounter::reset();
    vm::CopyMoveCounter a;
    take_by_value(a);  // именованный аргумент -> копия в параметр
    EXPECT_EQ(vm::CopyMoveCounter::copies(), 1);
    EXPECT_EQ(vm::CopyMoveCounter::moves(), 0);
}

TEST(CopyMoveCounter, PassByValueWithMoveMovesOnce) {
    vm::CopyMoveCounter::reset();
    vm::CopyMoveCounter a;
    take_by_value(std::move(a));  // std::move -> перемещение в параметр
    EXPECT_EQ(vm::CopyMoveCounter::copies(), 0);
    EXPECT_EQ(vm::CopyMoveCounter::moves(), 1);
}

// -----------------------------------------------------------------------------
// Задание 4. make_in_place (perfect forwarding)
// -----------------------------------------------------------------------------

namespace {
struct TwoArgs {
    int a;
    std::string b;
    TwoArgs(int a_, std::string b_) : a(a_), b(std::move(b_)) {}
};
}  // namespace

TEST(MakeInPlace, ForwardsArgumentsToConstructor) {
    auto obj = vm::make_in_place<TwoArgs>(42, std::string{"hi"});
    EXPECT_EQ(obj.a, 42);
    EXPECT_EQ(obj.b, "hi");
}

// rvalue-аргумент должен ПЕРЕМЕСТИТЬСЯ в T (а не скопироваться).
TEST(MakeInPlace, MovesRvalueArgument) {
    vm::CopyMoveCounter::reset();
    vm::CopyMoveCounter src;
    struct Holder {
        vm::CopyMoveCounter c;
        explicit Holder(vm::CopyMoveCounter cc) : c(std::move(cc)) {}
    };
    auto h = vm::make_in_place<Holder>(std::move(src));
    (void)h;
    // Идеальный проброс rvalue -> внутри ни одной копии исходного зонда.
    EXPECT_EQ(vm::CopyMoveCounter::copies(), 0);
    EXPECT_GE(vm::CopyMoveCounter::moves(), 1);
}

// lvalue-аргумент должен СКОПИРОВАТЬСЯ (категория сохраняется при пробросе).
TEST(MakeInPlace, CopiesLvalueArgument) {
    vm::CopyMoveCounter::reset();
    vm::CopyMoveCounter src;
    struct Holder {
        vm::CopyMoveCounter c;
        explicit Holder(vm::CopyMoveCounter cc) : c(std::move(cc)) {}
    };
    auto h = vm::make_in_place<Holder>(src);  // lvalue
    (void)h;
    EXPECT_GE(vm::CopyMoveCounter::copies(), 1);  // как минимум одна копия src
}

// -----------------------------------------------------------------------------
// Задание 5. is_lvalue
// -----------------------------------------------------------------------------

// Проверки времени компиляции тут вынесены в РАНТАЙМ намеренно: на заготовке
// is_lvalue<int&> == false, и если бы это был static_assert, проект бы не
// собрался. EXPECT_* позволяют коду скомпилироваться и упасть на КРАСНОМ тесте.
TEST(IsLvalue, TrueForLvalueReferences) {
    EXPECT_TRUE((vm::is_lvalue<int&>::value));
    EXPECT_TRUE((vm::is_lvalue_v<const int&>));
    EXPECT_TRUE((vm::is_lvalue_v<std::string&>));
}

TEST(IsLvalue, FalseForNonLvalueReferences) {
    EXPECT_FALSE((vm::is_lvalue<int>::value));
    EXPECT_FALSE((vm::is_lvalue_v<int&&>));
    EXPECT_FALSE((vm::is_lvalue_v<const int>));
}

TEST(IsLvalue, MatchesStandardTrait) {
    EXPECT_EQ((vm::is_lvalue_v<int&>), (std::is_lvalue_reference_v<int&>));
    EXPECT_EQ((vm::is_lvalue_v<int&&>), (std::is_lvalue_reference_v<int&&>));
    EXPECT_EQ((vm::is_lvalue_v<double>), (std::is_lvalue_reference_v<double>));
    EXPECT_EQ((vm::is_lvalue_v<const std::string&>),
              (std::is_lvalue_reference_v<const std::string&>));
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Эти тесты прогоняют сотни СГЕНЕРИРОВАННЫХ входов и проверяют ИНВАРИАНТЫ:
//   * my_move ссылается на тот же объект и эквивалентен std::move по значению;
//   * ScopedResource переносит хэндл и обнуляет источник (как unique_ptr);
//   * CopyMoveCounter точно считает копии/мувы в массовых сценариях;
//   * make_in_place идеально пробрасывает аргументы (оракул — прямое T(args...));
//   * is_lvalue совпадает со std::is_lvalue_reference на батарее типов.
// RNG зафиксирован сидом — тесты детерминированы, CI не «мерцает».

namespace {
// Произвольная «случайная» строка фиксированной длины: гарантированно длиннее SSO,
// чтобы у std::string было настоящее тело в куче (тогда move реально «крадёт»).
std::string random_long_string(std::mt19937& rng, std::size_t len) {
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<std::size_t> pick(0, sizeof(alphabet) - 2);
    std::string s;
    s.reserve(len);
    for (std::size_t i = 0; i < len; ++i) s.push_back(alphabet[pick(rng)]);
    return s;
}
}  // namespace

// -----------------------------------------------------------------------------
// Задание 1. my_move — property-тесты
// -----------------------------------------------------------------------------

// Инвариант: my_move меняет только КАТЕГОРИЮ, ссылаясь на тот же объект.
// Для случайных int должно быть &my_move(x) == &x и значение не тронуто.
TEST(MyMoveProps, RefersToSameObjectForRandomInts) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(-1'000'000, 1'000'000);
    for (int iter = 0; iter < 500; ++iter) {
        int x = dist(rng);
        const int saved = x;
        int&& r = vm::my_move(x);
        EXPECT_EQ(&r, &x);     // тот же объект
        EXPECT_EQ(r, saved);   // значение не изменилось — копирования/мутации нет
        EXPECT_EQ(x, saved);
    }
}

// Инвариант: инициализация из my_move(named string) даёт перемещение — копия
// содержимого корректна, а источник опустошён (тело «украдено»).
TEST(MyMoveProps, MoveRoundTripEmptiesSource) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<std::size_t> len_dist(40, 200);
    for (int iter = 0; iter < 300; ++iter) {
        std::string src = random_long_string(rng, len_dist(rng));
        const std::string expected = src;       // эталон содержимого
        std::string dst = vm::my_move(src);     // ожидаем move
        EXPECT_EQ(dst, expected);               // содержимое доехало целиком
        EXPECT_TRUE(src.empty());               // источник опустошён move-ом
    }
}

// Оракул: my_move по значению эквивалентен std::move — обе формы дают одно и то
// же значение цели и одинаково опустошают источник.
TEST(MyMoveProps, EquivalentToStdMove) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<std::size_t> len_dist(40, 200);
    for (int iter = 0; iter < 300; ++iter) {
        const std::string original = random_long_string(rng, len_dist(rng));

        std::string a = original;
        std::string via_mine = vm::my_move(a);

        std::string b = original;
        std::string via_std = std::move(b);

        EXPECT_EQ(via_mine, via_std);  // одинаковое содержимое цели
        EXPECT_EQ(a.empty(), b.empty());  // одинаково опустошён источник
    }
    // Тип результата обязан совпадать с типом std::move для одинакового аргумента.
    int z = 0;
    static_assert(std::is_same_v<decltype(vm::my_move(z)), decltype(std::move(z))>,
                  "my_move и std::move должны давать один и тот же тип-категорию");
    static_assert(
        std::is_same_v<decltype(vm::my_move(std::as_const(z))),
                       decltype(std::move(std::as_const(z)))>,
        "const сохраняется одинаково");
    SUCCEED();
}

// -----------------------------------------------------------------------------
// Задание 2. ScopedResource — property-тесты
// -----------------------------------------------------------------------------

// Инвариант move-конструктора: хэндл переезжает в цель, источник обнуляется.
// Прогоняем по случайным НЕпустым хэндлам (>= 0, т.к. -1 — это kEmpty).
TEST(ScopedResourceProps, MoveCtorTransfersAndClears) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (int iter = 0; iter < 400; ++iter) {
        const int h = dist(rng);
        vm::ScopedResource a{h};
        vm::ScopedResource b{std::move(a)};
        EXPECT_EQ(b.get(), h);
        EXPECT_TRUE(b.valid());
        EXPECT_EQ(a.get(), vm::ScopedResource::kEmpty);  // источник опустел
        EXPECT_FALSE(a.valid());
    }
}

// Инвариант move-присваивания: хэндл переезжает, источник обнуляется.
TEST(ScopedResourceProps, MoveAssignTransfersAndClears) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (int iter = 0; iter < 400; ++iter) {
        const int h = dist(rng);
        vm::ScopedResource a{h};
        vm::ScopedResource b{dist(rng)};
        b = std::move(a);
        EXPECT_EQ(b.get(), h);
        EXPECT_EQ(a.get(), vm::ScopedResource::kEmpty);
        EXPECT_FALSE(a.valid());
    }
}

// Инвариант release: возвращает текущий хэндл и обнуляет владельца.
TEST(ScopedResourceProps, ReleaseReturnsHandleAndEmpties) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (int iter = 0; iter < 400; ++iter) {
        const int h = dist(rng);
        vm::ScopedResource a{h};
        EXPECT_EQ(a.release(), h);
        EXPECT_EQ(a.get(), vm::ScopedResource::kEmpty);
        EXPECT_FALSE(a.valid());
        // Повторный release пустого ресурса отдаёт kEmpty (идемпотентно пуст).
        EXPECT_EQ(a.release(), vm::ScopedResource::kEmpty);
    }
}

// Самоприсваивание через move не должно «обнулять» ресурс (есть защита this!=&other).
TEST(ScopedResourceProps, SelfMoveKeepsHandle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (int iter = 0; iter < 300; ++iter) {
        const int h = dist(rng);
        vm::ScopedResource a{h};
        auto& ref = a;
        a = std::move(ref);
        EXPECT_EQ(a.get(), h);
        EXPECT_TRUE(a.valid());
    }
}

// Round-trip через вектор: после серии push_back (рост -> move noexcept) порядок
// и значения хэндлов сохраняются. move-only тип живёт в контейнере без копий.
TEST(ScopedResourceProps, VectorRoundTripPreservesHandles) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> n_dist(0, 64);
    std::uniform_int_distribution<int> h_dist(0, 1'000'000);
    for (int iter = 0; iter < 200; ++iter) {
        const int n = n_dist(rng);
        std::vector<int> expected;
        expected.reserve(static_cast<std::size_t>(n));
        std::vector<vm::ScopedResource> v;  // намеренно без reserve: провоцируем рост
        for (int i = 0; i < n; ++i) {
            const int h = h_dist(rng);
            expected.push_back(h);
            v.push_back(vm::ScopedResource{h});
        }
        ASSERT_EQ(v.size(), expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            EXPECT_EQ(v[i].get(), expected[i]);  // тот же хэндл на той же позиции
            EXPECT_TRUE(v[i].valid());
        }
    }
}

// -----------------------------------------------------------------------------
// Задание 3. CopyMoveCounter — property-тесты
// -----------------------------------------------------------------------------

// Инвариант: ровно одно копирование при копи-конструировании, ноль мувов;
// и наоборот для перемещения. Прогоняем многократно — счётчик точный.
TEST(CopyMoveCounterProps, SingleCopyVsSingleMove) {
    std::mt19937 rng(0xC0FFEEu);
    std::bernoulli_distribution coin(0.5);
    for (int iter = 0; iter < 500; ++iter) {
        vm::CopyMoveCounter::reset();
        vm::CopyMoveCounter a;
        if (coin(rng)) {
            vm::CopyMoveCounter b = a;  // copy
            (void)b;
            EXPECT_EQ(vm::CopyMoveCounter::copies(), 1);
            EXPECT_EQ(vm::CopyMoveCounter::moves(), 0);
        } else {
            vm::CopyMoveCounter b = std::move(a);  // move
            (void)b;
            EXPECT_EQ(vm::CopyMoveCounter::copies(), 0);
            EXPECT_EQ(vm::CopyMoveCounter::moves(), 1);
        }
    }
}

// Инвариант: вектор с reserve(n) + n move-вставок (push_back rvalue) даёт РОВНО
// n перемещений и НОЛЬ копий — никакой реаллокации, всё перемещается.
TEST(CopyMoveCounterProps, ReservedVectorMovesExactlyN) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> n_dist(0, 50);
    for (int iter = 0; iter < 300; ++iter) {
        const int n = n_dist(rng);
        std::vector<vm::CopyMoveCounter> v;
        v.reserve(static_cast<std::size_t>(n));  // ёмкости хватит -> без роста
        vm::CopyMoveCounter::reset();
        for (int i = 0; i < n; ++i) {
            vm::CopyMoveCounter tmp;
            v.push_back(std::move(tmp));  // move во временный слот
        }
        EXPECT_EQ(vm::CopyMoveCounter::copies(), 0);
        EXPECT_EQ(vm::CopyMoveCounter::moves(), n);
    }
}

// Инвариант передачи по значению: lvalue-аргумент -> ровно 1 копия, 0 мувов;
// rvalue-аргумент -> ровно 1 мув, 0 копий. (Параметр уничтожается внутри.)
TEST(CopyMoveCounterProps, PassByValueCategoryDecidesCopyOrMove) {
    std::mt19937 rng(0xC0FFEEu);
    std::bernoulli_distribution coin(0.5);
    for (int iter = 0; iter < 500; ++iter) {
        vm::CopyMoveCounter::reset();
        vm::CopyMoveCounter a;
        if (coin(rng)) {
            take_by_value(a);  // lvalue -> copy
            EXPECT_EQ(vm::CopyMoveCounter::copies(), 1);
            EXPECT_EQ(vm::CopyMoveCounter::moves(), 0);
        } else {
            take_by_value(std::move(a));  // rvalue -> move
            EXPECT_EQ(vm::CopyMoveCounter::copies(), 0);
            EXPECT_EQ(vm::CopyMoveCounter::moves(), 1);
        }
    }
}

// -----------------------------------------------------------------------------
// Задание 4. make_in_place — property-тесты
// -----------------------------------------------------------------------------

// Оракул: make_in_place<T>(args...) строит ровно то же значение, что и прямой
// вызов конструктора T(args...). Проверяем на случайных int + случайных строках.
TEST(MakeInPlaceProps, EqualsDirectConstruction) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> a_dist(-100000, 100000);
    std::uniform_int_distribution<std::size_t> len_dist(0, 40);
    for (int iter = 0; iter < 400; ++iter) {
        const int a = a_dist(rng);
        const std::string b = random_long_string(rng, len_dist(rng));
        auto made = vm::make_in_place<TwoArgs>(a, b);  // оба аргумента — lvalue
        // Оракул — прямая конструкция теми же значениями.
        EXPECT_EQ(made.a, a);
        EXPECT_EQ(made.b, b);
    }
}

// Инвариант perfect forwarding: rvalue-зонд пробрасывается как rvalue -> 0 копий,
// >=1 мув; lvalue-зонд -> >=1 копия. Категория аргумента сохраняется при пробросе.
TEST(MakeInPlaceProps, ForwardingPreservesValueCategory) {
    struct Holder {
        vm::CopyMoveCounter c;
        explicit Holder(vm::CopyMoveCounter cc) : c(std::move(cc)) {}
    };
    std::mt19937 rng(0xC0FFEEu);
    std::bernoulli_distribution coin(0.5);
    for (int iter = 0; iter < 400; ++iter) {
        vm::CopyMoveCounter src;
        vm::CopyMoveCounter::reset();
        if (coin(rng)) {
            auto h = vm::make_in_place<Holder>(std::move(src));  // rvalue
            (void)h;
            EXPECT_EQ(vm::CopyMoveCounter::copies(), 0);  // ни одной копии
            EXPECT_GE(vm::CopyMoveCounter::moves(), 1);
        } else {
            auto h = vm::make_in_place<Holder>(src);  // lvalue
            (void)h;
            EXPECT_GE(vm::CopyMoveCounter::copies(), 1);  // хотя бы одна копия
        }
    }
}

// -----------------------------------------------------------------------------
// Задание 5. is_lvalue — property-тесты (compile-time трейт)
// -----------------------------------------------------------------------------

namespace {
// Хелпер: is_lvalue<T> ОБЯЗАН совпадать со std::is_lvalue_reference<T> и при этом
// is_lvalue_v<T> == is_lvalue<T>::value. Проверяем оба факта одним static_assert.
template <class T>
constexpr bool matches_std_trait() {
    return (vm::is_lvalue<T>::value == std::is_lvalue_reference_v<T>) &&
           (vm::is_lvalue_v<T> == vm::is_lvalue<T>::value);
}
}  // namespace

// Батарея типов: значения, lvalue-ссылки, rvalue-ссылки, const, указатели,
// массивы — трейт обязан совпасть со стандартным на каждом.
TEST(IsLvalueProps, MatchesStandardTraitOverTypeBattery) {
    // Проверяем В РАНТАЙМЕ: на незаполненном стабе кастомный трейт неверен, и EXPECT
    // падает во время выполнения. static_assert на значении трейта сломал бы сборку стаба.
    EXPECT_TRUE((matches_std_trait<int>()));
    EXPECT_TRUE((matches_std_trait<int&>()));
    EXPECT_TRUE((matches_std_trait<int&&>()));
    EXPECT_TRUE((matches_std_trait<const int>()));
    EXPECT_TRUE((matches_std_trait<const int&>()));
    EXPECT_TRUE((matches_std_trait<const int&&>()));
    EXPECT_TRUE((matches_std_trait<volatile int&>()));
    EXPECT_TRUE((matches_std_trait<double>()));
    EXPECT_TRUE((matches_std_trait<double&>()));
    EXPECT_TRUE((matches_std_trait<std::string>()));
    EXPECT_TRUE((matches_std_trait<std::string&>()));
    EXPECT_TRUE((matches_std_trait<std::string&&>()));
    EXPECT_TRUE((matches_std_trait<const std::string&>()));
    EXPECT_TRUE((matches_std_trait<int*>()));
    EXPECT_TRUE((matches_std_trait<int*&>()));
    EXPECT_TRUE((matches_std_trait<int (&)[4]>()));
    EXPECT_TRUE((matches_std_trait<int[4]>()));
    EXPECT_TRUE((matches_std_trait<void>()));
    EXPECT_TRUE((matches_std_trait<void (&)()>()));
}
