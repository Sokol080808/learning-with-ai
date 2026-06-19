#include <gtest/gtest.h>
#include <string>
#include <stdexcept>
#include "task22.hpp"

// =============================================================================
// Задание 1. Stack<T> — строгая гарантия исключений на push.
// =============================================================================

TEST(Stack, PushTopPopSize) {
    Stack<int> s;
    EXPECT_TRUE(s.empty());
    s.push(1);
    s.push(2);
    s.push(3);
    EXPECT_EQ(s.size(), 3u);
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(s.top(), 3);
    s.pop();
    EXPECT_EQ(s.top(), 2);
    EXPECT_EQ(s.size(), 2u);
}

TEST(Stack, WorksForStrings) {
    Stack<std::string> s;
    s.push("a");
    s.push("b");
    EXPECT_EQ(s.top(), "b");
    EXPECT_EQ(s.size(), 2u);
}

TEST(Stack, TopOnEmptyThrowsOutOfRange) {
    Stack<int> s;
    EXPECT_THROW(s.top(), std::out_of_range);
}

TEST(Stack, PopOnEmptyThrowsOutOfRange) {
    Stack<int> s;
    EXPECT_THROW(s.pop(), std::out_of_range);
}

// Тип, который бросает при копировании после заданного числа успешных копий.
// Нужен, чтобы спровоцировать исключение ровно в момент push.
namespace {
struct Bomb {
    int value{0};
    static inline int copies_until_throw = 1000000;

    Bomb() = default;
    explicit Bomb(int v) : value(v) {}

    Bomb(const Bomb& other) : value(other.value) {
        if (copies_until_throw <= 0) {
            throw std::runtime_error("boom");
        }
        --copies_until_throw;
    }
    Bomb& operator=(const Bomb&) = default;
    Bomb(Bomb&&) noexcept = default;
    Bomb& operator=(Bomb&&) noexcept = default;
};
}  // namespace

TEST(Stack, StrongGuaranteeOnThrowingPush) {
    Stack<Bomb> s;
    Bomb::copies_until_throw = 1000000;
    s.push(Bomb{10});
    s.push(Bomb{20});
    ASSERT_EQ(s.size(), 2u);

    // Следующая копия должна бросить — push обязан оставить стек нетронутым.
    Bomb::copies_until_throw = 0;
    EXPECT_THROW(s.push(Bomb{30}), std::runtime_error);

    Bomb::copies_until_throw = 1000000;  // снять «мину», чтобы читать стек
    EXPECT_EQ(s.size(), 2u);             // состояние не изменилось
    EXPECT_EQ(s.top().value, 20);        // вершина прежняя
}

TEST(Stack, PushReservesAhead) {
    // Строгая гарантия обычно достигается резервом заранее: после первого push
    // запас (capacity) должен быть больше текущего размера, чтобы следующий push
    // не делал реаллокацию (источник броска) при уже изменённом состоянии.
    Stack<int> s;
    s.push(1);
    EXPECT_GT(s.capacity(), s.size());
}

// =============================================================================
// Задание 2. ScopeGuard.
// =============================================================================

TEST(ScopeGuard, RunsActionOnScopeExit) {
    int counter = 0;
    {
        ScopeGuard g([&] { ++counter; });
        EXPECT_EQ(counter, 0);  // ещё не сработал
    }
    EXPECT_EQ(counter, 1);  // деструктор выполнил действие
}

TEST(ScopeGuard, DismissCancelsAction) {
    int counter = 0;
    {
        ScopeGuard g([&] { ++counter; });
        g.dismiss();
    }
    EXPECT_EQ(counter, 0);  // действие отменено
}

TEST(ScopeGuard, RunsOnExceptionPath) {
    int cleaned = 0;
    try {
        ScopeGuard g([&] { ++cleaned; });
        throw std::runtime_error("oops");
    } catch (...) {
    }
    EXPECT_EQ(cleaned, 1);  // при выходе по исключению уборка всё равно случилась
}

TEST(ScopeGuard, RunsExactlyOnce) {
    int counter = 0;
    {
        ScopeGuard g([&] { ++counter; });
    }
    EXPECT_EQ(counter, 1);
}

// =============================================================================
// Задание 3. Buffer — copy-and-swap.
// =============================================================================

TEST(Buffer, ConstructAndAccess) {
    Buffer b(3);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.at(0), 0);
    b.set(1, 42);
    EXPECT_EQ(b.at(1), 42);
}

TEST(Buffer, OutOfRangeThrows) {
    Buffer b(2);
    EXPECT_THROW(b.at(2), std::out_of_range);
    EXPECT_THROW(b.set(5, 1), std::out_of_range);
}

TEST(Buffer, DeepCopyIsIndependent) {
    Buffer a(2);
    a.set(0, 7);
    a.set(1, 8);

    Buffer b = a;          // copy ctor
    b.set(0, 100);         // менять копию...
    EXPECT_EQ(a.at(0), 7); // ...не должно трогать оригинал
    EXPECT_EQ(b.at(0), 100);
}

TEST(Buffer, AssignmentDeepCopies) {
    Buffer a(2);
    a.set(0, 1);
    a.set(1, 2);

    Buffer c(5);
    c = a;                 // operator= (copy-and-swap)
    EXPECT_EQ(c.size(), 2u);
    EXPECT_EQ(c.at(0), 1);
    EXPECT_EQ(c.at(1), 2);

    c.set(0, 999);
    EXPECT_EQ(a.at(0), 1); // независимость после присваивания
}

TEST(Buffer, SelfAssignmentSafe) {
    Buffer a(2);
    a.set(0, 5);
    a.set(1, 6);
    a = a;                 // copy-and-swap обязан переживать самоприсваивание
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a.at(0), 5);
    EXPECT_EQ(a.at(1), 6);
}

TEST(Buffer, MoveLeavesSourceEmpty) {
    Buffer a(3);
    a.set(0, 11);
    Buffer b = std::move(a);
    EXPECT_EQ(b.size(), 3u);
    EXPECT_EQ(b.at(0), 11);
    EXPECT_EQ(a.size(), 0u);  // источник опустошён
}

TEST(Buffer, SwapExchangesContents) {
    Buffer a(1);
    a.set(0, 1);
    Buffer b(2);
    b.set(0, 2);
    b.set(1, 3);

    swap(a, b);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a.at(0), 2);
    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(b.at(0), 1);
}

// =============================================================================
// Задание 4. constexpr-математика.
//
// Когда заработает: раскомментируй блок static_assert ниже — он проверяет
// функции ПРЯМО НА ЭТАПЕ КОМПИЛЯЦИИ (если посчитано неверно, проект не
// соберётся). На стабах он бы не дал собрать модуль, поэтому он закомментирован,
// а RED-тесты ниже считают те же значения в constexpr-переменных в рантайме.
// =============================================================================

/*  Раскомментируй, когда factorial/gcd/is_prime будут готовы:
static_assert(factorial(0) == 1ULL, "0! == 1");
static_assert(factorial(5) == 120ULL, "5! == 120");
static_assert(factorial(10) == 3628800ULL, "10! == 3628800");

static_assert(gcd(0, 9) == 9u, "gcd(0,9) == 9");
static_assert(gcd(54, 24) == 6u, "gcd(54,24) == 6");
static_assert(gcd(17, 5) == 1u, "взаимно простые");

static_assert(!is_prime(1), "1 не простое");
static_assert(is_prime(2), "2 простое");
static_assert(!is_prime(21), "21 == 3*7");
static_assert(is_prime(97), "97 простое");
*/

TEST(ConstexprMath, FactorialIsConstexprAndCorrect) {
    // constexpr-переменная заставляет вычислить factorial в compile-time;
    // на стабе это компилируется (стаб constexpr), но значение неверно → RED.
    constexpr auto f0 = factorial(0);
    constexpr auto f5 = factorial(5);
    constexpr auto f6 = factorial(6);
    EXPECT_EQ(f0, 1ULL);
    EXPECT_EQ(f5, 120ULL);
    EXPECT_EQ(f6, 720ULL);
}

TEST(ConstexprMath, GcdIsConstexprAndCorrect) {
    constexpr auto g1 = gcd(54, 24);
    constexpr auto g2 = gcd(0, 7);
    constexpr auto g3 = gcd(100, 75);
    EXPECT_EQ(g1, 6u);
    EXPECT_EQ(g2, 7u);
    EXPECT_EQ(g3, 25u);
}

TEST(ConstexprMath, IsPrimeIsConstexprAndCorrect) {
    constexpr bool p0 = is_prime(0);
    constexpr bool p1 = is_prime(1);
    constexpr bool p2 = is_prime(2);
    constexpr bool p29 = is_prime(29);
    constexpr bool p100 = is_prime(100);
    EXPECT_FALSE(p0);
    EXPECT_FALSE(p1);
    EXPECT_TRUE(p2);
    EXPECT_TRUE(p29);
    EXPECT_FALSE(p100);
}

// =============================================================================
// Задание 5. cstr_len + constexpr-разворот массива.
//
// Те же правила: блок static_assert закомментирован, чтобы стабы собирались;
// раскомментируй его, когда задания будут готовы — он проверит всё в compile-time.
// =============================================================================

/*  Раскомментируй, когда cstr_len и reversed будут готовы:
static_assert(cstr_len("") == 0u, "пустая строка");
static_assert(cstr_len("hello") == 5u, "hello");
static_assert(cstr_len("constexpr") == 9u, "constexpr");

static_assert([] {
    Array<int, 3> in{{1, 2, 3}};
    Array<int, 3> expected{{3, 2, 1}};
    return reversed(in) == expected;
}(), "reversed разворачивает массив в compile-time");
*/

TEST(Constexpr, CstrLenIsConstexprAndCorrect) {
    constexpr auto l_empty = cstr_len("");
    constexpr auto l_abc = cstr_len("abc");
    constexpr auto l_long = cstr_len("a longer string");
    EXPECT_EQ(l_empty, 0u);
    EXPECT_EQ(l_abc, 3u);
    EXPECT_EQ(l_long, 15u);
}

TEST(Constexpr, ReversedIsConstexprAndCorrect) {
    constexpr Array<int, 3> in{{1, 2, 3}};
    constexpr Array<int, 3> expected{{3, 2, 1}};
    constexpr Array<int, 3> got = reversed(in);
    EXPECT_TRUE(got == expected);
}

TEST(Constexpr, ReversedSingleElement) {
    constexpr Array<int, 1> in{{42}};
    constexpr Array<int, 1> expected{{42}};
    constexpr Array<int, 1> got = reversed(in);
    EXPECT_TRUE(got == expected);
}
