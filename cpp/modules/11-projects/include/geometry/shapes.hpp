#pragma once

namespace geometry {

struct Point {
    double x;
    double y;
};

// Евклидово расстояние между двумя точками.
double distance(Point a, Point b);

}  // namespace geometry
