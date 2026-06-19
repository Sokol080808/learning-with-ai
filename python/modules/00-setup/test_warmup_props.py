# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_warmup.py проверяет ФИКСИРОВАННЫЕ примеры (add(2, 3) == 5 и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# огромные числа, нули, отрицательные) и проверяет не конкретный ответ, а ИНВАРИАНТЫ —
# свойства, которые обязаны выполняться для ЛЮБОГО входа. Если твоя реализация верна на
# примерах, но ломается на краевом случае, эти тесты это поймают и покажут минимальный
# контрпример.

from hypothesis import given
from hypothesis import strategies as st

from warmup import add, seconds_in

ints = st.integers(min_value=-10**12, max_value=10**12)
nonneg_hours = st.integers(min_value=0, max_value=10**9)


# --- add: сложение целых — это алгебраические инварианты ---------------------

@given(a=ints, b=ints)
def test_add_matches_builtin(a, b):
    # Сильная проверка корректности: совпадает со встроенным сложением.
    assert add(a, b) == a + b


@given(a=ints, b=ints)
def test_add_commutative(a, b):
    assert add(a, b) == add(b, a)


@given(a=ints, b=ints, c=ints)
def test_add_associative(a, b, c):
    assert add(add(a, b), c) == add(a, add(b, c))


@given(a=ints)
def test_add_identity_zero(a):
    # Ноль — нейтральный элемент.
    assert add(a, 0) == a
    assert add(0, a) == a


@given(a=ints)
def test_add_inverse(a):
    # a + (-a) == 0 для любого a.
    assert add(a, -a) == 0


@given(a=ints, b=ints)
def test_add_returns_int(a, b):
    assert isinstance(add(a, b), int)


def test_add_handles_huge_numbers():
    big = 10 ** 100
    assert add(big, big) == 2 * big
    assert add(big, -big) == 0


# --- seconds_in: линейная функция часов -> секунд ---------------------------

@given(hours=nonneg_hours)
def test_seconds_in_matches_formula(hours):
    assert seconds_in(hours) == hours * 3600


@given(hours=nonneg_hours)
def test_seconds_in_is_multiple_of_3600(hours):
    # Что бы ни вернула функция — это всегда кратно 3600.
    assert seconds_in(hours) % 3600 == 0


@given(hours=nonneg_hours)
def test_seconds_in_monotonic(hours):
    # Каждый следующий час добавляет ровно 3600 секунд.
    assert seconds_in(hours + 1) - seconds_in(hours) == 3600


def test_seconds_in_zero():
    assert seconds_in(0) == 0


def test_seconds_in_handles_huge():
    assert seconds_in(10 ** 18) == 10 ** 18 * 3600
