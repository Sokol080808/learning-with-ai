#include <gtest/gtest.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <random>
#include <string>
#include <vector>
#include "geometry/shapes.hpp"
#include "geometry/polygon.hpp"
#include "geometry/textstats.hpp"

using geometry::Point;

TEST(Geometry, Distance) {
    EXPECT_DOUBLE_EQ(geometry::distance({0, 0}, {3, 4}), 5.0);
    EXPECT_DOUBLE_EQ(geometry::distance({1, 1}, {1, 1}), 0.0);
    EXPECT_DOUBLE_EQ(geometry::distance({0, 0}, {0, 2}), 2.0);
}

TEST(Geometry, PerimeterSquare) {
    std::vector<Point> square{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    EXPECT_DOUBLE_EQ(geometry::perimeter(square), 4.0);
}

TEST(Geometry, PerimeterTriangle) {
    std::vector<Point> tri{{0, 0}, {3, 0}, {0, 4}};
    EXPECT_DOUBLE_EQ(geometry::perimeter(tri), 12.0);  // 3 + 4 + 5
}

TEST(Geometry, PerimeterDegenerate) {
    EXPECT_DOUBLE_EQ(geometry::perimeter({}), 0.0);
    EXPECT_DOUBLE_EQ(geometry::perimeter({{1, 1}}), 0.0);
}

// ---------------------------------------------------------------------------
// Задание 4: разрешение имён (using vs полное имя).
// geometry::distance — евклидово, geometry::metric::distance — манхэттенское,
// euclid_plus_manhattan должна вернуть их сумму.
// ---------------------------------------------------------------------------

TEST(NameResolution, ManhattanDistance) {
    EXPECT_DOUBLE_EQ(geometry::metric::distance({0, 0}, {3, 4}), 7.0);
    EXPECT_DOUBLE_EQ(geometry::metric::distance({1, 1}, {1, 1}), 0.0);
    EXPECT_DOUBLE_EQ(geometry::metric::distance({-2, 0}, {1, -4}), 7.0);
}

TEST(NameResolution, EuclidPlusManhattan) {
    // евклид(5) + манхэттен(7) = 12
    EXPECT_DOUBLE_EQ(geometry::euclid_plus_manhattan({0, 0}, {3, 4}), 12.0);
    EXPECT_DOUBLE_EQ(geometry::euclid_plus_manhattan({1, 1}, {1, 1}), 0.0);
    // евклид(0,0)-(0,2)=2, манхэттен=2 → 4
    EXPECT_DOUBLE_EQ(geometry::euclid_plus_manhattan({0, 0}, {0, 2}), 4.0);
}

// ---------------------------------------------------------------------------
// Задание 5: мини-«библиотека» textstats — тестируем её публичный API.
// ---------------------------------------------------------------------------

TEST(TextStats, WordCount) {
    EXPECT_EQ(textstats::word_count(""), 0);
    EXPECT_EQ(textstats::word_count("   "), 0);
    EXPECT_EQ(textstats::word_count("hello"), 1);
    EXPECT_EQ(textstats::word_count("the quick brown fox"), 4);
    EXPECT_EQ(textstats::word_count("  leading and   trailing  "), 3);
    EXPECT_EQ(textstats::word_count("tabs\tand\nnewlines\nhere"), 4);
}

TEST(TextStats, LongestWord) {
    EXPECT_EQ(textstats::longest_word(""), 0);
    EXPECT_EQ(textstats::longest_word("a bb ccc dd"), 3);
    EXPECT_EQ(textstats::longest_word("equal pairs"), 5);
    EXPECT_EQ(textstats::longest_word("  spaced  out  word  "), 6);
}

TEST(TextStats, ToUpper) {
    EXPECT_EQ(textstats::to_upper(""), "");
    EXPECT_EQ(textstats::to_upper("Hello, World!"), "HELLO, WORLD!");
    EXPECT_EQ(textstats::to_upper("mix3d C4se"), "MIX3D C4SE");
    // не-ASCII и цифры не трогаем
    EXPECT_EQ(textstats::to_upper("a1b2"), "A1B2");
}

// ===== РАНДОМИЗИРОВАННЫЕ / PROPERTY-ТЕСТЫ (детерминированы фиксированным сидом) =====
//
// Идея: вместо новых точечных примеров проверяем ИНВАРИАНТЫ на сотнях
// случайных, но воспроизводимых (фиксированный сид std::mt19937) входов.
// Каждый блок свёряется либо с независимым «оракулом», либо с математическим
// свойством, которое обязано держаться на эталонной реализации.

namespace {

// Небольшая «решётка» координат: целые в [-50, 50], чтобы суммы/корни были
// численно устойчивы, но входы всё равно разнообразны.
Point random_point(std::mt19937& rng) {
    std::uniform_int_distribution<int> coord(-50, 50);
    return Point{static_cast<double>(coord(rng)), static_cast<double>(coord(rng))};
}

// Независимый оракул евклидова расстояния (другой порядок операций — не копия src).
double euclid_oracle(Point a, Point b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::hypot(dx, dy);
}

// Независимый оракул манхэттенского расстояния.
double manhattan_oracle(Point a, Point b) {
    return std::fabs(a.x - b.x) + std::fabs(a.y - b.y);
}

// Оракул периметра: явно суммируем длины замыкающих рёбер.
double perimeter_oracle(const std::vector<Point>& pts) {
    if (pts.size() < 2) return 0.0;
    double total = 0.0;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        total += euclid_oracle(pts[i], pts[(i + 1) % pts.size()]);
    }
    return total;
}

// Оракул для textstats: разбиение по любому пробельному символу.
std::vector<std::string> split_ws(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (unsigned char ch : s) {
        if (std::isspace(ch)) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(static_cast<char>(ch));
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// Генератор «текста» из букв, цифр, пунктуации и пробельных символов.
std::string random_text(std::mt19937& rng) {
    static const std::string alphabet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-   \t\n";
    std::uniform_int_distribution<int> len(0, 40);
    std::uniform_int_distribution<std::size_t> pick(0, alphabet.size() - 1);
    const int n = len(rng);
    std::string s;
    s.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) s.push_back(alphabet[pick(rng)]);
    return s;
}

}  // namespace

// --- distance: симметрия, тождество, неотрицательность, оракул, треугольник ---
TEST(GeometryProps, EuclidInvariants) {
    std::mt19937 rng(0xC0FFEEu);
    for (int iter = 0; iter < 400; ++iter) {
        const Point a = random_point(rng);
        const Point b = random_point(rng);
        const Point c = random_point(rng);

        const double dab = geometry::distance(a, b);
        // неотрицательность
        EXPECT_GE(dab, 0.0);
        // совпадение с независимым оракулом
        EXPECT_NEAR(dab, euclid_oracle(a, b), 1e-9);
        // симметрия distance(a,b) == distance(b,a)
        EXPECT_DOUBLE_EQ(dab, geometry::distance(b, a));
        // тождество distance(a,a) == 0
        EXPECT_DOUBLE_EQ(geometry::distance(a, a), 0.0);
        // неравенство треугольника: |ac| <= |ab| + |bc|
        const double dac = geometry::distance(a, c);
        const double dbc = geometry::distance(b, c);
        EXPECT_LE(dac, dab + dbc + 1e-9);
    }
    // явные крайние случаи
    EXPECT_DOUBLE_EQ(geometry::distance({0, 0}, {0, 0}), 0.0);
    EXPECT_DOUBLE_EQ(geometry::distance({-3, -4}, {0, 0}), 5.0);  // отрицательные координаты
}

// --- metric::distance (L1): симметрия, оракул, и L2 <= L1 ---
TEST(GeometryProps, ManhattanInvariants) {
    std::mt19937 rng(0xC0FFEEu);
    for (int iter = 0; iter < 400; ++iter) {
        const Point a = random_point(rng);
        const Point b = random_point(rng);

        const double l1 = geometry::metric::distance(a, b);
        EXPECT_GE(l1, 0.0);
        EXPECT_NEAR(l1, manhattan_oracle(a, b), 1e-9);
        // симметрия
        EXPECT_DOUBLE_EQ(l1, geometry::metric::distance(b, a));
        // тождество
        EXPECT_DOUBLE_EQ(geometry::metric::distance(a, a), 0.0);
        // евклид (L2) никогда не больше манхэттена (L1)
        EXPECT_LE(geometry::distance(a, b), l1 + 1e-9);
    }
    EXPECT_DOUBLE_EQ(geometry::metric::distance({-2, 0}, {1, -4}), 7.0);
}

// --- euclid_plus_manhattan == distance + metric::distance ---
TEST(GeometryProps, EuclidPlusManhattanIsSum) {
    std::mt19937 rng(0xC0FFEEu);
    for (int iter = 0; iter < 400; ++iter) {
        const Point a = random_point(rng);
        const Point b = random_point(rng);
        const double expected = geometry::distance(a, b) + geometry::metric::distance(a, b);
        EXPECT_NEAR(geometry::euclid_plus_manhattan(a, b), expected, 1e-9);
        // ни одна из частей не делает результат отрицательным
        EXPECT_GE(geometry::euclid_plus_manhattan(a, b), 0.0);
    }
    // совпадение точек → 0 + 0 = 0
    EXPECT_DOUBLE_EQ(geometry::euclid_plus_manhattan({7, 7}, {7, 7}), 0.0);
}

// --- perimeter: оракул, неотрицательность, инвариантность к сдвигу/повороту/
//     развороту списка вершин, масштабирование, вырожденные случаи ---
TEST(GeometryProps, PerimeterInvariants) {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int> nverts(0, 8);
    std::uniform_int_distribution<int> shift_dist(-30, 30);
    std::uniform_int_distribution<int> scale_dist(1, 6);

    for (int iter = 0; iter < 300; ++iter) {
        const int n = nverts(rng);
        std::vector<Point> pts;
        pts.reserve(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) pts.push_back(random_point(rng));

        const double p = geometry::perimeter(pts);
        // неотрицательность
        EXPECT_GE(p, 0.0);
        // совпадение с независимым оракулом
        EXPECT_NEAR(p, perimeter_oracle(pts), 1e-9);

        // вырожденность: меньше двух точек → 0
        if (n < 2) {
            EXPECT_DOUBLE_EQ(p, 0.0);
            continue;
        }

        // инвариантность к ПАРАЛЛЕЛЬНОМУ ПЕРЕНОСУ всех вершин
        const double dx = shift_dist(rng), dy = shift_dist(rng);
        std::vector<Point> shifted = pts;
        for (Point& q : shifted) { q.x += dx; q.y += dy; }
        EXPECT_NEAR(geometry::perimeter(shifted), p, 1e-9);

        // инвариантность к ЦИКЛИЧЕСКОМУ СДВИГУ списка вершин (тот же контур)
        std::vector<Point> rotated(pts.begin() + 1, pts.end());
        rotated.push_back(pts.front());
        EXPECT_NEAR(geometry::perimeter(rotated), p, 1e-9);

        // инвариантность к РАЗВОРОТУ порядка обхода (контур тот же, рёбра те же)
        std::vector<Point> reversed_pts(pts.rbegin(), pts.rend());
        EXPECT_NEAR(geometry::perimeter(reversed_pts), p, 1e-9);

        // МАСШТАБИРОВАНИЕ на k множит периметр на k
        const double k = static_cast<double>(scale_dist(rng));
        std::vector<Point> scaled = pts;
        for (Point& q : scaled) { q.x *= k; q.y *= k; }
        EXPECT_NEAR(geometry::perimeter(scaled), p * k, 1e-9);
    }

    // явные крайние случаи
    EXPECT_DOUBLE_EQ(geometry::perimeter({}), 0.0);
    EXPECT_DOUBLE_EQ(geometry::perimeter({{5, 5}}), 0.0);
    // две точки: контур «туда-обратно» → 2 * расстояние
    EXPECT_DOUBLE_EQ(geometry::perimeter({{0, 0}, {3, 4}}), 10.0);
}

// --- word_count: совпадение с оракулом, неотрицательность, инвариант к to_upper ---
TEST(TextStatsProps, WordCountMatchesOracle) {
    std::mt19937 rng(0xC0FFEEu);
    for (int iter = 0; iter < 500; ++iter) {
        const std::string s = random_text(rng);
        const int wc = textstats::word_count(s);
        EXPECT_GE(wc, 0);
        EXPECT_EQ(wc, static_cast<int>(split_ws(s).size()));
        // to_upper не добавляет и не убирает пробелы → счётчик слов неизменен
        EXPECT_EQ(textstats::word_count(textstats::to_upper(s)), wc);
        // слов не больше, чем символов
        EXPECT_LE(wc, static_cast<int>(s.size()));
    }
    EXPECT_EQ(textstats::word_count(""), 0);
    EXPECT_EQ(textstats::word_count("   \t\n  "), 0);  // только пробельные → 0
}

// --- longest_word: совпадение с оракулом, границы, инвариант к to_upper ---
TEST(TextStatsProps, LongestWordMatchesOracle) {
    std::mt19937 rng(0xC0FFEEu);
    for (int iter = 0; iter < 500; ++iter) {
        const std::string s = random_text(rng);
        const auto words = split_ws(s);
        std::size_t expected = 0;
        for (const auto& w : words) expected = std::max(expected, w.size());

        const int lw = textstats::longest_word(s);
        EXPECT_EQ(lw, static_cast<int>(expected));
        // длина не превышает всей строки
        EXPECT_LE(lw, static_cast<int>(s.size()));
        // 0 тогда и только тогда, когда нет ни одного непробельного символа
        EXPECT_EQ(lw == 0, words.empty());
        // to_upper сохраняет длины слов
        EXPECT_EQ(textstats::longest_word(textstats::to_upper(s)), lw);
    }
    EXPECT_EQ(textstats::longest_word(""), 0);
}

// --- to_upper: сохранение длины, идемпотентность, оракул посимвольно ---
TEST(TextStatsProps, ToUpperInvariants) {
    std::mt19937 rng(0xC0FFEEu);
    for (int iter = 0; iter < 500; ++iter) {
        const std::string s = random_text(rng);
        const std::string up = textstats::to_upper(s);

        // длина строки не меняется
        EXPECT_EQ(up.size(), s.size());
        // идемпотентность: to_upper(to_upper(x)) == to_upper(x)
        EXPECT_EQ(textstats::to_upper(up), up);

        // посимвольный оракул: ASCII-строчные → верхний регистр, остальное без изменений
        for (std::size_t i = 0; i < s.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(s[i]);
            const char want = (c >= 'a' && c <= 'z')
                                  ? static_cast<char>(c - 'a' + 'A')
                                  : static_cast<char>(c);
            EXPECT_EQ(up[i], want);
        }
        // ни один пробельный символ не должен «исчезнуть» или появиться
        EXPECT_EQ(textstats::word_count(up), textstats::word_count(s));
    }
    EXPECT_EQ(textstats::to_upper(""), "");
}
