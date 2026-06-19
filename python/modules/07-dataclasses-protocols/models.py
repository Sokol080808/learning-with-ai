# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 07 стали зелёными.
#
# Подсказки и теория — в README.md рядом. Готовых решений тут нет: твоя задача —
# превратить красные тесты в зелёные, понимая, ПОЧЕМУ так.
from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Protocol, runtime_checkable


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


# ---------------------------------------------------------------------------
# Задание (существенное): Protocol + dataclass-фигуры + агрегатор холста
# ---------------------------------------------------------------------------


@runtime_checkable
class Drawable(Protocol):
    """Протокол для объектов, которые можно нарисовать на холсте.

    Любой класс, у которого есть методы ``area()`` и ``label()``, автоматически
    удовлетворяет этому протоколу — без явного наследования (структурная типизация).

    Методы:
        area() -> float
            Площадь объекта; обязана быть >= 0.
        label() -> str
            Короткая читаемая метка, например ``"Disk(r=3.0)"`` или ``"Rect(2x5)"``.
    """

    def area(self) -> float:
        """Площадь объекта (>= 0)."""
        ...

    def label(self) -> str:
        """Короткая метка вида 'ИмяТипа(параметры)'."""
        ...


@dataclass
class Disk:
    """Круг (диск) радиуса r на холсте.

    Поля:
        r: float — радиус (>= 0).

    Методы:
        area() -> float
            π * r²  (используй math.pi).
        label() -> str
            Строка вида ``"Disk(r=<r>)"``, где <r> — значение радиуса.
            Пример: Disk(r=3.0) → "Disk(r=3.0)".
    """

    r: float

    def area(self) -> float:
        """Площадь диска: π * r²."""
        raise NotImplementedError("TODO: верни math.pi * self.r ** 2")

    def label(self) -> str:
        """Метка вида 'Disk(r=<r>)'."""
        raise NotImplementedError("TODO: верни f'Disk(r={self.r})'")


@dataclass
class Rect:
    """Прямоугольник ширины w и высоты h на холсте.

    Поля:
        w: float — ширина (>= 0).
        h: float — высота (>= 0).

    Методы:
        area() -> float
            w * h.
        label() -> str
            Строка вида ``"Rect(<w>x<h>)"``.
            Пример: Rect(w=4.0, h=5.0) → "Rect(4.0x5.0)".
    """

    w: float
    h: float

    def area(self) -> float:
        """Площадь прямоугольника: w * h."""
        raise NotImplementedError("TODO: верни self.w * self.h")

    def label(self) -> str:
        """Метка вида 'Rect(<w>x<h>)'."""
        raise NotImplementedError("TODO: верни f'Rect({self.w}x{self.h})'")


@dataclass
class Ring:
    """Кольцо (полый круг) с внешним радиусом outer и внутренним inner.

    Инварианты:
        0 <= inner <= outer  (конструктор не проверяет это сам — твоя задача
        использовать объект корректно).

    Поля:
        outer: float — внешний радиус.
        inner: float — внутренний радиус (дырка).

    Методы:
        area() -> float
            π * (outer² - inner²).
        label() -> str
            Строка вида ``"Ring(outer=<outer>, inner=<inner>)"``.
            Пример: Ring(outer=5.0, inner=2.0) → "Ring(outer=5.0, inner=2.0)".
    """

    outer: float
    inner: float

    def area(self) -> float:
        """Площадь кольца: π * (outer² - inner²)."""
        raise NotImplementedError("TODO: верни math.pi * (self.outer**2 - self.inner**2)")

    def label(self) -> str:
        """Метка вида 'Ring(outer=<outer>, inner=<inner>)'."""
        raise NotImplementedError("TODO: верни f'Ring(outer={self.outer}, inner={self.inner})'")


def describe_canvas(shapes: list[Drawable]) -> list[str]:
    """Описание холста: метки фигур, отсортированные по убыванию площади.

    Принимает список любых объектов, удовлетворяющих протоколу ``Drawable``
    (структурная типизация: не нужно наследоваться — достаточно иметь методы
    ``area()`` и ``label()``).

    Возвращает список строк вида ``"<label>: area=<площадь>"``, где площадь
    округлена до 4 знаков после запятой (``round(area, 4)``).
    Список отсортирован по убыванию площади (самая большая фигура — первой).
    Для пустого списка возвращает [].

    Пример:
        >>> describe_canvas([Rect(w=2.0, h=3.0), Disk(r=1.0)])
        ['Rect(2.0x3.0): area=6.0', 'Disk(r=1.0): area=3.1416']
    """
    raise NotImplementedError("TODO: отсортируй shapes по убыванию area() и собери строки")
