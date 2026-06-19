# Property-тесты для mathx.statistics.
#
# hypothesis подбирает сотни входов автоматически. conftest.py курса включает
# derandomize=True — одни и те же примеры на каждом запуске. deadline=None —
# без падений по таймауту. Тесты проверяют ИНВАРИАНТЫ, а не конкретные значения.

import math

import pytest
from hypothesis import assume, given, settings
from hypothesis import strategies as st

from mathx import mean, median, mode, variance

# Стратегии
reasonable_floats = st.floats(
    min_value=-1e9,
    max_value=1e9,
    allow_nan=False,
    allow_infinity=False,
)
nonempty_list = st.lists(reasonable_floats, min_size=1, max_size=100)
nonempty_ints_list = st.lists(
    st.integers(min_value=-10_000, max_value=10_000), min_size=1, max_size=100
)


# ── mean ──────────────────────────────────────────────────────────────────────

@given(data=nonempty_list)
def test_mean_matches_builtin_formula(data):
    """mean совпадает с sum/len."""
    assert mean(data) == pytest.approx(sum(data) / len(data), rel=1e-9)


@given(data=nonempty_list)
def test_mean_in_range(data):
    """Среднее всегда лежит в диапазоне [min, max]."""
    m = mean(data)
    assert min(data) - 1e-9 <= m <= max(data) + 1e-9


@given(data=nonempty_list, c=reasonable_floats)
def test_mean_shift(data, c):
    """mean(x + c for all x) == mean(data) + c."""
    shifted = [x + c for x in data]
    assert mean(shifted) == pytest.approx(mean(data) + c, rel=1e-9, abs=1e-9)


@given(data=nonempty_list, c=reasonable_floats)
def test_mean_scale(data, c):
    """mean(x * c for all x) == mean(data) * c."""
    assume(abs(c) < 1e6)  # избегаем числового переполнения в pytest.approx
    scaled = [x * c for x in data]
    assert mean(scaled) == pytest.approx(mean(data) * c, rel=1e-7, abs=1e-7)


@given(data=st.lists(reasonable_floats, max_size=0))
def test_mean_empty_raises(data):
    """Пустая последовательность → ValueError."""
    with pytest.raises(ValueError):
        mean(data)


# ── median ────────────────────────────────────────────────────────────────────

@given(data=nonempty_list)
def test_median_in_range(data):
    """Медиана лежит в [min(data), max(data)]."""
    m = median(data)
    assert min(data) - 1e-9 <= m <= max(data) + 1e-9


@given(data=nonempty_list)
def test_median_does_not_mutate(data):
    """median не изменяет исходный список."""
    original = list(data)
    median(data)
    assert data == original


@given(data=nonempty_list)
def test_median_of_sorted_equals_median(data):
    """Медиана не зависит от порядка элементов: median(data) == median(sorted(data))."""
    assert median(data) == pytest.approx(median(sorted(data)), rel=1e-9)


@given(data=nonempty_list)
def test_median_of_reversed_equals_median(data):
    """median(data) == median(reversed(data))."""
    assert median(data) == pytest.approx(median(list(reversed(data))), rel=1e-9)


@given(data=st.lists(reasonable_floats, min_size=1, max_size=100))
def test_median_split_invariant(data):
    """В отсортированном ряду не более половины элементов строго меньше медианы
    и не более половины строго больше."""
    m = median(data)
    below = sum(1 for x in data if x < m - 1e-12)
    above = sum(1 for x in data if x > m + 1e-12)
    n = len(data)
    assert below <= n // 2
    assert above <= n // 2


@given(data=st.lists(reasonable_floats, max_size=0))
def test_median_empty_raises(data):
    with pytest.raises(ValueError):
        median(data)


# ── mode ──────────────────────────────────────────────────────────────────────

@given(data=nonempty_ints_list)
def test_mode_is_in_data(data):
    """Мода — один из элементов выборки."""
    assert mode(data) in data


@given(data=nonempty_ints_list)
def test_mode_frequency_is_maximal(data):
    """Частота моды >= частоты любого другого элемента."""
    m = mode(data)
    freq_m = data.count(m)
    for x in set(data):
        assert freq_m >= data.count(x)


@given(data=nonempty_ints_list)
def test_mode_tie_is_minimum(data):
    """При ничьей mode возвращает наименьший из самых частых."""
    m = mode(data)
    max_freq = max(data.count(x) for x in set(data))
    candidates = [x for x in set(data) if data.count(x) == max_freq]
    assert m == min(candidates)


@given(data=nonempty_ints_list)
def test_mode_invariant_under_order(data):
    """Мода не зависит от порядка элементов."""
    import random
    shuffled = list(data)
    # стабильное перемешивание: reverse — детерминировано
    shuffled_rev = list(reversed(shuffled))
    assert mode(shuffled) == mode(shuffled_rev)


@given(data=st.lists(st.integers(), max_size=0))
def test_mode_empty_raises(data):
    with pytest.raises(ValueError):
        mode(data)


# ── variance ──────────────────────────────────────────────────────────────────

@given(data=nonempty_list)
def test_variance_nonnegative(data):
    """Дисперсия всегда >= 0."""
    assert variance(data) >= -1e-9


@given(data=st.lists(reasonable_floats, min_size=2, max_size=100))
def test_variance_ddof1_nonnegative(data):
    assert variance(data, ddof=1) >= -1e-9


@given(c=reasonable_floats, n=st.integers(min_value=1, max_value=50))
def test_variance_constant_sequence_is_zero(c, n):
    """Все элементы одинаковы → дисперсия = 0."""
    data = [c] * n
    assert variance(data) == pytest.approx(0.0, abs=1e-9)


@given(data=nonempty_list, c=reasonable_floats)
def test_variance_shift_invariant(data, c):
    """Сдвиг на константу не меняет дисперсию: Var(x+c) == Var(x)."""
    assume(abs(c) < 1e6)
    shifted = [x + c for x in data]
    assert variance(shifted) == pytest.approx(variance(data), rel=1e-7, abs=1e-9)


@given(data=nonempty_list, c=reasonable_floats)
def test_variance_scale_squared(data, c):
    """Масштабирование: Var(c*x) == c^2 * Var(x)."""
    assume(abs(c) < 1e4)
    scaled = [x * c for x in data]
    assert variance(scaled) == pytest.approx(c ** 2 * variance(data), rel=1e-6, abs=1e-9)


@given(data=nonempty_list)
def test_variance_ddof0_le_ddof1(data):
    """Генеральная дисперсия <= выборочной (при N >= 2)."""
    if len(data) >= 2:
        assert variance(data, ddof=0) <= variance(data, ddof=1) + 1e-9


@given(data=st.lists(reasonable_floats, max_size=0))
def test_variance_empty_raises(data):
    with pytest.raises(ValueError):
        variance(data)


@given(data=st.lists(reasonable_floats, min_size=1, max_size=1))
def test_variance_single_ddof1_raises(data):
    with pytest.raises(ValueError):
        variance(data, ddof=1)


@given(data=st.lists(reasonable_floats, min_size=1))
@settings(max_examples=20)
def test_variance_bad_ddof_raises(data):
    with pytest.raises(ValueError):
        variance(data, ddof=2)
