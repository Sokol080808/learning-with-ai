# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_mathx.py проверяет ФИКСИРОВАННЫЕ примеры (add(2, 3) == 5 и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# огромные числа, нули, отрицательные) и проверяет не конкретный ответ, а ИНВАРИАНТЫ —
# свойства, которые обязаны выполняться для ЛЮБОГО входа. Если твоя реализация верна на
# примерах, но ломается на краевом случае, эти тесты это поймают и покажут минимальный
# контрпример.

import math

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from mathx import add, mul, factorial, fib

# Числа в разумных пределах: целочисленная арифметика в Python точна на любых int,
# но факториал/фибоначчи растут гигантски, поэтому их аргументы держим маленькими.
ints = st.integers(min_value=-10**12, max_value=10**12)
small_nonneg = st.integers(min_value=0, max_value=200)
neg_ints = st.integers(min_value=-10**6, max_value=-1)


# --- add: сумма как коммутативная, ассоциативная операция с нейтралью 0 ---

@given(a=ints, b=ints)
def test_add_matches_builtin(a, b):
    # add обязан совпадать с настоящим сложением Python.
    assert add(a, b) == a + b


@given(a=ints, b=ints)
def test_add_is_commutative(a, b):
    assert add(a, b) == add(b, a)


@given(a=ints, b=ints, c=ints)
def test_add_is_associative(a, b, c):
    assert add(add(a, b), c) == add(a, add(b, c))


@given(a=ints)
def test_add_zero_is_identity(a):
    assert add(a, 0) == a
    assert add(0, a) == a


# --- mul: произведение коммутативно, поглощается нулём, нейтраль 1, дистрибутивно ---

@given(a=ints, b=ints)
def test_mul_matches_builtin(a, b):
    assert mul(a, b) == a * b


@given(a=ints, b=ints)
def test_mul_is_commutative(a, b):
    assert mul(a, b) == mul(b, a)


@given(a=ints)
def test_mul_zero_absorbs_and_one_is_identity(a):
    assert mul(a, 0) == 0
    assert mul(0, a) == 0
    assert mul(a, 1) == a
    assert mul(1, a) == a


@given(a=ints, b=ints, c=ints)
def test_mul_distributes_over_add(a, b, c):
    # a * (b + c) == a*b + a*c — связка add и mul.
    assert mul(a, add(b, c)) == add(mul(a, b), mul(a, c))


# --- factorial: рекуррентность n! == n * (n-1)!, монотонность, согласие с math ---

@given(n=small_nonneg)
@settings(max_examples=100, deadline=None)
def test_factorial_matches_math(n):
    # Должен совпадать с эталонной реализацией из стандартной библиотеки.
    assert factorial(n) == math.factorial(n)


@given(n=st.integers(min_value=1, max_value=200))
@settings(max_examples=100, deadline=None)
def test_factorial_recurrence(n):
    # Определяющее свойство факториала: n! == n * (n-1)!.
    assert factorial(n) == n * factorial(n - 1)


@given(n=st.integers(min_value=1, max_value=200))
@settings(max_examples=100, deadline=None)
def test_factorial_is_strictly_increasing_from_one(n):
    # Начиная с n>=1 факториал строго растёт: n! > (n-1)! (т.к. множитель n>=2 при n>=2,
    # а при n==1: 1! == 1 == 0!), поэтому используем нестрогое для n==1 и строгое дальше.
    if n >= 2:
        assert factorial(n) > factorial(n - 1)
    else:
        assert factorial(n) >= factorial(n - 1)


def test_factorial_base_cases_exact():
    assert factorial(0) == 1
    assert factorial(1) == 1


@given(n=neg_ints)
def test_factorial_negative_raises(n):
    with pytest.raises(ValueError):
        factorial(n)


# --- fib: рекуррентность fib(n) == fib(n-1) + fib(n-2), тождество Кассини ---

@given(n=st.integers(min_value=2, max_value=500))
@settings(max_examples=100, deadline=None)
def test_fib_recurrence(n):
    # Определяющее свойство Фибоначчи.
    assert fib(n) == fib(n - 1) + fib(n - 2)


@given(n=st.integers(min_value=1, max_value=500))
@settings(max_examples=100, deadline=None)
def test_fib_cassini_identity(n):
    # Тождество Кассини: fib(n-1)*fib(n+1) - fib(n)^2 == (-1)^n.
    left = fib(n - 1) * fib(n + 1) - fib(n) ** 2
    assert left == (-1) ** n


@given(n=st.integers(min_value=0, max_value=500))
@settings(max_examples=100, deadline=None)
def test_fib_is_nondecreasing(n):
    # Последовательность не убывает (fib(1)==fib(2)==1, дальше строго растёт).
    assert fib(n + 1) >= fib(n)


def test_fib_base_cases_exact():
    assert fib(0) == 0
    assert fib(1) == 1
    assert fib(2) == 1


@given(n=neg_ints)
def test_fib_negative_raises(n):
    with pytest.raises(ValueError):
        fib(n)
