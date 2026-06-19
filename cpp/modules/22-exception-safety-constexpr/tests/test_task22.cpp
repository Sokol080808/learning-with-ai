#include <gtest/gtest.h>
#include <string>
#include <stdexcept>
#include <random>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cstring>
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

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Каждый блок гоняет много сгенерированных входов и проверяет ИНВАРИАНТ, а не
// заранее посчитанный пример. Сид фиксирован (std::mt19937 rng(...)), поэтому
// прогон детерминирован и CI не «мигает». Все свойства верны на эталонном решении.

// -----------------------------------------------------------------------------
// Задание 1. Stack<T> — инварианты против std::vector как оракула.
// -----------------------------------------------------------------------------

// Свойство: Stack ведёт себя как LIFO-обёртка над тем же набором push'ей.
// Сравниваем размер, top и порядок выталкивания со std::vector-оракулом.
TEST(StackProps, BehavesLikeVectorOracle) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> val(-1000, 1000);
    std::uniform_int_distribution<int> len(0, 80);

    for (int iter = 0; iter < 300; ++iter) {
        const int n = len(rng);
        Stack<int> s;
        std::vector<int> oracle;

        for (int i = 0; i < n; ++i) {
            const int v = val(rng);
            s.push(v);
            oracle.push_back(v);
            // После каждого push: размеры совпадают, вершина == последний пуш.
            ASSERT_EQ(s.size(), oracle.size());
            ASSERT_FALSE(s.empty());
            ASSERT_EQ(s.top(), oracle.back());
        }
        ASSERT_EQ(s.empty(), oracle.empty());

        // Выталкиваем всё: порядок ровно обратный порядку добавления (LIFO).
        while (!oracle.empty()) {
            ASSERT_EQ(s.top(), oracle.back());
            s.pop();
            oracle.pop_back();
            ASSERT_EQ(s.size(), oracle.size());
        }
        EXPECT_TRUE(s.empty());
    }
}

// Свойство: capacity() всегда покрывает size() и не убывает по ходу push'ей;
// первый push на пустом стеке резервирует запас (capacity > size) — иначе
// строгая гарантия была бы невозможна без реаллокации на изменённом состоянии.
TEST(StackProps, CapacityNeverShrinksAndCoversSize) {
    std::mt19937 rng(0x5EEDu);
    std::uniform_int_distribution<int> val(-50, 50);
    std::uniform_int_distribution<int> len(1, 100);

    for (int iter = 0; iter < 300; ++iter) {
        const int n = len(rng);
        Stack<int> s;
        std::size_t prev_cap = 0;
        for (int i = 0; i < n; ++i) {
            s.push(val(rng));
            ASSERT_GE(s.capacity(), s.size());   // вектор-инвариант: ёмкость >= размер
            ASSERT_GE(s.capacity(), prev_cap);   // ёмкость монотонно не убывает
            if (i == 0) {
                ASSERT_GT(s.capacity(), s.size());  // запас после первого push
            }
            prev_cap = s.capacity();
        }
        EXPECT_EQ(s.size(), static_cast<std::size_t>(n));
    }
}

// Свойство (строгая гарантия): если копия value бросает прямо на push,
// size() и top() остаются ровно такими, какими были до вызова.
TEST(StackProps, StrongGuaranteeKeepsStateOnRandomThrow) {
    std::mt19937 rng(0xBADF00Du);
    std::uniform_int_distribution<int> val(-1000, 1000);
    std::uniform_int_distribution<int> len(1, 40);

    for (int iter = 0; iter < 200; ++iter) {
        const int n = len(rng);
        Stack<Bomb> s;
        Bomb::copies_until_throw = 1000000;

        int last = 0;
        for (int i = 0; i < n; ++i) {
            last = val(rng);
            s.push(Bomb{last});
        }
        const std::size_t before = s.size();
        ASSERT_EQ(before, static_cast<std::size_t>(n));

        // Взводим «мину»: следующая копия бросит.
        Bomb::copies_until_throw = 0;
        EXPECT_THROW(s.push(Bomb{12345}), std::runtime_error);

        // Снимаем мину и читаем: состояние не изменилось.
        Bomb::copies_until_throw = 1000000;
        EXPECT_EQ(s.size(), before);
        EXPECT_EQ(s.top().value, last);
    }
}

// -----------------------------------------------------------------------------
// Задание 2. ScopeGuard — инварианты срабатывания.
// -----------------------------------------------------------------------------

// Свойство: при выходе из области action выполняется РОВНО один раз тогда и
// только тогда, когда dismiss() не вызывался; при dismiss — ноль раз.
TEST(ScopeGuardProps, FiresExactlyWhenNotDismissed) {
    std::mt19937 rng(0x12345u);
    std::bernoulli_distribution dismiss_it(0.5);

    for (int iter = 0; iter < 400; ++iter) {
        int counter = 0;
        const bool will_dismiss = dismiss_it(rng);
        {
            ScopeGuard g([&] { ++counter; });
            if (will_dismiss) {
                g.dismiss();
            }
        }
        EXPECT_EQ(counter, will_dismiss ? 0 : 1);
    }
}

// Свойство: даже если выход из области происходит по исключению, незаотменённый
// guard выполняет действие ровно один раз (RAII-откат).
TEST(ScopeGuardProps, FiresOnExceptionPathExactlyOnce) {
    std::mt19937 rng(0xABCDEu);
    std::bernoulli_distribution dismiss_it(0.5);

    for (int iter = 0; iter < 300; ++iter) {
        int counter = 0;
        const bool will_dismiss = dismiss_it(rng);
        try {
            ScopeGuard g([&] { ++counter; });
            if (will_dismiss) {
                g.dismiss();
            }
            throw std::runtime_error("unwind");
        } catch (const std::runtime_error&) {
            // перехватили — guard уже отработал при раскрутке
        }
        EXPECT_EQ(counter, will_dismiss ? 0 : 1);
    }
}

// -----------------------------------------------------------------------------
// Задание 3. Buffer — copy-and-swap: глубокая копия, перемещение, swap.
// -----------------------------------------------------------------------------

namespace {
// Сгенерировать Buffer случайной длины, заполнить случайными значениями,
// и вернуть и буфер, и эталонный std::vector с тем же содержимым.
std::vector<int> fill_random(Buffer& b, std::mt19937& rng) {
    std::uniform_int_distribution<int> val(-10000, 10000);
    std::vector<int> mirror(b.size());
    for (std::size_t i = 0; i < b.size(); ++i) {
        const int v = val(rng);
        b.set(i, v);
        mirror[i] = v;
    }
    return mirror;
}
}  // namespace

// Свойство: копия равна оригиналу поэлементно (round-trip), и изменение копии
// не задевает оригинал (независимость = настоящая глубокая копия).
TEST(BufferProps, CopyIsEqualAndIndependent) {
    std::mt19937 rng(0xDEADBEEFu);
    std::uniform_int_distribution<int> len(0, 60);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        Buffer a(n);
        std::vector<int> mirror = fill_random(a, rng);

        Buffer b = a;  // copy ctor
        ASSERT_EQ(b.size(), a.size());
        for (std::size_t i = 0; i < n; ++i) {
            ASSERT_EQ(b.at(i), mirror[i]);
            ASSERT_EQ(b.at(i), a.at(i));
        }

        // Мутируем копию — оригинал обязан остаться прежним.
        for (std::size_t i = 0; i < n; ++i) {
            b.set(i, mirror[i] + 1);
        }
        for (std::size_t i = 0; i < n; ++i) {
            ASSERT_EQ(a.at(i), mirror[i]);
        }
    }
}

// Свойство: operator= (copy-and-swap) даёт глубокую копию любого размера-цели,
// безопасен при самоприсваивании и оставляет содержимое цели независимым.
TEST(BufferProps, AssignmentDeepCopiesAndSelfAssignSafe) {
    std::mt19937 rng(0x0B1EC7u);
    std::uniform_int_distribution<int> len(0, 50);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        Buffer a(n);
        std::vector<int> mirror = fill_random(a, rng);

        Buffer c(static_cast<std::size_t>(len(rng)));  // произвольный исходный размер цели
        c = a;
        ASSERT_EQ(c.size(), a.size());
        for (std::size_t i = 0; i < n; ++i) {
            ASSERT_EQ(c.at(i), mirror[i]);
        }
        // Независимость после присваивания.
        for (std::size_t i = 0; i < n; ++i) {
            c.set(i, mirror[i] - 7);
        }
        for (std::size_t i = 0; i < n; ++i) {
            ASSERT_EQ(a.at(i), mirror[i]);
        }

        // Самоприсваивание не разрушает содержимое.
        a = a;
        ASSERT_EQ(a.size(), n);
        for (std::size_t i = 0; i < n; ++i) {
            ASSERT_EQ(a.at(i), mirror[i]);
        }
    }
}

// Свойство: move забирает содержимое (b == старое a) и опустошает источник.
TEST(BufferProps, MoveTransfersAndEmptiesSource) {
    std::mt19937 rng(0xF00Du);
    std::uniform_int_distribution<int> len(0, 60);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        Buffer a(n);
        std::vector<int> mirror = fill_random(a, rng);

        Buffer b = std::move(a);
        ASSERT_EQ(b.size(), n);
        for (std::size_t i = 0; i < n; ++i) {
            ASSERT_EQ(b.at(i), mirror[i]);
        }
        EXPECT_EQ(a.size(), 0u);  // источник опустошён
    }
}

// Свойство: swap обменивает содержимое (a<->b) целиком и обратим — swap дважды
// возвращает к исходному (инволюция).
TEST(BufferProps, SwapExchangesAndIsInvolution) {
    std::mt19937 rng(0x57AB1Eu);
    std::uniform_int_distribution<int> len(0, 40);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t na = static_cast<std::size_t>(len(rng));
        const std::size_t nb = static_cast<std::size_t>(len(rng));
        Buffer a(na);
        Buffer b(nb);
        std::vector<int> ma = fill_random(a, rng);
        std::vector<int> mb = fill_random(b, rng);

        swap(a, b);
        ASSERT_EQ(a.size(), nb);
        ASSERT_EQ(b.size(), na);
        for (std::size_t i = 0; i < nb; ++i) ASSERT_EQ(a.at(i), mb[i]);
        for (std::size_t i = 0; i < na; ++i) ASSERT_EQ(b.at(i), ma[i]);

        // Второй swap возвращает к исходному.
        swap(a, b);
        ASSERT_EQ(a.size(), na);
        ASSERT_EQ(b.size(), nb);
        for (std::size_t i = 0; i < na; ++i) ASSERT_EQ(a.at(i), ma[i]);
        for (std::size_t i = 0; i < nb; ++i) ASSERT_EQ(b.at(i), mb[i]);
    }
}

// Свойство: at/set вне границ бросают именно std::out_of_range (любой i >= size).
TEST(BufferProps, OutOfRangeThrowsSpecificType) {
    std::mt19937 rng(0x0FF5E7u);
    std::uniform_int_distribution<int> len(0, 30);
    std::uniform_int_distribution<int> beyond(0, 50);

    for (int iter = 0; iter < 300; ++iter) {
        const std::size_t n = static_cast<std::size_t>(len(rng));
        Buffer b(n);
        const std::size_t bad = n + static_cast<std::size_t>(beyond(rng));  // всегда >= n
        EXPECT_THROW(b.at(bad), std::out_of_range);
        EXPECT_THROW(b.set(bad, 1), std::out_of_range);
        // А вот внутри границ — без броска, и round-trip set/at работает.
        if (n > 0) {
            std::uniform_int_distribution<std::size_t> idx(0, n - 1);
            const std::size_t i = idx(rng);
            EXPECT_NO_THROW(b.set(i, 99));
            EXPECT_EQ(b.at(i), 99);
        }
    }
}

// -----------------------------------------------------------------------------
// Задание 4. constexpr-математика — против std::-оракулов и определений.
// -----------------------------------------------------------------------------

// Свойство: gcd делит оба аргумента и совпадает со std::gcd; gcd(0,x)==x;
// коммутативность gcd(a,b)==gcd(b,a).
TEST(ConstexprMathProps, GcdMatchesStdAndDividesBoth) {
    std::mt19937 rng(0x6CD00u);
    std::uniform_int_distribution<unsigned> dist(0, 100000u);

    for (int iter = 0; iter < 500; ++iter) {
        const unsigned a = dist(rng);
        const unsigned b = dist(rng);
        const unsigned g = gcd(a, b);

        ASSERT_EQ(g, std::gcd(a, b));
        ASSERT_EQ(gcd(a, b), gcd(b, a));  // коммутативность
        if (g != 0) {
            ASSERT_EQ(a % g, 0u);
            ASSERT_EQ(b % g, 0u);
        }
    }
    // Явные краевые случаи.
    EXPECT_EQ(gcd(0u, 0u), 0u);
    EXPECT_EQ(gcd(0u, 9u), 9u);
    EXPECT_EQ(gcd(9u, 0u), 9u);
    EXPECT_EQ(gcd(7u, 7u), 7u);
}

// Свойство: is_prime совпадает с наивным пробным делением (независимый оракул)
// на всём диапазоне; явно проверяем 0,1,2 и квадрат простого.
TEST(ConstexprMathProps, IsPrimeMatchesTrialDivisionOracle) {
    auto naive = [](unsigned n) -> bool {
        if (n < 2) return false;
        for (unsigned d = 2; d <= n / d; ++d) {
            if (n % d == 0) return false;
        }
        return true;
    };

    std::mt19937 rng(0x97A1u);
    std::uniform_int_distribution<unsigned> dist(0, 50000u);

    for (int iter = 0; iter < 500; ++iter) {
        const unsigned n = dist(rng);
        ASSERT_EQ(is_prime(n), naive(n)) << "n=" << n;
    }
    // Краевые случаи: 0,1 не простые; 2 простое; квадрат простого составной.
    EXPECT_FALSE(is_prime(0u));
    EXPECT_FALSE(is_prime(1u));
    EXPECT_TRUE(is_prime(2u));
    EXPECT_FALSE(is_prime(49u));   // 7*7
    EXPECT_TRUE(is_prime(7919u));  // известное простое
}

// Свойство: factorial(n) == factorial(n-1)*n (рекуррентность) на безопасном
// для unsigned long long диапазоне (n<=20: 20! ещё помещается).
TEST(ConstexprMathProps, FactorialRecurrenceHolds) {
    EXPECT_EQ(factorial(0u), 1ULL);
    unsigned long long prev = 1ULL;
    for (unsigned n = 1; n <= 20; ++n) {
        const unsigned long long cur = factorial(n);
        EXPECT_EQ(cur, prev * n) << "n=" << n;
        prev = cur;
    }
}

// -----------------------------------------------------------------------------
// Задание 5. cstr_len + reversed — round-trip и оракулы.
// -----------------------------------------------------------------------------

// Свойство: cstr_len совпадает со std::strlen на случайных строках без '\0'.
TEST(ConstexprStrProps, CstrLenMatchesStrlen) {
    std::mt19937 rng(0x57311u);
    std::uniform_int_distribution<int> lendist(0, 64);
    std::uniform_int_distribution<int> chardist(1, 126);  // печатаемые, без '\0'

    for (int iter = 0; iter < 400; ++iter) {
        const int n = lendist(rng);
        std::string str;
        str.reserve(n);
        for (int i = 0; i < n; ++i) {
            str.push_back(static_cast<char>(chardist(rng)));
        }
        ASSERT_EQ(cstr_len(str.c_str()), std::strlen(str.c_str()));
        ASSERT_EQ(cstr_len(str.c_str()), str.size());
    }
    EXPECT_EQ(cstr_len(""), 0u);  // явный край
}

// Свойство: reversed — инволюция (reversed(reversed(x))==x) и реально
// разворачивает (reversed(x)[i] == x[N-1-i]) для фиксированной длины N=8.
TEST(ConstexprStrProps, ReversedIsInvolutionAndReverses) {
    constexpr std::size_t N = 8;
    std::mt19937 rng(0x5EE5u);
    std::uniform_int_distribution<int> val(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        Array<int, N> in{};
        for (std::size_t i = 0; i < N; ++i) {
            in[i] = val(rng);
        }
        Array<int, N> rev = reversed(in);
        // Разворот совпадает с поэлементной зеркалкой.
        for (std::size_t i = 0; i < N; ++i) {
            ASSERT_EQ(rev[i], in[N - 1 - i]);
        }
        // Инволюция: дважды развернули — вернулись к исходному.
        Array<int, N> back = reversed(rev);
        ASSERT_TRUE(back == in);
    }
}

// Свойство: reversed одноэлементного и (вырожденно) согласован с std::reverse
// как оракулом на копии-векторе.
TEST(ConstexprStrProps, ReversedMatchesStdReverseOracle) {
    constexpr std::size_t N = 5;
    std::mt19937 rng(0xC0DEu);
    std::uniform_int_distribution<int> val(-500, 500);

    for (int iter = 0; iter < 300; ++iter) {
        Array<int, N> in{};
        std::vector<int> oracle(N);
        for (std::size_t i = 0; i < N; ++i) {
            const int v = val(rng);
            in[i] = v;
            oracle[i] = v;
        }
        std::reverse(oracle.begin(), oracle.end());
        Array<int, N> got = reversed(in);
        for (std::size_t i = 0; i < N; ++i) {
            ASSERT_EQ(got[i], oracle[i]);
        }
    }
}
