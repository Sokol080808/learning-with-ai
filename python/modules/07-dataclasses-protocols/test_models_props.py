# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_models.py проверяет ФИКСИРОВАННЫЕ примеры (Circle(2).area() == π·4 и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# огромные координаты, нули, отрицательные размеры, юникод) и проверяет не конкретный ответ,
# а ИНВАРИАНТЫ — свойства, которые обязаны выполняться для ЛЮБОГО входа: симметрия и
# неотрицательность расстояния, неравенство треугольника, сравнение dataclass по значению,
# аддитивность total_area, утиная природа is_iterable. Если твоя реализация верна на примерах,
# но ломается на краевом случае, эти тесты это поймают и покажут минимальный контрпример.

import math

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from models import (
    Circle,
    Point,
    Rectangle,
    Shape,
    is_iterable,
    total_area,
)

# Координаты в разумных пределах: за гранью ~1e6 квадраты разностей начинают терять точность
# у float и сравнения «плывут».
coords = st.floats(min_value=-1e6, max_value=1e6, allow_nan=False, allow_infinity=False)
sizes = st.floats(min_value=0.0, max_value=1e6, allow_nan=False, allow_infinity=False)
points = st.builds(Point, x=coords, y=coords)


# --- Point: dataclass-инварианты __eq__ ---

@given(p=points)
def test_point_equality_is_reflexive(p):
    # Объект всегда равен сам себе.
    assert p == p


@given(x=coords, y=coords)
def test_point_equality_by_value_and_symmetric(x, y):
    a = Point(x, y)
    b = Point(x, y)
    # Разные объекты с одинаковыми полями равны (== по значению, не по адресу)...
    assert a == b
    # ...и равенство симметрично.
    assert b == a
    assert a is not b


@given(p=points, q=points)
def test_point_eq_consistent_with_fields(p, q):
    # p == q ровно тогда, когда совпадают оба поля.
    assert (p == q) == (p.x == q.x and p.y == q.y)


@given(x=coords, y=coords)
def test_point_repr_roundtrips(x, y):
    # @dataclass даёт repr вида Point(x=..., y=...), пригодный для eval обратно в объект.
    p = Point(x, y)
    restored = eval(repr(p), {"Point": Point})
    assert restored == p


# --- Point.distance_to: метрика, у неё строгие свойства ---

@given(p=points)
def test_distance_to_self_is_zero(p):
    assert p.distance_to(p) == pytest.approx(0.0)


@given(p=points, q=points)
def test_distance_is_symmetric(p, q):
    # d(p, q) == d(q, p) для любых точек.
    assert p.distance_to(q) == pytest.approx(q.distance_to(p))


@given(p=points, q=points)
def test_distance_is_nonnegative(p, q):
    assert p.distance_to(q) >= 0.0


@given(p=points, q=points, r=points)
@settings(max_examples=200, deadline=None)
def test_distance_triangle_inequality(p, q, r):
    # Прямой путь не длиннее пути через промежуточную точку.
    direct = p.distance_to(r)
    detour = p.distance_to(q) + q.distance_to(r)
    assert direct <= detour + 1e-6


@given(x1=coords, y1=coords, x2=coords, y2=coords)
def test_distance_matches_pythagoras(x1, y1, x2, y2):
    got = Point(x1, y1).distance_to(Point(x2, y2))
    expected = math.hypot(x1 - x2, y1 - y2)
    assert got == pytest.approx(expected, rel=1e-9, abs=1e-6)


# --- Circle / Rectangle / Shape: полиморфизм area() ---

@given(r=sizes)
def test_circle_area_formula_and_nonnegative(r):
    a = Circle(r).area()
    assert a == pytest.approx(math.pi * r * r)
    assert a >= 0.0


@given(r=st.floats(min_value=1e-3, max_value=1e3, allow_nan=False, allow_infinity=False))
def test_circle_area_scales_quadratically(r):
    # Удвоение радиуса учетверяет площадь.
    assert Circle(2 * r).area() == pytest.approx(4 * Circle(r).area(), rel=1e-9)


@given(w=sizes, h=sizes)
def test_rectangle_area_formula_and_nonnegative(w, h):
    a = Rectangle(w, h).area()
    assert a == pytest.approx(w * h)
    assert a >= 0.0


@given(w=sizes, h=sizes)
def test_rectangle_area_is_symmetric_in_sides(w, h):
    # Перестановка ширины и высоты не меняет площадь.
    assert Rectangle(w, h).area() == pytest.approx(Rectangle(h, w).area())


@given(r=sizes, w=sizes, h=sizes)
def test_shapes_are_shape_instances(r, w, h):
    assert isinstance(Circle(r), Shape)
    assert isinstance(Rectangle(w, h), Shape)


# --- total_area: сумма площадей, аддитивность и согласованность с area() ---

shapes_strategy = st.lists(
    st.one_of(
        st.builds(Circle, r=sizes),
        st.builds(Rectangle, w=sizes, h=sizes),
    ),
    max_size=20,
)


@given(shapes=shapes_strategy)
@settings(max_examples=150, deadline=None)
def test_total_area_equals_sum_of_areas(shapes):
    assert total_area(shapes) == pytest.approx(sum(s.area() for s in shapes))


@given(shapes=shapes_strategy)
@settings(max_examples=150, deadline=None)
def test_total_area_is_nonnegative(shapes):
    assert total_area(shapes) >= 0.0


@given(a=shapes_strategy, b=shapes_strategy)
@settings(max_examples=150, deadline=None)
def test_total_area_additive_over_concatenation(a, b):
    # Площадь склейки списков == сумме площадей частей.
    assert total_area(a + b) == pytest.approx(total_area(a) + total_area(b))


def test_total_area_empty_is_zero():
    assert total_area([]) == pytest.approx(0.0)


# --- is_iterable: утиная типизация — ровно bool и согласован с __iter__ ---

@given(value=st.one_of(st.integers(), st.floats(allow_nan=True), st.booleans(), st.none()))
def test_is_iterable_false_for_scalars(value):
    # Числа, bool, None — не итерируемы; результат ровно False (а не «ложное что-то»).
    assert is_iterable(value) is False


@given(
    value=st.one_of(
        st.lists(st.integers()),
        st.text(),
        st.tuples(st.integers()),
        st.dictionaries(st.integers(), st.integers()),
        st.sets(st.integers()),
        st.frozensets(st.integers()),
    )
)
def test_is_iterable_true_for_containers(value):
    # Контейнеры и строки — итерируемы; результат ровно True.
    assert is_iterable(value) is True


@given(value=st.one_of(st.integers(), st.text(), st.lists(st.integers()), st.none()))
def test_is_iterable_agrees_with_dunder(value):
    # Утиная проверка должна совпадать с наличием __iter__.
    assert is_iterable(value) is hasattr(value, "__iter__")


@given(value=st.one_of(st.integers(), st.text(), st.lists(st.integers()), st.none()))
def test_is_iterable_returns_strict_bool(value):
    result = is_iterable(value)
    assert result is True or result is False


def test_is_iterable_generator_is_true():
    # Генератор — итерируемый объект.
    assert is_iterable(x for x in range(3)) is True
    assert is_iterable(range(5)) is True
