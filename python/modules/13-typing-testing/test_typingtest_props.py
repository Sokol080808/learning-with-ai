# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_typingtest.py проверяет ФИКСИРОВАННЫЕ примеры (clamp(3,0,5) == 3 и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# огромные числа, нули, отрицательные, пустые словари, юникод-ключи) и проверяет не конкретный
# ответ, а ИНВАРИАНТЫ — свойства, которые обязаны выполняться для ЛЮБОГО входа. Если твоя
# реализация верна на примерах, но ломается на краевом случае, эти тесты это поймают и покажут
# минимальный контрпример.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from typingtest import clamp, merge_counts, safe_get

# Числа в разумных, но широких пределах — включают огромные, нулевые и отрицательные.
ints = st.integers(min_value=-10**12, max_value=10**12)
keys = st.text(min_size=0, max_size=8)
count_dicts = st.dictionaries(keys=keys, values=ints, max_size=8)


# --- clamp: результат всегда внутри [lo, hi] и совпадает с x, когда x уже внутри ---

@given(x=ints, lo=ints, hi=ints)
def test_clamp_result_within_bounds(x, lo, hi):
    if lo > hi:
        # неверный диапазон обязан приводить к ValueError
        with pytest.raises(ValueError):
            clamp(x, lo, hi)
    else:
        result = clamp(x, lo, hi)
        # что бы ни подали, результат не вылезает за границы
        assert lo <= result <= hi


@given(x=ints, lo=ints, hi=ints)
def test_clamp_identity_when_inside(x, lo, hi):
    # если x уже внутри диапазона — возвращается ровно он, без искажений
    if lo <= x <= hi:
        assert clamp(x, lo, hi) == x


@given(x=ints, lo=ints, hi=ints)
def test_clamp_matches_min_max_formula(x, lo, hi):
    # каноничная формула «зажима»: max(lo, min(x, hi)) для корректного диапазона
    if lo <= hi:
        assert clamp(x, lo, hi) == max(lo, min(x, hi))


@given(x=ints, lo=ints, hi=ints)
def test_clamp_idempotent(x, lo, hi):
    # зажать уже зажатое — ничего не меняет (результат стабилен)
    if lo <= hi:
        once = clamp(x, lo, hi)
        assert clamp(once, lo, hi) == once


@given(x=ints, point=ints)
def test_clamp_degenerate_range_collapses(x, point):
    # диапазон из одной точки [point, point] схлопывает любой x в эту точку
    assert clamp(x, point, point) == point


@given(x=ints, lo=ints, hi=ints)
def test_clamp_bad_range_raises(x, lo, hi):
    # любой строго неверный диапазон (lo > hi) обязан бросать ValueError
    if lo > hi:
        with pytest.raises(ValueError):
            clamp(x, lo, hi)


def test_clamp_extreme_values():
    big = 10**18
    assert clamp(big, -5, 5) == 5
    assert clamp(-big, -5, 5) == -5
    assert clamp(0, -big, big) == 0


# --- safe_get: совпадает с d.get, не бросает, не мутирует словарь ---

@given(d=count_dicts, key=keys)
def test_safe_get_matches_dict_get(d, key):
    # safe_get обязан вести себя как штатный d.get(key) (None при отсутствии)
    assert safe_get(d, key) == d.get(key)


@given(d=count_dicts, key=keys)
def test_safe_get_present_returns_value_absent_returns_none(d, key):
    if key in d:
        result = safe_get(d, key)
        assert result == d[key]
        # даже значение 0 — это валидный ответ, а не «нет ключа»
        assert result is not None or d[key] is None
    else:
        assert safe_get(d, key) is None


@given(d=count_dicts, key=keys)
def test_safe_get_does_not_mutate(d, key):
    # чтение отсутствующего ключа не должно добавлять его в словарь
    before = dict(d)
    safe_get(d, key)
    assert d == before


def test_safe_get_zero_value_not_confused_with_missing():
    d = {"hits": 0}
    assert safe_get(d, "hits") == 0
    assert safe_get(d, "hits") is not None
    assert safe_get(d, "absent") is None


# --- merge_counts: сумма по ключам, объединение множеств ключей, без мутаций ---

@given(a=count_dicts, b=count_dicts)
def test_merge_counts_keys_are_union(a, b):
    result = merge_counts(a, b)
    assert set(result.keys()) == set(a.keys()) | set(b.keys())


@given(a=count_dicts, b=count_dicts)
def test_merge_counts_values_are_sums(a, b):
    result = merge_counts(a, b)
    for key in set(a) | set(b):
        assert result[key] == a.get(key, 0) + b.get(key, 0)


@given(a=count_dicts, b=count_dicts)
def test_merge_counts_does_not_mutate_inputs(a, b):
    a_before, b_before = dict(a), dict(b)
    merge_counts(a, b)
    assert a == a_before
    assert b == b_before


@given(a=count_dicts, b=count_dicts)
def test_merge_counts_total_sum_is_preserved(a, b):
    # сумма всех значений результата равна сумме всех значений обоих входов
    result = merge_counts(a, b)
    assert sum(result.values()) == sum(a.values()) + sum(b.values())


@given(a=count_dicts, b=count_dicts)
def test_merge_counts_result_is_new_object(a, b):
    result = merge_counts(a, b)
    assert result is not a
    assert result is not b


@given(a=count_dicts)
def test_merge_counts_with_empty_is_identity(a):
    # слияние с пустым словарём возвращает копию исходного (по значению)
    assert merge_counts(a, {}) == a
    assert merge_counts({}, a) == a


@settings(max_examples=150)
@given(a=count_dicts, b=count_dicts)
def test_merge_counts_is_commutative(a, b):
    # сложение счётчиков коммутативно: a+b даёт те же пары, что b+a
    assert merge_counts(a, b) == merge_counts(b, a)
