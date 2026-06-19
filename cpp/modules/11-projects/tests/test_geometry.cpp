#include <gtest/gtest.h>
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
