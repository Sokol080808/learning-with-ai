# Тесты для существенного задания модуля 14 — декораторы.
# Файл трогать не нужно: это эталон поведения.
# Сейчас тесты КРАСНЫЕ (NotImplementedError). Реализуй decorators.py — они позеленеют.

import pytest

from decorators import memoize, retry, timed


# ============================================================
# retry
# ============================================================

def test_retry_succeeds_on_first_attempt():
    """Функция, которая никогда не падает, должна вернуть результат с первой попытки."""
    @retry(3)
    def always_ok():
        return 99

    assert always_ok() == 99


def test_retry_retries_until_success():
    """Функция падает дважды, успевает на третьей попытке."""
    calls = []

    @retry(3)
    def flaky():
        calls.append(1)
        if len(calls) < 3:
            raise ValueError("not yet")
        return "done"

    result = flaky()
    assert result == "done"
    assert len(calls) == 3


def test_retry_raises_last_exception_when_exhausted():
    """Если все попытки исчерпаны — бросает исключение последней."""
    @retry(3)
    def always_fails():
        raise RuntimeError("boom")

    with pytest.raises(RuntimeError, match="boom"):
        always_fails()


def test_retry_times_1_behaves_like_no_decorator():
    """times=1: ровно одна попытка, никакого повтора."""
    calls = []

    @retry(1)
    def fails_once():
        calls.append(1)
        raise ValueError("nope")

    with pytest.raises(ValueError):
        fails_once()

    assert len(calls) == 1


def test_retry_passes_args_and_kwargs():
    """Аргументы и kwargs корректно пробрасываются при каждой попытке."""
    attempts = []

    @retry(3)
    def echo(x, *, multiplier=1):
        attempts.append(x)
        if len(attempts) < 2:
            raise Exception("retry me")
        return x * multiplier

    result = echo(5, multiplier=3)
    assert result == 15
    assert len(attempts) == 2


def test_retry_preserves_metadata():
    """functools.wraps: __name__ и __doc__ обёртки совпадают с оригиналом."""
    @retry(2)
    def my_func():
        """моя функция"""

    assert my_func.__name__ == "my_func"
    assert my_func.__doc__ == "моя функция"
    assert my_func.__wrapped__ is not None  # __wrapped__ обязан быть выставлен


def test_retry_exact_attempt_count():
    """Декоратор не вызывает функцию больше, чем times раз."""
    calls = []

    @retry(5)
    def always_fails():
        calls.append(1)
        raise ValueError

    with pytest.raises(ValueError):
        always_fails()

    assert len(calls) == 5


# ============================================================
# memoize
# ============================================================

def test_memoize_returns_correct_result():
    @memoize
    def add(a, b):
        return a + b

    assert add(2, 3) == 5


def test_memoize_caches_result():
    """Повторный вызов с теми же аргументами не вызывает функцию снова."""
    calls = []

    @memoize
    def compute(x):
        calls.append(x)
        return x * x

    assert compute(4) == 16
    assert compute(4) == 16
    assert len(calls) == 1


def test_memoize_different_args_call_function():
    """Разные аргументы — разные записи в кэше, функция вызывается для каждой."""
    calls = []

    @memoize
    def square(x):
        calls.append(x)
        return x ** 2

    square(2)
    square(3)
    square(2)  # из кэша, не новый вызов

    assert len(calls) == 2
    assert calls == [2, 3]


def test_memoize_cache_attribute_exists():
    """Обёртка обязана иметь атрибут .cache (dict)."""
    @memoize
    def f(x):
        return x

    f(10)
    assert hasattr(f, "cache")
    assert isinstance(f.cache, dict)


def test_memoize_cache_can_be_cleared():
    """После f.cache.clear() функция вызывается снова."""
    calls = []

    @memoize
    def f(x):
        calls.append(x)
        return x

    f(7)
    f.cache.clear()
    f(7)  # теперь снова вызывает функцию

    assert len(calls) == 2


def test_memoize_with_kwargs():
    """kwargs участвуют в ключе кэша."""
    calls = []

    @memoize
    def greet(name, *, loud=False):
        calls.append((name, loud))
        return name.upper() if loud else name

    greet("alice", loud=False)
    greet("alice", loud=True)
    greet("alice", loud=False)  # из кэша

    assert len(calls) == 2


def test_memoize_preserves_metadata():
    @memoize
    def important():
        """важная функция"""

    assert important.__name__ == "important"
    assert important.__doc__ == "важная функция"
    assert important.__wrapped__ is not None


def test_memoize_zero_arg_function():
    """Функция без аргументов кэшируется — вызывается ровно один раз."""
    calls = []

    @memoize
    def constant():
        calls.append(1)
        return 42

    assert constant() == 42
    assert constant() == 42
    assert len(calls) == 1


# ============================================================
# timed
# ============================================================

def test_timed_returns_correct_value():
    @timed
    def add(a, b):
        return a + b

    assert add(3, 4) == 7


def test_timed_last_elapsed_is_none_before_first_call():
    """До первого вызова last_elapsed == None."""
    @timed
    def f():
        return 1

    assert f.last_elapsed is None


def test_timed_last_elapsed_set_after_call():
    """После вызова last_elapsed — неотрицательный float."""
    @timed
    def f():
        return 1

    f()
    assert f.last_elapsed is not None
    assert isinstance(f.last_elapsed, float)
    assert f.last_elapsed >= 0.0


def test_timed_elapsed_updates_each_call():
    """last_elapsed обновляется при каждом новом вызове."""
    import time

    @timed
    def fast():
        pass

    @timed
    def slow():
        time.sleep(0.05)

    fast()
    elapsed_fast = fast.last_elapsed

    slow()
    elapsed_slow = slow.last_elapsed

    # медленная функция должна занять явно больше быстрой
    assert elapsed_slow > elapsed_fast


def test_timed_elapsed_recorded_even_on_exception():
    """При исключении last_elapsed всё равно выставляется, исключение пробрасывается."""
    @timed
    def raises():
        raise ValueError("oops")

    with pytest.raises(ValueError, match="oops"):
        raises()

    assert raises.last_elapsed is not None
    assert raises.last_elapsed >= 0.0


def test_timed_preserves_metadata():
    @timed
    def my_timed_func():
        """тайминг"""

    assert my_timed_func.__name__ == "my_timed_func"
    assert my_timed_func.__doc__ == "тайминг"
    assert my_timed_func.__wrapped__ is not None


def test_timed_multiple_calls_update_elapsed():
    """Вызываем дважды — last_elapsed после второго вызова актуален для второго."""
    import time

    @timed
    def f(sleep_time):
        time.sleep(sleep_time)

    f(0.0)
    first_elapsed = f.last_elapsed

    f(0.05)
    second_elapsed = f.last_elapsed

    # второй вызов был медленнее — last_elapsed должен отражать именно его
    assert second_elapsed > first_elapsed
