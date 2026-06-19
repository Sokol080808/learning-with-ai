# Property-тесты для декораторов модуля 14 (hypothesis, derandomize через conftest.py курса).
# Файл трогать не нужно.  Сейчас — красные.  Реализуй decorators.py — станут зелёными.

from hypothesis import given, settings
from hypothesis import strategies as st

from decorators import memoize, retry, timed


# ============================================================
# retry — инварианты
# ============================================================

@given(
    good_at=st.integers(min_value=1, max_value=5),
    times=st.integers(min_value=1, max_value=10),
)
def test_retry_prop_succeeds_when_enough_attempts(good_at, times):
    """Если функция становится успешной на попытке good_at и times >= good_at,
    retry(times) ОБЯЗАН вернуть результат, а не бросить исключение."""
    if times < good_at:
        return  # этот случай рассматривается в другом тесте

    calls = []

    @retry(times)
    def f():
        calls.append(1)
        if len(calls) < good_at:
            raise ValueError("not yet")
        return "success"

    result = f()
    assert result == "success"
    assert len(calls) == good_at


@given(times=st.integers(min_value=1, max_value=8))
def test_retry_prop_exhausts_exactly_times(times):
    """Если функция ВСЕГДА падает — вызывается ровно times раз, не больше и не меньше."""
    calls = []

    @retry(times)
    def always_bad():
        calls.append(1)
        raise RuntimeError("always")

    try:
        always_bad()
    except RuntimeError:
        pass

    assert len(calls) == times


@given(
    value=st.integers(),
    times=st.integers(min_value=1, max_value=5),
)
def test_retry_prop_transparent_return_value(value, times):
    """retry не изменяет возвращаемое значение функции."""
    @retry(times)
    def f():
        return value

    assert f() == value


@given(times=st.integers(min_value=1, max_value=5))
def test_retry_prop_preserves_name(times):
    """functools.wraps: __name__ не теряется независимо от times."""
    @retry(times)
    def sentinel_function():
        pass

    assert sentinel_function.__name__ == "sentinel_function"


# ============================================================
# memoize — инварианты
# ============================================================

@given(xs=st.lists(st.integers(min_value=-100, max_value=100), min_size=0, max_size=20))
def test_memoize_prop_call_count_equals_unique_inputs(xs):
    """Реальных вызовов функции == число уникальных значений в xs."""
    calls = []

    @memoize
    def identity(x):
        calls.append(x)
        return x

    for x in xs:
        identity(x)

    assert len(calls) == len(set(xs))


@given(
    a=st.integers(min_value=-50, max_value=50),
    b=st.integers(min_value=-50, max_value=50),
    repeats=st.integers(min_value=1, max_value=10),
)
def test_memoize_prop_same_result_every_time(a, b, repeats):
    """Кэшированный результат всегда совпадает с первым вызовом."""
    @memoize
    def add(x, y):
        return x + y

    expected = add(a, b)
    for _ in range(repeats - 1):
        assert add(a, b) == expected


@given(xs=st.lists(st.integers(), min_size=0, max_size=15))
def test_memoize_prop_cache_size_equals_unique_args(xs):
    """Размер cache == число уникальных аргументов после всех вызовов."""
    @memoize
    def f(x):
        return x * 2

    for x in xs:
        f(x)

    assert len(f.cache) == len(set(xs))


@given(value=st.integers())
def test_memoize_prop_preserves_return_value(value):
    """memoize не изменяет возвращаемое значение."""
    @memoize
    def f(x):
        return x + 1

    assert f(value) == value + 1
    assert f(value) == value + 1  # повторный вызов из кэша — тот же результат


# ============================================================
# timed — инварианты
# ============================================================

@given(value=st.integers())
def test_timed_prop_transparent_return_value(value):
    """timed не меняет возвращаемое значение."""
    @timed
    def f(x):
        return x

    assert f(value) == value


@given(n=st.integers(min_value=1, max_value=20))
def test_timed_prop_elapsed_nonnegative_after_n_calls(n):
    """last_elapsed >= 0.0 после каждого из n вызовов."""
    @timed
    def f():
        pass

    for _ in range(n):
        f()
        assert f.last_elapsed is not None
        assert f.last_elapsed >= 0.0


@given(
    values=st.lists(st.integers(min_value=-1000, max_value=1000), min_size=1, max_size=10)
)
def test_timed_prop_result_matches_unwrapped(values):
    """Результат timed-обёртки совпадает с прямым вызовом оригинала на любом входе."""
    def original(x):
        return x * x - 1

    wrapped = timed(original)

    for v in values:
        assert wrapped(v) == original(v)


@given(n=st.integers(min_value=1, max_value=5))
def test_timed_prop_name_preserved(n):
    """__name__ сохраняется независимо от числа вызовов."""
    @timed
    def my_named_function():
        pass

    assert my_named_function.__name__ == "my_named_function"
    for _ in range(n):
        my_named_function()
    assert my_named_function.__name__ == "my_named_function"
