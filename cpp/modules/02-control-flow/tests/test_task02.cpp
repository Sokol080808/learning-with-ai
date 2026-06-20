#include <gtest/gtest.h>
#include <random>
#include <numeric>
#include <algorithm>
#include <set>
#include "task02.hpp"

TEST(ControlFlow, Factorial) {
    EXPECT_EQ(factorial(0), 1);
    EXPECT_EQ(factorial(1), 1);
    EXPECT_EQ(factorial(5), 120);
    EXPECT_EQ(factorial(7), 5040);
}

TEST(ControlFlow, Fibonacci) {
    EXPECT_EQ(fib(0), 0);
    EXPECT_EQ(fib(1), 1);
    EXPECT_EQ(fib(2), 1);
    EXPECT_EQ(fib(7), 13);
    EXPECT_EQ(fib(10), 55);
}

TEST(ControlFlow, Gcd) {
    EXPECT_EQ(gcd(12, 8), 4);
    EXPECT_EQ(gcd(17, 5), 1);
    EXPECT_EQ(gcd(100, 25), 25);
    EXPECT_EQ(gcd(0, 9), 9);
}

TEST(ControlFlow, FizzBuzz) {
    EXPECT_EQ(fizzbuzz(1), "1");
    EXPECT_EQ(fizzbuzz(3), "Fizz");
    EXPECT_EQ(fizzbuzz(5), "Buzz");
    EXPECT_EQ(fizzbuzz(15), "FizzBuzz");
    EXPECT_EQ(fizzbuzz(7), "7");
    EXPECT_EQ(fizzbuzz(30), "FizzBuzz");
}

TEST(ControlFlow, AreaOverloads) {
    EXPECT_EQ(area(4), 16);
    EXPECT_EQ(area(3, 4), 12);
    EXPECT_EQ(area(5), 25);
}

TEST(ControlFlow, Lcm) {
    EXPECT_EQ(lcm(4, 6), 12);
    EXPECT_EQ(lcm(3, 5), 15);
    EXPECT_EQ(lcm(6, 6), 6);
    EXPECT_EQ(lcm(21, 6), 42);
    EXPECT_EQ(lcm(0, 7), 0);   // НОК с нулём — ноль
    EXPECT_EQ(lcm(7, 0), 0);
}

TEST(ControlFlow, Sieve) {
    EXPECT_EQ(sieve(1), std::vector<int>{});
    EXPECT_EQ(sieve(2), (std::vector<int>{2}));
    EXPECT_EQ(sieve(10), (std::vector<int>{2, 3, 5, 7}));
    EXPECT_EQ(sieve(20), (std::vector<int>{2, 3, 5, 7, 11, 13, 17, 19}));
    // граница включается: 13 — простое
    EXPECT_EQ(sieve(13), (std::vector<int>{2, 3, 5, 7, 11, 13}));
}

TEST(ControlFlow, BinarySearchFound) {
    std::vector<int> v{1, 3, 5, 7, 9, 11};
    EXPECT_EQ(binary_search(v, 1), 0);
    EXPECT_EQ(binary_search(v, 7), 3);
    EXPECT_EQ(binary_search(v, 11), 5);
    EXPECT_EQ(binary_search(v, 5), 2);
}

TEST(ControlFlow, BinarySearchMissing) {
    std::vector<int> v{1, 3, 5, 7, 9, 11};
    EXPECT_EQ(binary_search(v, 0), -1);   // меньше всех
    EXPECT_EQ(binary_search(v, 4), -1);   // в середине, но отсутствует
    EXPECT_EQ(binary_search(v, 12), -1);  // больше всех
    EXPECT_EQ(binary_search({}, 5), -1);  // пустой вектор
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Эти тесты прогоняют сотни сгенерированных входов и проверяют ИНВАРИАНТЫ
// (рекуррентность, сверку с оракулом из std::, перестановочность, round-trip),
// а не отдельные заранее посчитанные примеры. RNG засеян константой —
// результат повторяется на каждом запуске, CI не «мигает».

namespace {

// Простота через пробное деление — независимый оракул для sieve().
bool is_prime_oracle(int x) {
    if (x < 2) return false;
    for (long long d = 2; d * d <= x; ++d)
        if (x % d == 0) return false;
    return true;
}

} // namespace

// --- factorial: мультипликативная рекуррентность n! * (n+1) == (n+1)! ---
// Граница: 12! = 479001600 ещё помещается в 32-битный int, 13! — уже нет.
TEST(ControlFlowProps, FactorialRecurrence) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> dist(0, 11);
    for (int iter = 0; iter < 400; ++iter) {
        int n = dist(rng);
        // (n+1)! = n! * (n+1)
        EXPECT_EQ(factorial(n + 1), factorial(n) * (n + 1)) << "n=" << n;
        // факториал монотонно не убывает на неотрицательных
        EXPECT_LE(factorial(n), factorial(n + 1)) << "n=" << n;
    }
    // явные краевые случаи
    EXPECT_EQ(factorial(0), 1);
    EXPECT_EQ(factorial(1), 1);
    EXPECT_EQ(factorial(12), 479001600);
}

// --- fib: рекуррентность fib(n) == fib(n-1) + fib(n-2) ---
// Граница: fib(46) = 1836311903 < 2^31-1, fib(47) переполняет int.
TEST(ControlFlowProps, FibonacciRecurrence) {
    std::mt19937 rng(0x1B2D3F4u);
    std::uniform_int_distribution<int> dist(2, 46);
    for (int iter = 0; iter < 400; ++iter) {
        int n = dist(rng);
        EXPECT_EQ(fib(n), fib(n - 1) + fib(n - 2)) << "n=" << n;
        // числа Фибоначчи неотрицательны и не убывают, начиная с fib(1)
        EXPECT_GE(fib(n), fib(n - 1)) << "n=" << n;
    }
    EXPECT_EQ(fib(0), 0);
    EXPECT_EQ(fib(1), 1);
    EXPECT_EQ(fib(46), 1836311903);
}

// --- gcd: сверка с std::gcd, делимость, коммутативность ---
TEST(ControlFlowProps, GcdOracleAndDivides) {
    std::mt19937 rng(0xABCDEF1u);
    std::uniform_int_distribution<int> dist(0, 100000);
    for (int iter = 0; iter < 500; ++iter) {
        int a = dist(rng);
        int b = dist(rng);
        int g = gcd(a, b);
        // совпадает с оракулом стандартной библиотеки
        EXPECT_EQ(g, std::gcd(a, b)) << "a=" << a << " b=" << b;
        // коммутативность
        EXPECT_EQ(gcd(a, b), gcd(b, a)) << "a=" << a << " b=" << b;
        if (a != 0 || b != 0) {
            EXPECT_GT(g, 0) << "a=" << a << " b=" << b;
            // НОД делит оба аргумента
            EXPECT_EQ(a % g, 0) << "a=" << a << " g=" << g;
            EXPECT_EQ(b % g, 0) << "b=" << b << " g=" << g;
        }
    }
    // краевые: один аргумент ноль -> другой; оба нуля -> 0
    EXPECT_EQ(gcd(0, 9), 9);
    EXPECT_EQ(gcd(9, 0), 9);
    EXPECT_EQ(gcd(0, 0), 0);
}

// --- lcm: сверка с std::lcm и тождество gcd(a,b)*lcm(a,b) == a*b ---
// Малый диапазон, чтобы произведение помещалось в int.
TEST(ControlFlowProps, LcmOracleAndIdentity) {
    std::mt19937 rng(0x5EED77u);
    std::uniform_int_distribution<int> dist(1, 1000);
    for (int iter = 0; iter < 500; ++iter) {
        int a = dist(rng);
        int b = dist(rng);
        int l = lcm(a, b);
        EXPECT_EQ(l, std::lcm(a, b)) << "a=" << a << " b=" << b;
        // тождество НОД·НОК = a·b (произведения в этом диапазоне < 2^31)
        EXPECT_EQ(static_cast<long long>(gcd(a, b)) * l,
                  static_cast<long long>(a) * b) << "a=" << a << " b=" << b;
        // НОК кратно обоим аргументам
        EXPECT_EQ(l % a, 0) << "a=" << a << " l=" << l;
        EXPECT_EQ(l % b, 0) << "b=" << b << " l=" << l;
        // коммутативность
        EXPECT_EQ(lcm(a, b), lcm(b, a)) << "a=" << a << " b=" << b;
    }
    // правило нуля
    EXPECT_EQ(lcm(0, 7), 0);
    EXPECT_EQ(lcm(7, 0), 0);
    EXPECT_EQ(lcm(0, 0), 0);
}

// --- fizzbuzz: восстановление по независимому оракулу ---
TEST(ControlFlowProps, FizzBuzzOracle) {
    std::mt19937 rng(0xF12B82Cu);
    std::uniform_int_distribution<int> dist(1, 100000);
    for (int iter = 0; iter < 500; ++iter) {
        int n = dist(rng);
        std::string expected;
        if (n % 3 == 0) expected += "Fizz";
        if (n % 5 == 0) expected += "Buzz";
        if (expected.empty()) expected = std::to_string(n);
        EXPECT_EQ(fizzbuzz(n), expected) << "n=" << n;
    }
    // явные границы классов
    EXPECT_EQ(fizzbuzz(3), "Fizz");
    EXPECT_EQ(fizzbuzz(5), "Buzz");
    EXPECT_EQ(fizzbuzz(15), "FizzBuzz");
    EXPECT_EQ(fizzbuzz(1), "1");
}

// --- area: перегрузки квадрата и прямоугольника ---
TEST(ControlFlowProps, AreaOverloads) {
    std::mt19937 rng(0xA4EA001u);
    std::uniform_int_distribution<int> dist(0, 1000);
    for (int iter = 0; iter < 400; ++iter) {
        int s = dist(rng);
        int w = dist(rng);
        int h = dist(rng);
        // площадь квадрата = s*s
        EXPECT_EQ(area(s), s * s) << "s=" << s;
        // площадь прямоугольника = w*h и симметрична по сторонам
        EXPECT_EQ(area(w, h), w * h) << "w=" << w << " h=" << h;
        EXPECT_EQ(area(w, h), area(h, w)) << "w=" << w << " h=" << h;
        // квадрат как частный случай прямоугольника
        EXPECT_EQ(area(s), area(s, s)) << "s=" << s;
    }
    EXPECT_EQ(area(0), 0);
    EXPECT_EQ(area(0, 5), 0);
}

// --- sieve: всё внутри простое, отсортировано, сверка количества с оракулом ---
TEST(ControlFlowProps, SieveInvariants) {
    std::mt19937 rng(0x51E7E99u);
    std::uniform_int_distribution<int> dist(-5, 300);
    for (int iter = 0; iter < 300; ++iter) {
        int n = dist(rng);
        std::vector<int> primes = sieve(n);
        // строго возрастает + всё в диапазоне [2, n] + каждое реально простое
        for (std::size_t i = 0; i < primes.size(); ++i) {
            EXPECT_TRUE(is_prime_oracle(primes[i])) << "p=" << primes[i];
            EXPECT_GE(primes[i], 2);
            EXPECT_LE(primes[i], n);
            if (i > 0) EXPECT_LT(primes[i - 1], primes[i]);
        }
        // количество совпадает с независимым перебором
        int oracle_count = 0;
        for (int x = 2; x <= n; ++x)
            if (is_prime_oracle(x)) ++oracle_count;
        EXPECT_EQ(static_cast<int>(primes.size()), oracle_count) << "n=" << n;
    }
    // края: при n < 2 — пусто; граница n включается
    EXPECT_TRUE(sieve(1).empty());
    EXPECT_TRUE(sieve(0).empty());
    EXPECT_TRUE(sieve(-7).empty());
    EXPECT_EQ(sieve(2), (std::vector<int>{2}));
    EXPECT_EQ(sieve(13).back(), 13);  // верхняя граница — простое — включена
}

// --- binary_search: round-trip по отсортированному вектору уникальных чисел ---
TEST(ControlFlowProps, BinarySearchRoundTrip) {
    std::mt19937 rng(0xB1A5E55u);
    std::uniform_int_distribution<int> size_dist(0, 60);
    std::uniform_int_distribution<int> val_dist(-200, 200);
    for (int iter = 0; iter < 400; ++iter) {
        // строим отсортированный вектор уникальных значений
        int target_size = size_dist(rng);
        std::set<int> uniq;
        while (static_cast<int>(uniq.size()) < target_size)
            uniq.insert(val_dist(rng));
        std::vector<int> v(uniq.begin(), uniq.end());  // уже отсортирован

        // каждый присутствующий элемент находится по своему индексу (round-trip)
        for (std::size_t i = 0; i < v.size(); ++i)
            EXPECT_EQ(binary_search(v, v[i]), static_cast<int>(i))
                << "i=" << i << " val=" << v[i];

        // отсутствующее значение -> -1, сверка с std::binary_search как оракулом
        int probe = val_dist(rng);
        bool present = std::binary_search(v.begin(), v.end(), probe);
        int res = binary_search(v, probe);
        if (present) {
            ASSERT_NE(res, -1) << "probe=" << probe;
            EXPECT_EQ(v[res], probe) << "probe=" << probe;
        } else {
            EXPECT_EQ(res, -1) << "probe=" << probe;
        }
    }
    // явные края
    EXPECT_EQ(binary_search({}, 5), -1);           // пустой вектор
    EXPECT_EQ(binary_search({42}, 42), 0);         // один элемент — найден
    EXPECT_EQ(binary_search({42}, 7), -1);         // один элемент — нет
}

// ===== Задание 9: print_box =====

// --- Фиксированные тесты ---

TEST(ControlFlow, PrintBoxFixed) {
    // Квадрат 2×2 с дефолтным fill
    EXPECT_EQ(print_box(2), "**\n**");
    // Квадрат 2×2 явно через перегрузку
    EXPECT_EQ(print_box(2, '*'), "**\n**");
    // Прямоугольник 3×1 — одна строка, нет '\n'
    EXPECT_EQ(print_box(3, 1), "***");
    // Прямоугольник 1×3 — три строки по одному символу
    EXPECT_EQ(print_box(1, 3), "*\n*\n*");
    // Дефолтный fill символ — звёздочка
    EXPECT_EQ(print_box(2, 2), "**\n**");
    // Не-дефолтный fill
    EXPECT_EQ(print_box(3, 2, '#'), "###\n###");
    // Одноаргументный вызов берёт перегрузку-квадрат:
    // print_box(3) должна совпадать с print_box(3, 3)
    EXPECT_EQ(print_box(3), print_box(3, 3));
    EXPECT_EQ(print_box(4), print_box(4, 4));
    // Квадрат 1×1
    EXPECT_EQ(print_box(1), "*");
}

// --- Property-тест: инварианты строки для случайных w и h в [1..8] ---
// Инвариант 1: число '\n' == h - 1
// Инвариант 2: общая длина строки == w * h + (h - 1)
// Инвариант 3: каждый символ либо fill, либо '\n'
TEST(ControlFlowProps, PrintBoxInvariants) {
    std::mt19937 rng(0xB0B1DEAu);
    std::uniform_int_distribution<int> dim_dist(1, 8);
    // Несколько fill-символов для разнообразия
    const std::string fills = "*#@+";
    std::uniform_int_distribution<int> fill_dist(0, static_cast<int>(fills.size()) - 1);

    for (int iter = 0; iter < 300; ++iter) {
        int w = dim_dist(rng);
        int h = dim_dist(rng);
        char fill = fills[static_cast<std::size_t>(fill_dist(rng))];

        std::string s = print_box(w, h, fill);

        // Инвариант 1: количество '\n' == h - 1
        int newline_count = 0;
        for (char c : s)
            if (c == '\n') ++newline_count;
        EXPECT_EQ(newline_count, h - 1)
            << "w=" << w << " h=" << h << " fill=" << fill;

        // Инвариант 2: полная длина == w*h + (h-1)
        EXPECT_EQ(static_cast<int>(s.size()), w * h + (h - 1))
            << "w=" << w << " h=" << h << " fill=" << fill;

        // Инвариант 3: каждый символ — либо fill, либо '\n'
        for (char c : s) {
            EXPECT_TRUE(c == fill || c == '\n')
                << "unexpected char '" << c << "' w=" << w << " h=" << h;
        }

        // Инвариант 4: одноаргументный вызов совпадает с квадратом
        std::string sq = print_box(w);
        std::string sq2 = print_box(w, w, '*');
        EXPECT_EQ(sq, sq2) << "square overload mismatch w=" << w;
    }
}
