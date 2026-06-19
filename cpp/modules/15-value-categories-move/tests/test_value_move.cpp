#include <gtest/gtest.h>

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
