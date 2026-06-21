# Эти тесты трогать не нужно — это эталон поведения.
# Модуль 06 — ООП: классы, dunder-методы, свойства.
# Запуск:  ./python/run.sh 06

import pytest

from oop import Fraction


# --- Нормализация в __init__ -------------------------------------------------

def test_normalize_reduces_by_gcd():
    f = Fraction(2, 4)
    assert (f.num, f.den) == (1, 2)


def test_normalize_already_reduced():
    f = Fraction(3, 5)
    assert (f.num, f.den) == (3, 5)


def test_normalize_improper_fraction():
    f = Fraction(6, 3)
    assert (f.num, f.den) == (2, 1)


def test_sign_moves_to_numerator():
    f = Fraction(1, -2)
    assert (f.num, f.den) == (-1, 2)


def test_negative_numerator_stays():
    f = Fraction(-4, 8)
    assert (f.num, f.den) == (-1, 2)


def test_two_negatives_become_positive():
    f = Fraction(-1, -2)
    assert (f.num, f.den) == (1, 2)


def test_denominator_is_always_positive():
    f = Fraction(3, -9)
    assert f.den > 0
    assert (f.num, f.den) == (-1, 3)


def test_zero_normalizes_to_zero_over_one():
    f = Fraction(0, 5)
    assert (f.num, f.den) == (0, 1)


def test_zero_numerator_negative_denominator():
    f = Fraction(0, -7)
    assert (f.num, f.den) == (0, 1)


def test_zero_denominator_raises_value_error():
    with pytest.raises(ValueError):
        Fraction(1, 0)


def test_zero_over_zero_also_raises():
    with pytest.raises(ValueError):
        Fraction(0, 0)


# --- Сложение (add) ----------------------------------------------------------

def test_add_different_denominators():
    result = Fraction(1, 2).add(Fraction(1, 3))
    assert (result.num, result.den) == (5, 6)


def test_add_result_is_normalized():
    result = Fraction(1, 2).add(Fraction(1, 2))
    assert (result.num, result.den) == (1, 1)


def test_add_to_zero():
    result = Fraction(1, 2).add(Fraction(-1, 2))
    assert (result.num, result.den) == (0, 1)


def test_add_with_negative():
    result = Fraction(1, 3).add(Fraction(-1, 6))
    assert (result.num, result.den) == (1, 6)


def test_add_returns_new_object_and_keeps_operands():
    a = Fraction(1, 2)
    b = Fraction(1, 3)
    result = a.add(b)
    assert result is not a and result is not b
    assert (a.num, a.den) == (1, 2)
    assert (b.num, b.den) == (1, 3)


# --- Умножение (mul) ---------------------------------------------------------

def test_mul_basic():
    result = Fraction(2, 3).mul(Fraction(3, 4))
    assert (result.num, result.den) == (1, 2)


def test_mul_with_negative():
    result = Fraction(-1, 2).mul(Fraction(1, 3))
    assert (result.num, result.den) == (-1, 6)


def test_mul_two_negatives():
    result = Fraction(-1, 2).mul(Fraction(-2, 3))
    assert (result.num, result.den) == (1, 3)


def test_mul_by_zero():
    result = Fraction(7, 9).mul(Fraction(0, 1))
    assert (result.num, result.den) == (0, 1)


def test_mul_returns_new_object():
    a = Fraction(2, 3)
    b = Fraction(3, 4)
    result = a.mul(b)
    assert result is not a and result is not b


# --- Равенство (__eq__) ------------------------------------------------------

def test_eq_same_content():
    assert Fraction(1, 2) == Fraction(1, 2)


def test_eq_after_normalization():
    assert Fraction(2, 4) == Fraction(1, 2)
    assert Fraction(-1, 2) == Fraction(1, -2)


def test_not_eq_different_fractions():
    assert Fraction(1, 2) != Fraction(1, 3)


def test_eq_with_non_fraction_is_false():
    assert (Fraction(1, 2) == "1/2") is False
    assert (Fraction(1, 2) == 0.5) is False


def test_ne_with_non_fraction_is_true():
    assert (Fraction(1, 2) != "1/2") is True


def test_eq_returns_notimplemented_for_other_type():
    # __eq__ напрямую должен вернуть NotImplemented для чужого типа,
    # чтобы Python мог попробовать сравнение с другой стороны.
    assert Fraction(1, 2).__eq__("nope") is NotImplemented


# --- Представление (__repr__ и __str__) -------------------------------------

def test_repr_simple():
    assert repr(Fraction(1, 2)) == "Fraction(1, 2)"


def test_repr_normalizes_before_printing():
    assert repr(Fraction(2, 4)) == "Fraction(1, 2)"


def test_repr_negative():
    assert repr(Fraction(1, -2)) == "Fraction(-1, 2)"


def test_repr_integer_like():
    assert repr(Fraction(6, 3)) == "Fraction(2, 1)"


def test_repr_zero():
    assert repr(Fraction(0, 9)) == "Fraction(0, 1)"


def test_str_simple():
    assert str(Fraction(1, 2)) == "1/2"


def test_str_negative():
    assert str(Fraction(1, -2)) == "-1/2"


def test_str_zero():
    assert str(Fraction(0, 5)) == "0/1"


# --- __hash__ — хешируемость и согласованность с __eq__ ---------------------

def test_hash_equal_fractions_have_equal_hash():
    # 2/4 и 1/2 — одна дробь после нормализации; хеш обязан совпасть
    assert hash(Fraction(2, 4)) == hash(Fraction(1, 2))


def test_hash_negative_equivalent():
    assert hash(Fraction(1, -2)) == hash(Fraction(-1, 2))


def test_hash_fraction_usable_as_dict_key():
    d = {Fraction(1, 2): "half", Fraction(1, 3): "third"}
    assert d[Fraction(2, 4)] == "half"   # 2/4 нормализуется в 1/2 → тот же ключ


def test_hash_fraction_usable_in_set():
    s = {Fraction(1, 2), Fraction(2, 4), Fraction(3, 6)}
    assert len(s) == 1   # все три равны после нормализации


def test_hash_distinct_fractions_likely_differ():
    # Не гарантия (коллизии возможны), но для простых дробей — обычно разные
    assert hash(Fraction(1, 2)) != hash(Fraction(1, 3))


# --- Операторы __add__ и __mul__ --------------------------------------------

def test_operator_add_basic():
    assert Fraction(1, 2) + Fraction(1, 3) == Fraction(5, 6)


def test_operator_add_same_as_method():
    a, b = Fraction(2, 3), Fraction(1, 4)
    assert a + b == a.add(b)


def test_operator_add_with_non_fraction_returns_notimplemented():
    result = Fraction(1, 2).__add__(3)
    assert result is NotImplemented


def test_operator_mul_basic():
    assert Fraction(2, 3) * Fraction(3, 4) == Fraction(1, 2)


def test_operator_mul_same_as_method():
    a, b = Fraction(2, 3), Fraction(3, 5)
    assert a * b == a.mul(b)


def test_operator_mul_with_non_fraction_returns_notimplemented():
    result = Fraction(1, 2).__mul__("x")
    assert result is NotImplemented


def test_operator_add_commutativity():
    a, b = Fraction(1, 3), Fraction(2, 5)
    assert a + b == b + a


def test_operator_add_returns_new_object():
    a, b = Fraction(1, 4), Fraction(1, 4)
    result = a + b
    assert result is not a and result is not b


# --- @classmethod from_int ---------------------------------------------------

def test_from_int_creates_fraction_over_one():
    f = Fraction.from_int(3)
    assert (f.num, f.den) == (3, 1)


def test_from_int_zero():
    f = Fraction.from_int(0)
    assert (f.num, f.den) == (0, 1)


def test_from_int_negative():
    f = Fraction.from_int(-5)
    assert (f.num, f.den) == (-5, 1)


def test_from_int_equals_constructor():
    assert Fraction.from_int(7) == Fraction(7, 1)


def test_from_int_arithmetic():
    # 3 + 1/2 = 7/2 (через from_int)
    assert Fraction.from_int(3) + Fraction(1, 2) == Fraction(7, 2)


# --- Свойство value ----------------------------------------------------------

def test_value_is_float():
    v = Fraction(1, 2).value
    assert isinstance(v, float)
    assert v == 0.5


def test_value_negative():
    assert Fraction(-1, 2).value == -0.5


def test_value_integer_like():
    assert Fraction(6, 3).value == 2.0


def test_value_zero():
    assert Fraction(0, 5).value == 0.0


def test_value_accessed_without_parentheses():
    # value — это property: обращаемся как к атрибуту, а не как к методу.
    f = Fraction(3, 4)
    assert f.value == 0.75
