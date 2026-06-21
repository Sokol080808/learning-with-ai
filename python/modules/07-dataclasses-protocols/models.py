# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
from __future__ import annotations

import math
from dataclasses import dataclass, field
from decimal import Decimal, ROUND_HALF_UP
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


# ---------------------------------------------------------------------------
# Существенное задание: Protocol + dataclass-фигуры + агрегатор холста
# ---------------------------------------------------------------------------

@runtime_checkable
class Drawable(Protocol):
    """Структурный протокол для рисуемых фигур.

    Любой класс, у которого есть методы area() и label() с совместимыми
    сигнатурами, автоматически удовлетворяет этому протоколу — без явного
    наследования. @runtime_checkable позволяет проверять протокол через
    isinstance() в момент выполнения программы.
    """

    def area(self) -> float: ...
    def label(self) -> str: ...


@dataclass
class Disk:
    """Диск (заполненный круг) радиуса r.

    Реализует протокол Drawable структурно — без явного наследования.
    """

    r: float

    def area(self) -> float:
        """Площадь диска: π * r²."""
        return math.pi * self.r ** 2

    def label(self) -> str:
        """Метка вида 'Disk(r=<r>)'."""
        return f"Disk(r={self.r})"


@dataclass
class Rect:
    """Прямоугольник шириной w и высотой h.

    Реализует протокол Drawable структурно — без явного наследования.
    """

    w: float
    h: float

    def area(self) -> float:
        """Площадь прямоугольника: w * h."""
        return self.w * self.h

    def label(self) -> str:
        """Метка вида 'Rect(<w>x<h>)'."""
        return f"Rect({self.w}x{self.h})"


@dataclass
class Ring:
    """Кольцо с внешним радиусом outer и внутренним радиусом inner.

    Частный случай: inner == 0 — вырождается в диск; inner == outer — площадь нулевая.
    Реализует протокол Drawable структурно — без явного наследования.
    """

    outer: float
    inner: float

    def area(self) -> float:
        """Площадь кольца: π * (outer² - inner²)."""
        return math.pi * (self.outer ** 2 - self.inner ** 2)

    def label(self) -> str:
        """Метка вида 'Ring(outer=<outer>, inner=<inner>)'."""
        return f"Ring(outer={self.outer}, inner={self.inner})"


def describe_canvas(shapes: list[Drawable]) -> list[str]:
    """Описание холста: список строк для каждой фигуры, отсортированных по убыванию площади.

    Каждая строка имеет вид '<label>: area=<площадь>', где площадь округлена до 4 знаков.
    Принимает любые объекты, удовлетворяющие протоколу Drawable (не только Disk/Rect/Ring).
    Для пустого списка возвращает [].
    """
    ordered = sorted(shapes, key=lambda s: s.area(), reverse=True)
    return [f"{s.label()}: area={round(s.area(), 4)}" for s in ordered]


# ---------------------------------------------------------------------------
# Существенное задание 2: Money — frozen + order + __post_init__
# ---------------------------------------------------------------------------

@dataclass(frozen=True, order=True)
class Money:
    """Денежная сумма с валютой и необязательными метками.

    Демонстрирует три новые идеи одновременно:
    - frozen=True: иммутабельность и хешируемость (можно класть в set/dict).
    - order=True: сравнение и сортировка по (amount, currency, tags).
    - __post_init__: валидация инвариантов после автогенерированного __init__.
    - field(default=()): безопасный иммутабельный дефолт для кортежа тегов.

    Поля:
        amount: Decimal — сумма >= 0; Decimal даёт точную арифметику (без float-погрешности).
        currency: str — код валюты ровно из 3 заглавных латинских букв (напр. "USD").
        tags: tuple[str, ...] — метки; иммутабельны (frozen требует хешируемых полей).
    """

    amount: Decimal
    currency: str
    tags: tuple[str, ...] = field(default=())

    def __post_init__(self) -> None:
        """Проверяет инварианты сразу после автогенерированного __init__.

        Бросает ValueError, если:
        - amount < 0
        - currency не состоит ровно из 3 заглавных латинских букв
        """
        if self.amount < Decimal("0"):
            raise ValueError(
                f"amount не может быть отрицательным: {self.amount}"
            )
        if (
            len(self.currency) != 3
            or not self.currency.isalpha()
            or not self.currency.isupper()
        ):
            raise ValueError(
                f"currency должен быть 3 заглавными буквами: {self.currency!r}"
            )

    def convert(self, rate: Decimal, target_currency: str) -> "Money":
        """Конвертирует сумму по указанному курсу в другую валюту.

        Возвращает новый Money (оригинал не изменяется — frozen).
        Результирующая сумма округляется до 2 знаков (ROUND_HALF_UP).
        Теги не переносятся (новый объект получает пустой кортеж).
        """
        new_amount = (self.amount * rate).quantize(
            Decimal("0.01"), rounding=ROUND_HALF_UP
        )
        return Money(amount=new_amount, currency=target_currency)
