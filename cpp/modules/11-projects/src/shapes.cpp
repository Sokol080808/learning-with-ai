#include "geometry/shapes.hpp"
#include <cmath>   // std::sqrt, std::abs

namespace geometry {

double distance(Point a, Point b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

namespace metric {

double distance(Point a, Point b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

}  // namespace metric

double euclid_plus_manhattan(Point a, Point b) {
    // Оба имени `distance` видны по короткому имени, поэтому зовём каждое
    // полным именем — компилятор не угадывает, выбор делаем явно.
    return geometry::distance(a, b) + geometry::metric::distance(a, b);
}

}  // namespace geometry
