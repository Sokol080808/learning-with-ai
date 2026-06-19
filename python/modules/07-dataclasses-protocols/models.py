# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 07 стали зелёными.
#
# Подсказки и теория — в README.md рядом. Готовых решений тут нет: твоя задача —
# превратить красные тесты в зелёные, понимая, ПОЧЕМУ так.
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
        raise NotImplementedError("TODO: посчитай евклидово расстояние до other")


class Shape:
    """Базовый класс-абстракция для геометрических фигур.

    Сам по себе «фигуры вообще» не существует, поэтому area() в базовом классе не
    реализована: она лишь объявляет контракт «у наследника будет метод area».
    """

    def area(self) -> float:
        """Площадь фигуры. В базовом классе не определена — переопредели в наследнике."""
        raise NotImplementedError("TODO: базовый Shape площадь не считает — это абстракция")


class Circle(Shape):
    """Круг радиуса r. area() = math.pi * r ** 2."""

    def __init__(self, r: float) -> None:
        """Сохрани радиус в self.r."""
        raise NotImplementedError("TODO: сохрани радиус в self.r")

    def area(self) -> float:
        """Площадь круга: π * r^2 (используй math.pi)."""
        raise NotImplementedError("TODO: верни площадь круга")


class Rectangle(Shape):
    """Прямоугольник ширины w и высоты h. area() = w * h."""

    def __init__(self, w: float, h: float) -> None:
        """Сохрани ширину в self.w и высоту в self.h."""
        raise NotImplementedError("TODO: сохрани w и h")

    def area(self) -> float:
        """Площадь прямоугольника: w * h."""
        raise NotImplementedError("TODO: верни площадь прямоугольника")


def total_area(shapes: list[Shape]) -> float:
    """Сумма площадей всех фигур из списка (полиморфизм).

    Не разбирай тип каждой фигуры — просто вызывай area() у каждой и складывай.
    Для пустого списка верни 0.0.
    """
    raise NotImplementedError("TODO: сложи area() всех фигур")


def is_iterable(obj: object) -> bool:
    """Утиная проверка: можно ли по obj итерироваться.

    Не перечисляй конкретные типы — спроси у объекта про умение (наличие __iter__).
    Списки, строки, словари, множества, генераторы → True; числа, None, функции → False.
    """
    raise NotImplementedError("TODO: проверь объект 'по-утиному'")
