# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_oop.py проверяет ФИКСИРОВАННЫЕ примеры (Fraction(2, 4) -> 1/2 и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни пар (num, den), включая
# отрицательные, нулевые числители, огромные значения, и проверяет не конкретный ответ, а
# ИНВАРИАНТЫ класса Fraction: дробь всегда нормализована (den > 0, gcd == 1), __eq__
# рефлексивно/симметрично и согласовано с математическим равенством, add/mul совпадают с
# точной арифметикой и не мутируют операнды, value совпадает с num/den. Если реализация верна
# на примерах, но ломается на краевом случае, эти тесты это поймают и покажут контрпример.

from math import gcd

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from oop import Fraction

# Числители — любые (вкл. ноль и отрицательные); знаменатели — НЕнулевые (ноль проверяем отдельно).
nums = st.integers(min_value=-10**6, max_value=10**6)
nonzero_dens = st.integers(min_value=-10**6, max_value=10**6).filter(lambda d: d != 0)


@st.composite
def fractions(draw):
    return Fraction(draw(nums), draw(nonzero_dens))


def _is_normalized(f: Fraction) -> bool:
    if f.den <= 0:
        return False
    if f.num == 0:
        return f.den == 1
    return gcd(abs(f.num), f.den) == 1


# --- Инвариант нормализации: верен для ЛЮБОЙ построенной дроби ---

@given(num=nums, den=nonzero_dens)
def test_construction_is_always_normalized(num, den):
    f = Fraction(num, den)
    assert f.den > 0
    assert _is_normalized(f)


@given(num=nums, den=nonzero_dens)
def test_construction_preserves_value(num, den):
    # Нормализация не меняет математическое значение дроби.
    f = Fraction(num, den)
    assert f.num * den == f.den * num


@given(den=nonzero_dens)
def test_zero_numerator_becomes_zero_over_one(den):
    f = Fraction(0, den)
    assert (f.num, f.den) == (0, 1)


@given(num=nums)
def test_zero_denominator_always_raises(num):
    with pytest.raises(ValueError):
        Fraction(num, 0)


# --- Идемпотентность нормализации: уже сокращённую дробь повторно строить не меняя ---

@given(num=nums, den=nonzero_dens)
def test_renormalizing_is_idempotent(num, den):
    f = Fraction(num, den)
    g = Fraction(f.num, f.den)
    assert (g.num, g.den) == (f.num, f.den)


# --- __eq__: рефлексивность, симметрия, согласие с математическим равенством ---

@given(num=nums, den=nonzero_dens)
def test_eq_is_reflexive(num, den):
    f = Fraction(num, den)
    assert f == f


@given(f=fractions(), g=fractions())
def test_eq_is_symmetric(f, g):
    assert (f == g) == (g == f)


@given(num=nums, den=nonzero_dens)
def test_equal_when_built_from_same_args(num, den):
    # Два независимо построенных объекта с теми же аргументами равны (равенство по содержимому).
    assert Fraction(num, den) == Fraction(num, den)


@given(num=nums, den=nonzero_dens, k=st.integers(min_value=1, max_value=1000))
def test_scaled_fraction_is_equal(num, den, k):
    # num/den и (k*num)/(k*den) — одна и та же дробь.
    assert Fraction(num, den) == Fraction(k * num, k * den)


@given(f=fractions(), other=st.one_of(st.text(), st.floats(allow_nan=False), st.integers()))
def test_eq_with_foreign_type_returns_notimplemented(f, other):
    # Прямой вызов __eq__ с чужим типом обязан вернуть NotImplemented (не False, не исключение).
    assert f.__eq__(other) is NotImplemented


# --- add: совпадает с точной арифметикой, коммутативна, не мутирует операнды ---

@given(a=nums, b=nonzero_dens, c=nums, d=nonzero_dens)
def test_add_matches_exact_arithmetic(a, b, c, d):
    result = Fraction(a, b).add(Fraction(c, d))
    # a/b + c/d = (a*d + c*b)/(b*d); сравним перекрёстным умножением, чтобы не зависеть от знака.
    assert result.num * (b * d) == result.den * (a * d + c * b)
    assert _is_normalized(result)


@given(a=nums, b=nonzero_dens, c=nums, d=nonzero_dens)
def test_add_is_commutative(a, b, c, d):
    assert Fraction(a, b).add(Fraction(c, d)) == Fraction(c, d).add(Fraction(a, b))


@given(num=nums, den=nonzero_dens)
def test_add_zero_is_identity(num, den):
    f = Fraction(num, den)
    assert f.add(Fraction(0, 1)) == f


@given(f=fractions(), g=fractions())
def test_add_does_not_mutate_operands(f, g):
    fn, fd, gn, gd = f.num, f.den, g.num, g.den
    result = f.add(g)
    assert result is not f and result is not g
    assert (f.num, f.den) == (fn, fd)
    assert (g.num, g.den) == (gn, gd)


# --- mul: совпадает с точной арифметикой, коммутативна, ноль поглощает ---

@given(a=nums, b=nonzero_dens, c=nums, d=nonzero_dens)
def test_mul_matches_exact_arithmetic(a, b, c, d):
    result = Fraction(a, b).mul(Fraction(c, d))
    # a/b * c/d = (a*c)/(b*d)
    assert result.num * (b * d) == result.den * (a * c)
    assert _is_normalized(result)


@given(a=nums, b=nonzero_dens, c=nums, d=nonzero_dens)
def test_mul_is_commutative(a, b, c, d):
    assert Fraction(a, b).mul(Fraction(c, d)) == Fraction(c, d).mul(Fraction(a, b))


@given(num=nums, den=nonzero_dens)
def test_mul_by_one_is_identity(num, den):
    f = Fraction(num, den)
    assert f.mul(Fraction(1, 1)) == f


@given(num=nums, den=nonzero_dens)
def test_mul_by_zero_is_zero(num, den):
    f = Fraction(num, den)
    assert f.mul(Fraction(0, 1)) == Fraction(0, 1)


@given(f=fractions(), g=fractions())
def test_mul_does_not_mutate_operands(f, g):
    fn, fd, gn, gd = f.num, f.den, g.num, g.den
    f.mul(g)
    assert (f.num, f.den) == (fn, fd)
    assert (g.num, g.den) == (gn, gd)


# --- value и __repr__: связь со значением и формой строки ---

@given(num=nums, den=nonzero_dens)
def test_value_equals_num_over_den(num, den):
    f = Fraction(num, den)
    assert isinstance(f.value, float)
    assert f.value == f.num / f.den


@given(num=nums, den=nonzero_dens)
def test_repr_has_slash_form(num, den):
    f = Fraction(num, den)
    r = repr(f)
    assert r == f"{f.num}/{f.den}"
    assert "/" in r
    # repr можно «прочитать обратно» в числитель и знаменатель.
    left, _, right = r.partition("/")
    assert int(left) == f.num
    assert int(right) == f.den


def test_repr_extremes():
    assert repr(Fraction(0, 5)) == "0/1"
    assert repr(Fraction(3, -9)) == "-1/3"
    assert repr(Fraction(-1, -2)) == "1/2"
