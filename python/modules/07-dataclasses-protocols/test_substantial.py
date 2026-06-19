# Тесты к заданию «Protocol + dataclass-фигуры + агрегатор холста».
#
# Запуск: ./python/run.sh 07
# Сейчас — КРАСНЫЕ: стаб кидает NotImplementedError. Реализуй Disk / Rect / Ring /
# describe_canvas в models.py — тесты позеленеют.
import math

import pytest

from models import (
    Disk,
    Drawable,
    Rect,
    Ring,
    describe_canvas,
)


# ---------------------------------------------------------------------------
# Drawable: структурная типизация — isinstance без наследования
# ---------------------------------------------------------------------------

def test_disk_satisfies_drawable_protocol():
    # Disk нигде явно не наследует Drawable, но runtime_checkable позволяет проверить
    assert isinstance(Disk(r=1.0), Drawable)


def test_rect_satisfies_drawable_protocol():
    assert isinstance(Rect(w=2.0, h=3.0), Drawable)


def test_ring_satisfies_drawable_protocol():
    assert isinstance(Ring(outer=5.0, inner=2.0), Drawable)


def test_arbitrary_class_satisfies_drawable_structurally():
    # Посторонний класс, у которого есть нужные методы, тоже удовлетворяет протоколу
    class Star:
        def area(self) -> float:
            return 42.0

        def label(self) -> str:
            return "Star"

    assert isinstance(Star(), Drawable)


def test_class_without_label_does_not_satisfy_drawable():
    class Blob:
        def area(self) -> float:
            return 1.0

    assert not isinstance(Blob(), Drawable)


def test_class_without_area_does_not_satisfy_drawable():
    class Blob:
        def label(self) -> str:
            return "Blob"

    assert not isinstance(Blob(), Drawable)


# ---------------------------------------------------------------------------
# Disk
# ---------------------------------------------------------------------------

def test_disk_area_unit_radius():
    assert Disk(r=1.0).area() == pytest.approx(math.pi)


def test_disk_area_zero_radius():
    assert Disk(r=0.0).area() == pytest.approx(0.0)


def test_disk_area_formula():
    r = 3.5
    assert Disk(r=r).area() == pytest.approx(math.pi * r * r)


def test_disk_label():
    assert Disk(r=3.0).label() == "Disk(r=3.0)"


def test_disk_label_zero():
    assert Disk(r=0.0).label() == "Disk(r=0.0)"


def test_disk_is_dataclass_eq_by_value():
    assert Disk(r=2.0) == Disk(r=2.0)
    assert Disk(r=2.0) != Disk(r=3.0)


def test_disk_repr_readable():
    r = repr(Disk(r=1.5))
    assert "Disk" in r and "1.5" in r


# ---------------------------------------------------------------------------
# Rect
# ---------------------------------------------------------------------------

def test_rect_area():
    assert Rect(w=4.0, h=5.0).area() == pytest.approx(20.0)


def test_rect_area_zero_side():
    assert Rect(w=0.0, h=10.0).area() == pytest.approx(0.0)


def test_rect_area_square():
    assert Rect(w=3.0, h=3.0).area() == pytest.approx(9.0)


def test_rect_label():
    assert Rect(w=4.0, h=5.0).label() == "Rect(4.0x5.0)"


def test_rect_label_zero():
    assert Rect(w=0.0, h=7.0).label() == "Rect(0.0x7.0)"


def test_rect_is_dataclass_eq_by_value():
    assert Rect(w=2.0, h=3.0) == Rect(w=2.0, h=3.0)
    assert Rect(w=2.0, h=3.0) != Rect(w=3.0, h=2.0)


# ---------------------------------------------------------------------------
# Ring
# ---------------------------------------------------------------------------

def test_ring_area_formula():
    outer, inner = 5.0, 3.0
    assert Ring(outer=outer, inner=inner).area() == pytest.approx(
        math.pi * (outer**2 - inner**2)
    )


def test_ring_area_zero_inner_equals_disk():
    r = 4.0
    assert Ring(outer=r, inner=0.0).area() == pytest.approx(Disk(r=r).area())


def test_ring_area_equal_radii_is_zero():
    assert Ring(outer=3.0, inner=3.0).area() == pytest.approx(0.0)


def test_ring_label():
    assert Ring(outer=5.0, inner=2.0).label() == "Ring(outer=5.0, inner=2.0)"


def test_ring_is_dataclass_eq_by_value():
    assert Ring(outer=5.0, inner=2.0) == Ring(outer=5.0, inner=2.0)
    assert Ring(outer=5.0, inner=2.0) != Ring(outer=5.0, inner=1.0)


# ---------------------------------------------------------------------------
# describe_canvas
# ---------------------------------------------------------------------------

def test_describe_canvas_empty():
    assert describe_canvas([]) == []


def test_describe_canvas_single_disk():
    result = describe_canvas([Disk(r=1.0)])
    assert len(result) == 1
    assert result[0] == f"Disk(r=1.0): area={round(math.pi, 4)}"


def test_describe_canvas_sorted_by_area_descending():
    # Rect(2x10)=20 > Disk(r=2)≈12.57 > Ring(5,4)≈28.27... wait, let me use clear values:
    # Rect(3x3)=9, Disk(r=1)≈3.14, Rect(1x1)=1  → sorted: 9, 3.14, 1
    shapes = [Rect(w=1.0, h=1.0), Disk(r=1.0), Rect(w=3.0, h=3.0)]
    result = describe_canvas(shapes)
    areas = [float(s.split("area=")[1]) for s in result]
    assert areas == sorted(areas, reverse=True)


def test_describe_canvas_format_contains_label_and_area():
    shapes = [Rect(w=2.0, h=5.0)]
    result = describe_canvas(shapes)
    assert result[0].startswith("Rect(2.0x5.0): area=")
    assert "10.0" in result[0]


def test_describe_canvas_area_rounded_to_4_places():
    result = describe_canvas([Disk(r=1.0)])
    # math.pi rounded to 4 decimals = 3.1416
    assert result[0].endswith("area=3.1416")


def test_describe_canvas_mixed_shapes_order():
    # Ring(outer=10, inner=0) == Disk(r=10), area≈314.16
    # Rect(5x5)=25
    # Disk(r=1)≈3.14
    shapes = [Rect(w=5.0, h=5.0), Ring(outer=10.0, inner=0.0), Disk(r=1.0)]
    result = describe_canvas(shapes)
    # Largest first: Ring, then Rect, then Disk
    assert result[0].startswith("Ring(")
    assert result[1].startswith("Rect(")
    assert result[2].startswith("Disk(")


def test_describe_canvas_accepts_custom_drawable():
    # describe_canvas принимает ЛЮБОЙ объект, удовлетворяющий протоколу Drawable
    class Triangle:
        def __init__(self, base: float, height: float) -> None:
            self._base = base
            self._height = height

        def area(self) -> float:
            return 0.5 * self._base * self._height

        def label(self) -> str:
            return f"Triangle({self._base}x{self._height})"

    shapes: list[Drawable] = [Triangle(base=4.0, height=3.0), Disk(r=0.5)]
    result = describe_canvas(shapes)
    # Triangle area=6.0 > Disk(r=0.5) area≈0.7854
    assert result[0].startswith("Triangle(")
    assert result[1].startswith("Disk(")
