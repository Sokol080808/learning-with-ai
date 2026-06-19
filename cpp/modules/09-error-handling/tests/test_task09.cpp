#include <gtest/gtest.h>
#include <stdexcept>
#include <random>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <limits>
#include <cstdint>
#include "task09.hpp"

TEST(ErrorHandling, SafeDivide) {
    EXPECT_EQ(safe_divide(10, 2), 5);
    EXPECT_THROW(safe_divide(1, 0), std::invalid_argument);
}

TEST(ErrorHandling, ElementAt) {
    std::vector<int> v{10, 20, 30};
    EXPECT_EQ(element_at(v, 0), 10);
    EXPECT_EQ(element_at(v, 2), 30);
    EXPECT_THROW(element_at(v, 3), std::out_of_range);
    EXPECT_THROW(element_at(v, -1), std::out_of_range);
}

TEST(ErrorHandling, ToInt) {
    EXPECT_EQ(to_int("42"), std::optional<int>(42));
    EXPECT_EQ(to_int("-7"), std::optional<int>(-7));
    EXPECT_EQ(to_int("abc"), std::nullopt);
    EXPECT_EQ(to_int("12abc"), std::nullopt);
    EXPECT_EQ(to_int(""), std::nullopt);
}

TEST(ErrorHandling, FirstEven) {
    EXPECT_EQ(first_even({1, 3, 4, 6}), std::optional<int>(4));
    EXPECT_EQ(first_even({1, 3, 5}), std::nullopt);
    EXPECT_EQ(first_even({}), std::nullopt);
}

TEST(ErrorHandling, Describe) {
    EXPECT_EQ(describe(std::variant<int, std::string>(7)), "int: 7");
    EXPECT_EQ(describe(std::variant<int, std::string>(std::string("hi"))), "string: hi");
}

// --- Задание 6: parse_int ---------------------------------------------------

TEST(ErrorHandling, ParseIntValid) {
    EXPECT_EQ(parse_int("0"), std::optional<int>(0));
    EXPECT_EQ(parse_int("42"), std::optional<int>(42));
    EXPECT_EQ(parse_int("+42"), std::optional<int>(42));
    EXPECT_EQ(parse_int("-7"), std::optional<int>(-7));
    EXPECT_EQ(parse_int("007"), std::optional<int>(7));
    EXPECT_EQ(parse_int("2147483647"), std::optional<int>(2147483647));
    EXPECT_EQ(parse_int("-2147483648"), std::optional<int>(-2147483648));
}

TEST(ErrorHandling, ParseIntInvalid) {
    // Контрольное валидное значение в той же группе — чтобы заглушка,
    // которая всегда возвращает nullopt, не «прошла» этот тест.
    EXPECT_EQ(parse_int("123"), std::optional<int>(123));
    EXPECT_EQ(parse_int(""), std::nullopt);
    EXPECT_EQ(parse_int("abc"), std::nullopt);
    EXPECT_EQ(parse_int("12abc"), std::nullopt);
    EXPECT_EQ(parse_int("  5"), std::nullopt);   // ведущий пробел
    EXPECT_EQ(parse_int("5 "), std::nullopt);    // хвостовой пробел
    EXPECT_EQ(parse_int("+"), std::nullopt);     // только знак, без цифр
    EXPECT_EQ(parse_int("-"), std::nullopt);
    EXPECT_EQ(parse_int("3.14"), std::nullopt);
    EXPECT_EQ(parse_int("1,000"), std::nullopt);
}

TEST(ErrorHandling, ParseIntOverflow) {
    // Граница допустимого должна разбираться, а шаг за неё — нет.
    EXPECT_EQ(parse_int("2147483647"), std::optional<int>(2147483647));
    EXPECT_EQ(parse_int("2147483648"), std::nullopt);    // INT_MAX + 1
    EXPECT_EQ(parse_int("-2147483649"), std::nullopt);   // INT_MIN - 1
    EXPECT_EQ(parse_int("99999999999999999999"), std::nullopt);
}

// --- Задание 7: Expected<T, E> ----------------------------------------------

TEST(ErrorHandling, ExpectedHoldsValue) {
    Expected<int, std::string> e(42);
    EXPECT_TRUE(e.has_value());
    EXPECT_TRUE(static_cast<bool>(e));
    EXPECT_EQ(e.value(), 42);
}

TEST(ErrorHandling, ExpectedHoldsError) {
    Expected<int, std::string> e = Expected<int, std::string>::fail("boom");
    EXPECT_FALSE(e.has_value());
    EXPECT_FALSE(static_cast<bool>(e));
    EXPECT_EQ(e.error(), "boom");
    // А значение у ошибки должно быть недоступно (бросок std::logic_error) —
    // заглушка, читающая variant без проверки, этот тест провалит.
    EXPECT_THROW(e.value(), std::logic_error);
}

TEST(ErrorHandling, ExpectedWrongAccessThrows) {
    Expected<int, std::string> ok(1);
    EXPECT_THROW(ok.error(), std::logic_error);   // у значения нет ошибки

    Expected<int, std::string> bad = Expected<int, std::string>::fail("x");
    EXPECT_THROW(bad.value(), std::logic_error);  // у ошибки нет значения
}

TEST(ErrorHandling, ExpectedSameTypes) {
    // Когда T и E совпадают, fail() обязан создавать именно ошибку.
    Expected<int, int> v(5);
    Expected<int, int> err = Expected<int, int>::fail(5);
    EXPECT_TRUE(v.has_value());
    EXPECT_FALSE(err.has_value());
    EXPECT_EQ(v.value(), 5);
    EXPECT_EQ(err.error(), 5);
}

// --- Задание 8: safe_div + DivideByZero -------------------------------------

TEST(ErrorHandling, SafeDivOk) {
    EXPECT_EQ(safe_div(10, 2), 5);
    EXPECT_EQ(safe_div(-9, 3), -3);
}

TEST(ErrorHandling, SafeDivThrowsCustom) {
    EXPECT_THROW(safe_div(1, 0), DivideByZero);
}

TEST(ErrorHandling, SafeDivThrowsDerivesFromRuntimeError) {
    // DivideByZero — это std::runtime_error, поэтому ловится и по базе.
    EXPECT_THROW(safe_div(1, 0), std::runtime_error);
    try {
        safe_div(7, 0);
        FAIL() << "ожидалось исключение DivideByZero";
    } catch (const std::exception& ex) {
        EXPECT_STREQ(ex.what(), "division by zero");
    }
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо фиксированных примеров проверяем ИНВАРИАНТЫ на множестве
// сгенерированных входов (фиксированный сид -> CI не флакает) плюс явные
// крайние случаи (0/1/MIN/MAX, отрицательные, дубликаты, границы).

// --- safe_divide / safe_div: совпадение с оракулом, исключения --------------

TEST(SafeDivideProps, MatchesOracleAndThrowsOnZero) {
    std::mt19937 rng(0xC0FFEEu);
    // Делитель не равен нулю — результат обязан совпадать с обычным a / b.
    std::uniform_int_distribution<int> da(-100000, 100000);
    for (int i = 0; i < 400; ++i) {
        int a = da(rng);
        int b = da(rng);
        if (b == 0) b = 1;  // нулевой делитель проверяем отдельно ниже
        EXPECT_EQ(safe_divide(a, b), a / b);
        EXPECT_EQ(safe_div(a, b), a / b);
        // Оба варианта деления дают одно и то же значение при b != 0.
        EXPECT_EQ(safe_divide(a, b), safe_div(a, b));
    }
    // При нулевом делителе — тип исключения строго документированный.
    for (int i = 0; i < 50; ++i) {
        int a = da(rng);
        EXPECT_THROW(safe_divide(a, 0), std::invalid_argument);
        EXPECT_THROW(safe_div(a, 0), DivideByZero);
        EXPECT_THROW(safe_div(a, 0), std::runtime_error);  // ловится и по базе
    }
}

TEST(SafeDivideProps, EdgeCasesAroundOneAndExtremes) {
    // Деление на 1 / -1 и крайние значения int.
    EXPECT_EQ(safe_divide(0, 5), 0);
    EXPECT_EQ(safe_divide(7, 1), 7);
    EXPECT_EQ(safe_divide(7, -1), -7);
    EXPECT_EQ(safe_divide(std::numeric_limits<int>::max(), 1),
              std::numeric_limits<int>::max());
    // INT_MIN / -1 переполняет int и поведение UB — поэтому его НЕ трогаем,
    // а проверяем безопасную близкую границу INT_MIN / 1.
    EXPECT_EQ(safe_div(std::numeric_limits<int>::min(), 1),
              std::numeric_limits<int>::min());
}

// --- element_at: границы и согласованность с operator[] ----------------------

TEST(ElementAtProps, InBoundsMatchesIndexingOutOfBoundsThrows) {
    std::mt19937 rng(0x1234ABCDu);
    std::uniform_int_distribution<int> dlen(0, 30);
    std::uniform_int_distribution<int> dval(-1000, 1000);

    for (int iter = 0; iter < 300; ++iter) {
        int n = dlen(rng);
        std::vector<int> v(static_cast<std::size_t>(n));
        for (auto& x : v) x = dval(rng);

        // Любой корректный индекс отдаёт ровно v[index].
        for (int idx = 0; idx < n; ++idx)
            EXPECT_EQ(element_at(v, idx), v[static_cast<std::size_t>(idx)]);

        // index == size и любой больший — выход за границы.
        EXPECT_THROW(element_at(v, n), std::out_of_range);
        std::uniform_int_distribution<int> dbig(n + 1, n + 100);
        EXPECT_THROW(element_at(v, dbig(rng)), std::out_of_range);

        // Любой отрицательный индекс — тоже out_of_range (не UB).
        std::uniform_int_distribution<int> dneg(-100, -1);
        EXPECT_THROW(element_at(v, dneg(rng)), std::out_of_range);
    }
}

TEST(ElementAtProps, EmptyAndSingleton) {
    std::vector<int> empty;
    EXPECT_THROW(element_at(empty, 0), std::out_of_range);
    EXPECT_THROW(element_at(empty, -1), std::out_of_range);

    std::vector<int> one{99};
    EXPECT_EQ(element_at(one, 0), 99);
    EXPECT_THROW(element_at(one, 1), std::out_of_range);
}

// --- to_int / parse_int: round-trip число -> строка -> число -----------------

TEST(ToIntProps, RoundTripsViaToString) {
    std::mt19937 rng(0xBEEFCAFEu);
    std::uniform_int_distribution<int> dval(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());
    for (int i = 0; i < 500; ++i) {
        int n = dval(rng);
        std::string s = std::to_string(n);   // канонический вид без '+', без хвоста
        // to_int обязан восстановить ровно то же число.
        EXPECT_EQ(to_int(s), std::optional<int>(n));
        // parse_int — тоже (он принимает ту же каноническую форму).
        EXPECT_EQ(parse_int(s), std::optional<int>(n));
    }
}

TEST(ToIntProps, TrailingGarbageAlwaysFails) {
    std::mt19937 rng(0x0DDF00Du);
    std::uniform_int_distribution<int> dval(-50000, 50000);
    const std::string tails[] = {"x", " ", "abc", ".0", "_", "!", "00z"};
    for (int i = 0; i < 300; ++i) {
        int n = dval(rng);
        std::string s = std::to_string(n);
        const std::string& tail = tails[static_cast<std::size_t>(rng()) % 7];
        // Любой непустой хвост из не-цифр делает разбор неудачным.
        EXPECT_EQ(to_int(s + tail), std::nullopt);
        EXPECT_EQ(parse_int(s + tail), std::nullopt);
    }
}

// --- parse_int: знаки, переполнение, согласованность с int64-оракулом --------

TEST(ParseIntProps, PlusSignAndNonNegativeAreEquivalent) {
    std::mt19937 rng(0x51514141u);
    std::uniform_int_distribution<int> dval(0, 1000000);
    for (int i = 0; i < 300; ++i) {
        int n = dval(rng);
        std::string s = std::to_string(n);
        // Ведущий '+' допустим и не меняет значения неотрицательного числа.
        EXPECT_EQ(parse_int("+" + s), std::optional<int>(n));
        EXPECT_EQ(parse_int(s), std::optional<int>(n));
    }
    // Строка из одного знака — не число.
    EXPECT_EQ(parse_int("+"), std::nullopt);
    EXPECT_EQ(parse_int("-"), std::nullopt);
}

TEST(ParseIntProps, OverflowOracleAgainstInt64) {
    std::mt19937 rng(0x9E3779B9u);
    // Берём 64-битные числа, заведомо часто выходящие за диапазон int.
    std::uniform_int_distribution<std::int64_t> d(
        static_cast<std::int64_t>(std::numeric_limits<int>::min()) - 5000,
        static_cast<std::int64_t>(std::numeric_limits<int>::max()) + 5000);
    constexpr std::int64_t lo = std::numeric_limits<int>::min();
    constexpr std::int64_t hi = std::numeric_limits<int>::max();

    for (int i = 0; i < 500; ++i) {
        std::int64_t n = d(rng);
        std::string s = std::to_string(n);           // канонический вид
        bool in_range = (n >= lo && n <= hi);
        if (in_range) {
            // Помещается в int — должно успешно разобраться в это же значение.
            EXPECT_EQ(parse_int(s), std::optional<int>(static_cast<int>(n)));
        } else {
            // Не помещается — перелив должен дать nullopt.
            EXPECT_EQ(parse_int(s), std::nullopt);
        }
    }
}

TEST(ParseIntProps, ExplicitBoundaries) {
    EXPECT_EQ(parse_int("2147483647"), std::optional<int>(2147483647));      // INT_MAX
    EXPECT_EQ(parse_int("-2147483648"), std::optional<int>(-2147483648));    // INT_MIN
    EXPECT_EQ(parse_int("2147483648"), std::nullopt);                        // INT_MAX+1
    EXPECT_EQ(parse_int("-2147483649"), std::nullopt);                       // INT_MIN-1
    EXPECT_EQ(parse_int(""), std::nullopt);
    EXPECT_EQ(parse_int("0"), std::optional<int>(0));
    EXPECT_EQ(parse_int("+0"), std::optional<int>(0));
    EXPECT_EQ(parse_int("-0"), std::optional<int>(0));
}

TEST(ParseIntProps, WhitespaceAndStrayCharsRejected) {
    std::mt19937 rng(0x77777777u);
    std::uniform_int_distribution<int> dval(-9999, 9999);
    for (int i = 0; i < 200; ++i) {
        int n = dval(rng);
        std::string s = std::to_string(n);
        // Ведущий/хвостовой пробел недопустим (в отличие от std::stoi).
        EXPECT_EQ(parse_int(" " + s), std::nullopt);
        EXPECT_EQ(parse_int(s + " "), std::nullopt);
    }
}

// --- first_even: инвариант относительно линейного поиска ---------------------

TEST(FirstEvenProps, MatchesLinearScan) {
    std::mt19937 rng(0xFACEFEEDu);
    std::uniform_int_distribution<int> dlen(0, 25);
    std::uniform_int_distribution<int> dval(-1000, 1000);

    for (int iter = 0; iter < 400; ++iter) {
        int n = dlen(rng);
        std::vector<int> v(static_cast<std::size_t>(n));
        for (auto& x : v) x = dval(rng);

        // Независимый оракул: первый x с x % 2 == 0.
        std::optional<int> oracle;
        for (int x : v)
            if (x % 2 == 0) { oracle = x; break; }

        std::optional<int> got = first_even(v);
        EXPECT_EQ(got, oracle);
        // Если значение найдено — оно действительно чётное.
        if (got.has_value())
            EXPECT_EQ(*got % 2, 0);
    }
}

TEST(FirstEvenProps, AllOddGivesNulloptAllEvenGivesFirst) {
    std::mt19937 rng(0xA11EE7u);
    std::uniform_int_distribution<int> dlen(1, 20);
    std::uniform_int_distribution<int> dodd(-500, 500);

    for (int iter = 0; iter < 200; ++iter) {
        int n = dlen(rng);

        // Список только из нечётных -> nullopt.
        std::vector<int> odds(static_cast<std::size_t>(n));
        for (auto& x : odds) { int t = dodd(rng); x = (t % 2 == 0) ? t + 1 : t; }
        EXPECT_EQ(first_even(odds), std::nullopt);

        // Список только из чётных -> вернёт самый первый.
        std::vector<int> evens(static_cast<std::size_t>(n));
        for (auto& x : evens) { int t = dodd(rng); x = (t % 2 != 0) ? t + 1 : t; }
        EXPECT_EQ(first_even(evens), std::optional<int>(evens.front()));
    }

    // Явные крайние случаи.
    EXPECT_EQ(first_even({}), std::nullopt);              // пусто
    EXPECT_EQ(first_even({2}), std::optional<int>(2));    // один чётный
    EXPECT_EQ(first_even({3}), std::nullopt);             // один нечётный
    EXPECT_EQ(first_even({0}), std::optional<int>(0));    // ноль чётный
    EXPECT_EQ(first_even({-4, -3}), std::optional<int>(-4));  // отрицательный чётный
}

// --- describe: round-trip формата для int и string --------------------------

TEST(DescribeProps, IntFormatMatchesToString) {
    std::mt19937 rng(0xD35C71BEu);
    std::uniform_int_distribution<int> dval(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());
    for (int i = 0; i < 400; ++i) {
        int n = dval(rng);
        std::variant<int, std::string> var(n);
        EXPECT_EQ(describe(var), "int: " + std::to_string(n));
    }
    // Крайние значения int.
    EXPECT_EQ(describe(std::variant<int, std::string>(std::numeric_limits<int>::max())),
              "int: " + std::to_string(std::numeric_limits<int>::max()));
    EXPECT_EQ(describe(std::variant<int, std::string>(std::numeric_limits<int>::min())),
              "int: " + std::to_string(std::numeric_limits<int>::min()));
}

TEST(DescribeProps, StringFormatRoundTrips) {
    std::mt19937 rng(0x5712A9C0u);
    std::uniform_int_distribution<int> dlen(0, 24);
    // Печатаемые ASCII-символы, включая пробел.
    std::uniform_int_distribution<int> dch(32, 126);

    for (int iter = 0; iter < 300; ++iter) {
        int len = dlen(rng);
        std::string s;
        s.reserve(static_cast<std::size_t>(len));
        for (int i = 0; i < len; ++i)
            s.push_back(static_cast<char>(dch(rng)));

        std::string out = describe(std::variant<int, std::string>(s));
        const std::string prefix = "string: ";
        // Префикс на месте, а «хвост» точно равен исходной строке (round-trip).
        ASSERT_EQ(out.compare(0, prefix.size(), prefix), 0);
        EXPECT_EQ(out.substr(prefix.size()), s);
    }
    // Пустая строка.
    EXPECT_EQ(describe(std::variant<int, std::string>(std::string(""))), "string: ");
}

// --- Expected<T, E>: инварианты успеха/ошибки и неверного доступа ------------

TEST(ExpectedProps, ValueBranchInvariants) {
    std::mt19937 rng(0xE7EC7EDu);
    std::uniform_int_distribution<int> dval(std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::max());
    for (int i = 0; i < 300; ++i) {
        int n = dval(rng);
        Expected<int, std::string> e(n);
        // Успех: has_value(), bool(), value() возвращает то же значение.
        EXPECT_TRUE(e.has_value());
        EXPECT_TRUE(static_cast<bool>(e));
        EXPECT_EQ(e.value(), n);
        // Чтение ошибки у значения — строго std::logic_error.
        EXPECT_THROW(e.error(), std::logic_error);
    }
}

TEST(ExpectedProps, ErrorBranchInvariants) {
    std::mt19937 rng(0xE770F2Eu);
    std::uniform_int_distribution<int> dcode(-1000000, 1000000);
    for (int i = 0; i < 300; ++i) {
        int code = dcode(rng);
        // T и E совпадают (int,int) — fail() обязан создавать именно ошибку.
        Expected<int, int> e = Expected<int, int>::fail(code);
        EXPECT_FALSE(e.has_value());
        EXPECT_FALSE(static_cast<bool>(e));
        EXPECT_EQ(e.error(), code);
        // Чтение значения у ошибки — строго std::logic_error.
        EXPECT_THROW(e.value(), std::logic_error);
    }
}

TEST(ExpectedProps, ValueAndErrorAreMutuallyExclusive) {
    std::mt19937 rng(0xC0DEFACEu);
    std::uniform_int_distribution<int> coin(0, 1);
    std::uniform_int_distribution<int> dval(-100000, 100000);
    for (int i = 0; i < 400; ++i) {
        int payload = dval(rng);
        if (coin(rng) == 0) {
            Expected<int, std::string> e(payload);
            // Ровно одна из веток истинна: значение есть, ошибки нет.
            EXPECT_TRUE(e.has_value());
            EXPECT_NO_THROW((void)e.value());
            EXPECT_THROW((void)e.error(), std::logic_error);
        } else {
            Expected<int, std::string> e =
                Expected<int, std::string>::fail("err" + std::to_string(payload));
            EXPECT_FALSE(e.has_value());
            EXPECT_NO_THROW((void)e.error());
            EXPECT_THROW((void)e.value(), std::logic_error);
            EXPECT_EQ(e.error(), "err" + std::to_string(payload));
        }
    }
}
