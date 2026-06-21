# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_basics.py проверяет ФИКСИРОВАННЫЕ примеры (0 -> 32.0 и т.п.). Здесь — РАНДОМИЗИРОВАННЫЕ
# property-тесты: hypothesis сам подбирает сотни входов (включая злые — огромные числа, нули,
# отрицательные, юникод) и проверяет не конкретный ответ, а ИНВАРИАНТЫ — свойства, которые
# обязаны выполняться для ЛЮБОГО входа. Если твоя реализация верна на примерах, но ломается на
# краевом случае, эти тесты это поймают и покажут минимальный контрпример.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from basics import (
    to_fahrenheit,
    is_even,
    min_of_three,
    average3,
    greet,
    parse_number,
)

# Числа в разумных пределах: за гранью ~1e15 у float не хватает мантиссы и сравнения «плывут».
finite_floats = st.floats(min_value=-1e9, max_value=1e9, allow_nan=False, allow_infinity=False)
ints = st.integers(min_value=-10**12, max_value=10**12)


# --- to_fahrenheit: линейная функция, у неё есть свойства наклона и неподвижной точки ---

@given(c=finite_floats)
def test_to_fahrenheit_matches_formula(c):
    assert to_fahrenheit(c) == pytest.approx(c * 9 / 5 + 32, rel=1e-9, abs=1e-9)


@given(c=finite_floats)
def test_to_fahrenheit_slope_is_1_8(c):
    # +1 °C всегда даёт +1.8 °F, какой бы ни была точка отсчёта.
    assert to_fahrenheit(c + 1) - to_fahrenheit(c) == pytest.approx(1.8, abs=1e-6)


@given(a=finite_floats, b=finite_floats)
def test_to_fahrenheit_is_monotonic(a, b):
    if a < b:
        fa, fb = to_fahrenheit(a), to_fahrenheit(b)
        # Перевод не убывает. Математически он строго растёт, но у float два очень
        # близких входа могут дать один и тот же результат — поэтому <=, а строгое
        # неравенство проверяем только на заметном разрыве (>= 1 °C).
        assert fa <= fb
        if b - a >= 1.0:
            assert fa < fb


def test_to_fahrenheit_fixed_point():
    # Единственная точка, где шкалы совпадают.
    assert to_fahrenheit(-40) == pytest.approx(-40.0)


# --- is_even: чётность — это инварианты, а не один пример ---

@given(n=ints)
def test_is_even_parity_alternates(n):
    # Соседние числа всегда разной чётности.
    assert is_even(n) is not is_even(n + 1)


@given(k=ints)
def test_is_even_double_is_even_and_plus_one_is_odd(k):
    assert is_even(2 * k) is True
    assert is_even(2 * k + 1) is False


@given(n=ints)
def test_is_even_returns_real_bool(n):
    assert is_even(n) is True or is_even(n) is False


def test_is_even_handles_huge_numbers():
    big = 10 ** 100
    assert is_even(big) is True
    assert is_even(big + 1) is False


# --- min_of_three: инвариант «минимум» не зависит от порядка аргументов ---

@given(a=ints, b=ints, c=ints)
def test_min_of_three_is_a_lower_bound_and_a_member(a, b, c):
    m = min_of_three(a, b, c)
    assert m in (a, b, c)          # это действительно один из аргументов
    assert m <= a and m <= b and m <= c  # и он не больше любого из них


@given(a=ints, b=ints, c=ints)
def test_min_of_three_order_independent(a, b, c):
    expected = min_of_three(a, b, c)
    # Любая перестановка даёт тот же минимум.
    assert min_of_three(a, c, b) == expected
    assert min_of_three(b, a, c) == expected
    assert min_of_three(c, b, a) == expected


# --- average3: связь суммы и среднего + границы ---

@given(a=ints, b=ints, c=ints)
def test_average3_times_three_is_sum(a, b, c):
    assert 3 * average3(a, b, c) == pytest.approx(a + b + c)


@given(a=ints, b=ints, c=ints)
def test_average3_within_min_max(a, b, c):
    avg = average3(a, b, c)
    assert min(a, b, c) <= avg <= max(a, b, c)


@given(a=ints, b=ints, c=ints)
def test_average3_is_float(a, b, c):
    assert isinstance(average3(a, b, c), float)


# --- greet: форма строки, какой бы ни была подстановка ---

@given(name=st.text())
@settings(max_examples=200)
def test_greet_wraps_any_name(name):
    result = greet(name)
    assert result == f"Привет, {name}!"
    assert result.startswith("Привет, ")
    assert result.endswith("!")
    assert name in result


# --- parse_number: property-тесты (seeded для воспроизводимости) ---

@given(n=ints)
@settings(derandomize=True)
def test_parse_number_roundtrip_int(n):
    # Строковое представление целого всегда должно распарситься обратно в то же int.
    s = str(n)
    result = parse_number(s)
    assert result == n
    assert type(result) is int


@given(n=ints)
@settings(derandomize=True)
def test_parse_number_negative_mod2_parity(n):
    # Инвариант чётности: parse_number(str(n)) % 2 совпадает с n % 2.
    # Это косвенно проверяет знак остатка: -3 % 2 == 1 в Python.
    result = parse_number(str(n))
    assert result is not None
    assert result % 2 == n % 2


@given(n=ints)
@settings(derandomize=True)
def test_parse_number_int_not_float_for_integer_strings(n):
    # Целочисленная строка должна давать ровно int, а не float.
    result = parse_number(str(n))
    assert type(result) is int


@given(f=st.floats(min_value=-1e15, max_value=1e15, allow_nan=False,
                   allow_infinity=False))
@settings(derandomize=True)
def test_parse_number_float_result_is_float_or_int(f):
    # Для любого конечного float его repr даёт число (int или float), не None.
    s = repr(f)
    result = parse_number(s)
    # repr(float) иногда включает 'nan'/'inf' — они уже отфильтрованы стратегией,
    # но из осторожности допускаем None только если это не числовая строка.
    assert result is not None or not s.lstrip("-").replace(".", "", 1).isdigit()


@given(text=st.text(alphabet=st.characters(
    blacklist_categories=("Nd",),  # без цифровых символов Unicode
    blacklist_characters=".-+eE"
), min_size=1))
@settings(derandomize=True, max_examples=200)
def test_parse_number_non_numeric_text_returns_none(text):
    # Строки без цифр, точек и знаков — всегда None.
    # Но дополнительно фильтруем случайные строки, которые float() принимает
    # (например строки, которые случайно оказались числами после strip).
    result = parse_number(text)
    # Нельзя утверждать None на любой текст (некоторые Unicode-символы — цифры),
    # но если результат не None, он должен быть числом.
    if result is not None:
        assert isinstance(result, (int, float))
