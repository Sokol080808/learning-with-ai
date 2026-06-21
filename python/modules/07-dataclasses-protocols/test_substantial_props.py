# Property-тесты к заданию «Protocol + dataclass-фигуры + агрегатор холста».
#
# hypothesis + derandomize из корневого conftest.py → детерминированные прогоны.
# Проверяем инварианты, а не конкретные числа.
import math
from decimal import Decimal

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from models import (
    Disk,
    Drawable,
    Money,
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


# ---------------------------------------------------------------------------
# Money: property-тесты (seeded, derandomize=True через conftest)
# ---------------------------------------------------------------------------

# Стратегии для Money
_valid_currencies = st.sampled_from(["USD", "EUR", "RUB", "GBP", "CNY", "JPY", "CHF"])
_amounts = st.decimals(
    min_value=Decimal("0"),
    max_value=Decimal("1000000"),
    places=2,
    allow_nan=False,
    allow_infinity=False,
)
_rates = st.decimals(
    min_value=Decimal("0.01"),
    max_value=Decimal("100"),
    places=4,
    allow_nan=False,
    allow_infinity=False,
)
_money = st.builds(Money, amount=_amounts, currency=_valid_currencies)


@given(amount=_amounts, currency=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_valid_inputs_never_raise(amount: Decimal, currency: str) -> None:
    """Валидные входы (amount>=0, currency 3 заглавных буквы) не бросают исключений."""
    m = Money(amount=amount, currency=currency)
    assert m.amount == amount
    assert m.currency == currency


@given(m=_money)
@settings(derandomize=True, max_examples=200)
def test_money_frozen_is_hashable(m: Money) -> None:
    """frozen=True → Money хешируем: можно класть в set и dict."""
    # hash() не бросает
    h = hash(m)
    assert isinstance(h, int)
    # Объект оседает в множестве
    s = {m}
    assert m in s


@given(amount=_amounts, currency=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_eq_reflexive(amount: Decimal, currency: str) -> None:
    """Объект равен сам себе (рефлексивность ==)."""
    m = Money(amount=amount, currency=currency)
    assert m == m


@given(amount=_amounts, currency=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_eq_two_objects_same_fields(amount: Decimal, currency: str) -> None:
    """Два разных объекта с одинаковыми полями равны (== по значению, не по адресу)."""
    m1 = Money(amount=amount, currency=currency)
    m2 = Money(amount=amount, currency=currency)
    assert m1 == m2
    assert m1 is not m2
    # Одинаковый хеш у равных объектов (требование Python)
    assert hash(m1) == hash(m2)


@given(
    a1=_amounts, c1=_valid_currencies,
    a2=_amounts, c2=_valid_currencies,
)
@settings(derandomize=True, max_examples=150)
def test_money_order_consistent_with_fields(
    a1: Decimal, c1: str, a2: Decimal, c2: str
) -> None:
    """order=True: сравнение согласовано с полями; обратная проверка."""
    m1 = Money(a1, c1)
    m2 = Money(a2, c2)
    # Либо m1 < m2, либо m1 == m2, либо m1 > m2 — ровно одно
    lt = m1 < m2
    eq = m1 == m2
    gt = m1 > m2
    assert lt + eq + gt == 1  # ровно одно из трёх истинно


@given(moneys=st.lists(_money, min_size=1, max_size=20))
@settings(derandomize=True, max_examples=150)
def test_money_sorted_nondecreasing(moneys: list) -> None:
    """sorted() работает на списке Money; результат не убывает."""
    result = sorted(moneys)
    for i in range(len(result) - 1):
        assert result[i] <= result[i + 1]


@given(m=_money, rate=_rates, target=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_convert_result_nonnegative(
    m: Money, rate: Decimal, target: str
) -> None:
    """convert всегда возвращает Money с неотрицательной суммой."""
    result = m.convert(rate=rate, target_currency=target)
    assert result.amount >= Decimal("0")


@given(m=_money, rate=_rates, target=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_convert_currency_matches_target(
    m: Money, rate: Decimal, target: str
) -> None:
    """convert устанавливает валюту равной target_currency."""
    result = m.convert(rate=rate, target_currency=target)
    assert result.currency == target


@given(m=_money, rate=_rates, target=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_convert_two_decimal_places(
    m: Money, rate: Decimal, target: str
) -> None:
    """Результат convert всегда имеет ровно 2 знака после запятой."""
    result = m.convert(rate=rate, target_currency=target)
    assert result.amount == result.amount.quantize(Decimal("0.01"))


@given(m=_money, rate=_rates, target=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_convert_original_unchanged(
    m: Money, rate: Decimal, target: str
) -> None:
    """frozen: оригинальный объект не изменяется после convert."""
    original_amount = m.amount
    original_currency = m.currency
    m.convert(rate=rate, target_currency=target)
    assert m.amount == original_amount
    assert m.currency == original_currency


@given(m=_money, rate=_rates, target=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_convert_result_is_money_instance(
    m: Money, rate: Decimal, target: str
) -> None:
    """convert возвращает экземпляр Money (а не что-то другое)."""
    result = m.convert(rate=rate, target_currency=target)
    assert isinstance(result, Money)


@given(m=_money, rate=_rates, target=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_convert_drops_tags(
    m: Money, rate: Decimal, target: str
) -> None:
    """convert не переносит теги — новый объект всегда имеет пустой кортеж."""
    m_with_tags = Money(m.amount, m.currency, tags=("foo", "bar"))
    result = m_with_tags.convert(rate=rate, target_currency=target)
    assert result.tags == ()


@given(amount=_amounts, currency=_valid_currencies)
@settings(derandomize=True, max_examples=200)
def test_money_tags_default_empty_tuple(amount: Decimal, currency: str) -> None:
    """Дефолт поля tags — пустой кортеж, а не список (иммутабельность)."""
    m = Money(amount=amount, currency=currency)
    assert m.tags == ()
    assert isinstance(m.tags, tuple)
