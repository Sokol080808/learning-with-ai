# Property-тесты к заданию «Protocol + dataclass-фигуры + агрегатор холста».
#
# hypothesis + derandomize из корневого conftest.py → детерминированные прогоны.
# Проверяем инварианты, а не конкретные числа.
import math

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from models import (
    Disk,
    Drawable,
    Rect,
    Ring,
    describe_canvas,
)

# ---------------------------------------------------------------------------
# Стратегии
# ---------------------------------------------------------------------------
radii = st.floats(min_value=0.0, max_value=1e4, allow_nan=False, allow_infinity=False)
sides = st.floats(min_value=0.0, max_value=1e4, allow_nan=False, allow_infinity=False)

disks = st.builds(Disk, r=radii)
rects = st.builds(Rect, w=sides, h=sides)


@st.composite
def rings(draw):
    inner = draw(st.floats(min_value=0.0, max_value=5e3, allow_nan=False, allow_infinity=False))
    outer = draw(st.floats(min_value=inner, max_value=1e4, allow_nan=False, allow_infinity=False))
    return Ring(outer=outer, inner=inner)


drawables = st.one_of(disks, rects, rings())
canvas_list = st.lists(drawables, max_size=30)


# ---------------------------------------------------------------------------
# Disk: инварианты площади и метки
# ---------------------------------------------------------------------------

@given(r=radii)
def test_disk_area_nonnegative(r: float) -> None:
    assert Disk(r=r).area() >= 0.0


@given(r=radii)
def test_disk_area_matches_formula(r: float) -> None:
    assert Disk(r=r).area() == pytest.approx(math.pi * r * r, rel=1e-9, abs=1e-12)


@given(r=st.floats(min_value=1e-3, max_value=1e3, allow_nan=False, allow_infinity=False))
def test_disk_area_scales_quadratically(r: float) -> None:
    # Удвоение радиуса → площадь в 4 раза больше
    assert Disk(r=2 * r).area() == pytest.approx(4 * Disk(r=r).area(), rel=1e-9)


@given(r=radii)
def test_disk_label_contains_radius(r: float) -> None:
    lbl = Disk(r=r).label()
    assert "Disk" in lbl
    assert str(r) in lbl


@given(r=radii)
def test_disk_satisfies_drawable_at_runtime(r: float) -> None:
    assert isinstance(Disk(r=r), Drawable)


@given(r=radii)
def test_disk_dataclass_eq_reflexive(r: float) -> None:
    d = Disk(r=r)
    assert d == d


@given(r1=radii, r2=radii)
def test_disk_eq_iff_same_radius(r1: float, r2: float) -> None:
    assert (Disk(r=r1) == Disk(r=r2)) == (r1 == r2)


# ---------------------------------------------------------------------------
# Rect: инварианты площади и метки
# ---------------------------------------------------------------------------

@given(w=sides, h=sides)
def test_rect_area_nonnegative(w: float, h: float) -> None:
    assert Rect(w=w, h=h).area() >= 0.0


@given(w=sides, h=sides)
def test_rect_area_matches_formula(w: float, h: float) -> None:
    assert Rect(w=w, h=h).area() == pytest.approx(w * h, rel=1e-9, abs=1e-12)


@given(w=sides, h=sides)
def test_rect_area_symmetric_in_sides(w: float, h: float) -> None:
    assert Rect(w=w, h=h).area() == pytest.approx(Rect(w=h, h=w).area(), rel=1e-9, abs=1e-12)


@given(w=sides, h=sides)
def test_rect_label_contains_dimensions(w: float, h: float) -> None:
    lbl = Rect(w=w, h=h).label()
    assert "Rect" in lbl
    assert str(w) in lbl
    assert str(h) in lbl


@given(w=sides, h=sides)
def test_rect_satisfies_drawable_at_runtime(w: float, h: float) -> None:
    assert isinstance(Rect(w=w, h=h), Drawable)


# ---------------------------------------------------------------------------
# Ring: инварианты площади и метки
# ---------------------------------------------------------------------------

@given(ring=rings())
def test_ring_area_nonnegative(ring: Ring) -> None:
    assert ring.area() >= 0.0


@given(ring=rings())
def test_ring_area_matches_formula(ring: Ring) -> None:
    expected = math.pi * (ring.outer**2 - ring.inner**2)
    assert ring.area() == pytest.approx(expected, rel=1e-9, abs=1e-12)


@given(r=radii)
def test_ring_inner_zero_equals_disk(r: float) -> None:
    assert Ring(outer=r, inner=0.0).area() == pytest.approx(Disk(r=r).area(), rel=1e-9, abs=1e-12)


@given(r=radii)
def test_ring_equal_radii_area_zero(r: float) -> None:
    assert Ring(outer=r, inner=r).area() == pytest.approx(0.0, abs=1e-9)


@given(ring=rings())
def test_ring_satisfies_drawable_at_runtime(ring: Ring) -> None:
    assert isinstance(ring, Drawable)


@given(ring=rings())
def test_ring_label_contains_outer_and_inner(ring: Ring) -> None:
    lbl = ring.label()
    assert "Ring" in lbl
    assert str(ring.outer) in lbl
    assert str(ring.inner) in lbl


# ---------------------------------------------------------------------------
# describe_canvas: инварианты агрегатора
# ---------------------------------------------------------------------------

@given(shapes=canvas_list)
@settings(max_examples=200, deadline=None)
def test_describe_canvas_length_matches_input(shapes: list) -> None:
    assert len(describe_canvas(shapes)) == len(shapes)


@given(shapes=canvas_list)
@settings(max_examples=200, deadline=None)
def test_describe_canvas_sorted_descending_by_area(shapes: list) -> None:
    result = describe_canvas(shapes)
    # Извлекаем площади из строк вида "...: area=<float>"
    areas = [float(s.split("area=")[1]) for s in result]
    assert areas == sorted(areas, reverse=True)


@given(shapes=canvas_list)
@settings(max_examples=200, deadline=None)
def test_describe_canvas_each_entry_contains_label_and_area_keyword(shapes: list) -> None:
    for entry in describe_canvas(shapes):
        assert ": area=" in entry


@given(shapes=canvas_list)
@settings(max_examples=150, deadline=None)
def test_describe_canvas_area_values_match_shape_areas(shapes: list) -> None:
    # Площади в строках совпадают с round(shape.area(), 4), хотя порядок другой —
    # сравниваем мультимножества
    expected_areas = sorted([round(s.area(), 4) for s in shapes], reverse=True)
    result_areas = sorted(
        [float(entry.split("area=")[1]) for entry in describe_canvas(shapes)],
        reverse=True,
    )
    assert result_areas == pytest.approx(expected_areas, rel=1e-6, abs=1e-9)


@given(shapes=canvas_list)
@settings(max_examples=150, deadline=None)
def test_describe_canvas_each_label_appears_in_entry(shapes: list) -> None:
    # Метка каждой фигуры должна присутствовать в описании холста (в каком-то порядке)
    result = describe_canvas(shapes)
    result_str = "\n".join(result)
    for shape in shapes:
        assert shape.label() in result_str


@given(a=canvas_list, b=canvas_list)
@settings(max_examples=100, deadline=None)
def test_describe_canvas_stable_under_concatenation_count(a: list, b: list) -> None:
    # Длина результата — сумма длин частей
    assert len(describe_canvas(a + b)) == len(a) + len(b)
