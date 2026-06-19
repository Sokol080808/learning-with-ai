# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_calc.py проверяет ФИКСИРОВАННЫЕ примеры (calc(2, "+", 3) == 5 и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# огромные числа, нули, отрицательные, дробные) и проверяет не конкретный ответ, а
# ИНВАРИАНТЫ — свойства, которые обязаны выполняться для ЛЮБОГО входа. Если твоя
# реализация верна на примерах, но ломается на краевом случае, эти тесты это поймают и
# покажут минимальный контрпример.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from calc import calc

# Числа в разумных пределах: за гранью ~1e9 произведение уже теряет точность у float.
finite = st.floats(min_value=-1e9, max_value=1e9, allow_nan=False, allow_infinity=False)
nonzero = st.floats(
    min_value=-1e9, max_value=1e9, allow_nan=False, allow_infinity=False
).filter(lambda x: abs(x) > 1e-6)


# --- штатные операции: каждая совпадает со своим арифметическим аналогом -----

@given(a=finite, b=finite)
def test_calc_add_matches(a, b):
    assert calc(a, "+", b) == pytest.approx(a + b)


@given(a=finite, b=finite)
def test_calc_sub_matches(a, b):
    assert calc(a, "-", b) == pytest.approx(a - b)


@given(a=finite, b=finite)
def test_calc_mul_matches(a, b):
    assert calc(a, "*", b) == pytest.approx(a * b)


@given(a=finite, b=nonzero)
def test_calc_div_matches(a, b):
    assert calc(a, "/", b) == pytest.approx(a / b)


# --- алгебраические связи между операциями -----------------------------------

@given(a=finite, b=finite)
def test_add_sub_inverse(a, b):
    # Прибавили и тут же вычли — вернулись к a. Допуск растёт с |b|: при b >> a
    # сложение и вычитание у float теряют младшие разряды (катастрофическое сокращение),
    # поэтому точного равенства тут ждать нельзя — только с поправкой на масштаб b.
    assert calc(calc(a, "+", b), "-", b) == pytest.approx(a, rel=1e-9, abs=1e-6 + abs(b) * 1e-9)


@given(a=finite, b=finite)
def test_add_commutative(a, b):
    assert calc(a, "+", b) == pytest.approx(calc(b, "+", a))


@given(a=finite, b=finite)
def test_mul_commutative(a, b):
    assert calc(a, "*", b) == pytest.approx(calc(b, "*", a))


@given(a=nonzero, b=nonzero)
def test_mul_div_inverse(a, b):
    # Умножили и поделили на одно и то же — вернулись к a.
    assert calc(calc(a, "*", b), "/", b) == pytest.approx(a)


# --- ошибки: деление на ноль и неизвестная операция --------------------------

@given(a=finite)
def test_division_by_zero_always_raises(a):
    with pytest.raises(ValueError):
        calc(a, "/", 0)
    with pytest.raises(ValueError):
        calc(a, "/", 0.0)


@given(
    a=finite,
    b=finite,
    op=st.text().filter(lambda s: s not in ("+", "-", "*", "/")),
)
@settings(max_examples=100, deadline=None)
def test_unknown_operation_always_raises(a, b, op):
    # Любая строка, кроме четырёх допустимых операций, должна давать ValueError.
    with pytest.raises(ValueError):
        calc(a, op, b)
