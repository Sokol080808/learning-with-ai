#include <gtest/gtest.h>
#include <vector>
#include "geometry/shapes.hpp"
#include "geometry/polygon.hpp"

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
