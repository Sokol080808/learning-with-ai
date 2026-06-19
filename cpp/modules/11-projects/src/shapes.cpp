#include "geometry/shapes.hpp"
// #include <cmath>   // std::sqrt

namespace geometry {

double distance(Point a, Point b) {
    // TODO: sqrt((a.x-b.x)^2 + (a.y-b.y)^2)
    (void)a; (void)b;
    return 0.0;
}

namespace metric {

double distance(Point a, Point b) {
    // TODO: манхэттенское расстояние |a.x-b.x| + |a.y-b.y|
    (void)a; (void)b;
    return 0.0;
}

}  // namespace metric

double euclid_plus_manhattan(Point a, Point b) {
    // TODO: вернуть geometry::distance(a,b) + geometry::metric::distance(a,b).
    // Здесь видны ОБА имени `distance` — выбери нужное явно (полным именем или using).
    (void)a; (void)b;
    return 0.0;
}

}  // namespace geometry
