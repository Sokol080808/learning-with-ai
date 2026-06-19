// Готовое приложение, использующее библиотеку geometry.
#include <iostream>
#include <vector>
#include "geometry/shapes.hpp"
#include "geometry/polygon.hpp"

int main() {
    using geometry::Point;

    std::vector<Point> square{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
    std::cout << "Расстояние (0,0)-(3,4) = " << geometry::distance({0, 0}, {3, 4}) << "\n";
    std::cout << "Периметр единичного квадрата = " << geometry::perimeter(square) << "\n";
    return 0;
}
