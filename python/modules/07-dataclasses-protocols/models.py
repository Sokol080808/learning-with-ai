# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
from __future__ import annotations

import math
from dataclasses import dataclass


@dataclass
class Point:
    """Точка на плоскости.

    Поля:
        x: float — координата по горизонтали.
        y: float — координата по вертикали.

    Поскольку это @dataclass, __init__, __repr__ и __eq__ генерируются автоматически
    по аннотированным полям. Сравнение (==) идёт по значениям x и y.
    """

    x: float
    y: float

    def distance_to(self, other: "Point") -> float:
        """Евклидово расстояние от этой точки до точки other.

        distance_to между (x1, y1) и (x2, y2) — корень из (x1-x2)^2 + (y1-y2)^2.
        Например, расстояние от (0, 0) до (3, 4) равно 5.0.
        """
        return ((self.x - other.x) ** 2 + (self.y - other.y) ** 2) ** 0.5


class Shape:
    """Базовый класс-абстракция для геометрических фигур.

    Сам по себе «фигуры вообще» не существует, поэтому area() в базовом классе не
    реализована: она лишь объявляет контракт «у наследника будет метод area».
    """

    def area(self) -> float:
        """Площадь фигуры. В базовом классе не определена — переопредели в наследнике."""
        raise NotImplementedError


class Circle(Shape):
    """Круг радиуса r. area() = math.pi * r ** 2."""

    def __init__(self, r: float) -> None:
        """Сохрани радиус в self.r."""
        self.r = r

    def area(self) -> float:
        """Площадь круга: π * r^2 (используй math.pi)."""
        return math.pi * self.r ** 2


class Rectangle(Shape):
    """Прямоугольник ширины w и высоты h. area() = w * h."""

    def __init__(self, w: float, h: float) -> None:
        """Сохрани ширину в self.w и высоту в self.h."""
        self.w = w
        self.h = h

    def area(self) -> float:
        """Площадь прямоугольника: w * h."""
        return self.w * self.h


def total_area(shapes: list[Shape]) -> float:
    """Сумма площадей всех фигур из списка (полиморфизм).

    Не разбирай тип каждой фигуры — просто вызывай area() у каждой и складывай.
    Для пустого списка верни 0.0.
    """
    return sum(shape.area() for shape in shapes)


def is_iterable(obj: object) -> bool:
    """Утиная проверка: можно ли по obj итерироваться.

    Не перечисляй конкретные типы — спроси у объекта про умение (наличие __iter__).
    Списки, строки, словари, множества, генераторы → True; числа, None, функции → False.
    """
    return hasattr(obj, "__iter__")
