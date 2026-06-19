#pragma once

namespace geometry {

struct Point {
    double x;
    double y;
};

// Евклидово расстояние между двумя точками.
double distance(Point a, Point b);

// --- Задание на разрешение имён (using vs полное имя) ---
//
// Во ВЛОЖЕННОМ пространстве имён geometry::metric лежит своя функция distance,
// считающая «манхэттенское» расстояние |dx| + |dy|. Имя `distance` теперь живёт
// в ДВУХ местах: geometry::distance (евклидово) и geometry::metric::distance
// (манхэттенское). Учись их различать.
namespace metric {

// Манхэттенское (L1) расстояние: |a.x - b.x| + |a.y - b.y|.
double distance(Point a, Point b);

}  // namespace metric

// Должна вернуть СУММУ евклидова и манхэттенского расстояний между a и b.
// Внутри тела обе функции `distance` видимы по имени — придётся явно выбрать,
// какую звать (полное имя или using). Объявление здесь, тело — в src/shapes.cpp.
double euclid_plus_manhattan(Point a, Point b);

}  // namespace geometry
