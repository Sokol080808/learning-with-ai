# Эти тесты трогать не нужно — это эталон поведения.
#
# Запуск:  ./python/run.sh 07
# Сейчас они КРАСНЫЕ: стаб в models.py кидает NotImplementedError. Реализуешь функции —
# станут зелёными.
import math

import pytest

from models import (
    Circle,
    Point,
    Rectangle,
    Shape,
    is_iterable,
    total_area,
)


# --- Point: @dataclass даёт __init__/__repr__/__eq__ ---

def test_point_fields():
    p = Point(1.0, 2.0)
    assert p.x == 1.0
    assert p.y == 2.0


def test_point_equality_by_value():
    # __eq__ сравнивает по полям, а не по идентичности объекта
    assert Point(1.0, 2.0) == Point(1.0, 2.0)
    assert Point(1.0, 2.0) != Point(1.0, 3.0)


def test_point_repr_is_readable():
    # @dataclass генерирует repr вида Point(x=1.0, y=2.0)
    r = repr(Point(1.0, 2.0))
    assert "Point" in r
    assert "1.0" in r and "2.0" in r


def test_point_distance_pythagorean():
    assert Point(0.0, 0.0).distance_to(Point(3.0, 4.0)) == pytest.approx(5.0)


def test_point_distance_symmetric_and_zero():
    a = Point(1.0, 1.0)
    b = Point(4.0, 5.0)
    assert a.distance_to(b) == pytest.approx(5.0)
    assert b.distance_to(a) == pytest.approx(5.0)
    assert a.distance_to(a) == pytest.approx(0.0)


# --- Shape / Circle / Rectangle: полиморфизм ---

def test_shape_base_is_abstract():
    # Базовый Shape не умеет считать площадь — это намеренная абстракция
    with pytest.raises(NotImplementedError):
        Shape().area()


def test_circle_area():
    assert Circle(2.0).area() == pytest.approx(math.pi * 4.0)
    assert Circle(1.0).area() == pytest.approx(math.pi)


def test_rectangle_area():
    assert Rectangle(2.0, 3.0).area() == pytest.approx(6.0)
    assert Rectangle(5.0, 0.0).area() == pytest.approx(0.0)


def test_circle_and_rectangle_are_shapes():
    assert isinstance(Circle(1.0), Shape)
    assert isinstance(Rectangle(1.0, 1.0), Shape)


def test_total_area_mixed():
    shapes = [Circle(1.0), Rectangle(2.0, 3.0)]
    assert total_area(shapes) == pytest.approx(math.pi + 6.0)


def test_total_area_empty_is_zero():
    assert total_area([]) == pytest.approx(0.0)


def test_total_area_uses_polymorphism():
    # total_area не должна знать конкретные типы: достаточно метода area()
    class Triangle(Shape):
        def __init__(self, base, height):
            self.base = base
            self.height = height

        def area(self):
            return 0.5 * self.base * self.height

    shapes = [Circle(2.0), Rectangle(2.0, 2.0), Triangle(4.0, 3.0)]
    expected = math.pi * 4.0 + 4.0 + 6.0
    assert total_area(shapes) == pytest.approx(expected)


# --- is_iterable: утиная типизация ---

@pytest.mark.parametrize(
    "value",
    [
        [1, 2, 3],
        (1, 2, 3),
        "abc",
        {"a": 1},
        {1, 2, 3},
        range(5),
        (x for x in range(3)),
    ],
)
def test_is_iterable_true(value):
    assert is_iterable(value) is True


@pytest.mark.parametrize(
    "value",
    [
        42,
        3.14,
        None,
        True,
        len,
    ],
)
def test_is_iterable_false(value):
    assert is_iterable(value) is False


def test_is_iterable_returns_bool_not_truthy():
    # Ровно bool, а не «что-то истинное» вроде метода __iter__
    assert is_iterable([1]) is True
    assert is_iterable(7) is False
