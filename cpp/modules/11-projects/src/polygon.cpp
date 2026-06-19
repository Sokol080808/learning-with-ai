#include "geometry/polygon.hpp"

namespace geometry {

double perimeter(const std::vector<Point>& pts) {
    const std::size_t n = pts.size();
    if (n < 2) {
        return 0.0;
    }
    double total = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        // (i + 1) % n замыкает контур: последняя точка соединяется с первой.
        total += distance(pts[i], pts[(i + 1) % n]);
    }
    return total;
}

}  // namespace geometry
