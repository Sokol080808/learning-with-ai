#pragma once
#include <vector>
#include "geometry/shapes.hpp"

namespace geometry {

// Периметр замкнутого многоугольника по списку вершин (последняя соединяется с первой).
// Для менее чем двух точек — 0.
double perimeter(const std::vector<Point>& pts);

}  // namespace geometry
