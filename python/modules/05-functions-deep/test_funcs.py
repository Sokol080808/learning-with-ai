# Эти тесты трогать не нужно — это эталон поведения.
#
# Они описывают, КАК должен вести себя твой код в funcs.py. Сейчас они красные
# (стаб кидает NotImplementedError). Реализуй функции — станут зелёными.
#
# Запуск: ./python/run.sh 05

import functools

from funcs import (
    apply_twice,
    make_multiplier,
    compose,
    sort_by_length,
    memoize,
)


# ---------- apply_twice ----------

def test_apply_twice_increment():
    # 1 -> 4 -> 7
    assert apply_twice(lambda n: n + 3, 1) == 7


def test_apply_twice_double():
    # 5 -> 10 -> 20
    assert apply_twice(lambda n: n * 2, 5) == 20


def test_apply_twice_on_strings():
    # "ab" -> "ab!" -> "ab!!"
    assert apply_twice(lambda s: s + "!", "ab") == "ab!!"


def test_apply_twice_uses_given_function():
    # Если f — тождество, f(f(x)) == x.
    assert apply_twice(lambda x: x, 42) == 42


# ---------- make_multiplier ----------

def test_make_multiplier_returns_callable():
    triple = make_multiplier(3)
    assert callable(triple)


def test_make_multiplier_basic():
    triple = make_multiplier(3)
    assert triple(4) == 12
    assert triple(0) == 0
    assert triple(-2) == -6


def test_make_multiplier_independent_closures():
    # Каждый вызов make_multiplier создаёт своё окружение — они не мешают друг другу.
    triple = make_multiplier(3)
    ten = make_multiplier(10)
    assert triple(5) == 15
    assert ten(5) == 50
    # Повторно убеждаемся, что triple не «испортился» после создания ten.
    assert triple(2) == 6


def test_make_multiplier_inline_call():
    assert make_multiplier(7)(6) == 42


# ---------- compose ----------

def test_compose_order_matters():
    inc = lambda n: n + 1
    dbl = lambda n: n * 2
    # compose(inc, dbl)(5): сначала dbl(5)=10, потом inc(10)=11
    assert compose(inc, dbl)(5) == 11
    # обратный порядок даёт другой результат: inc(5)=6, dbl(6)=12
    assert compose(dbl, inc)(5) == 12


def test_compose_returns_callable():
    h = compose(lambda x: x, lambda x: x)
    assert callable(h)


def test_compose_mixed_types():
    # g превращает число в строку, f дописывает '!'
    f = lambda s: s + "!"
    g = lambda n: str(n)
    assert compose(f, g)(7) == "7!"


def test_compose_with_len():
    # g=len, f=удвоить: len("abcd")=4 -> 8
    assert compose(lambda n: n * 2, len)("abcd") == 8


# ---------- sort_by_length ----------

def test_sort_by_length_basic():
    assert sort_by_length(["bbb", "a", "cc"]) == ["a", "cc", "bbb"]


def test_sort_by_length_already_sorted():
    assert sort_by_length(["a", "bb", "ccc"]) == ["a", "bb", "ccc"]


def test_sort_by_length_stable_on_ties():
    # При равной длине сохраняется исходный порядок (устойчивая сортировка).
    assert sort_by_length(["bb", "aa", "c", "dd"]) == ["c", "bb", "aa", "dd"]


def test_sort_by_length_does_not_mutate_input():
    original = ["xxx", "y", "zz"]
    snapshot = list(original)
    result = sort_by_length(original)
    # исходный список не тронут
    assert original == snapshot
    # а результат — отсортирован и это другой объект
    assert result == ["y", "zz", "xxx"]
    assert result is not original


def test_sort_by_length_empty():
    assert sort_by_length([]) == []


# ---------- memoize ----------

def test_memoize_returns_same_results():
    @memoize
    def square(n):
        return n * n

    assert square(2) == 4
    assert square(3) == 9
    assert square(2) == 4  # из кэша, но значение то же


def test_memoize_caches_real_calls():
    calls = []

    @memoize
    def slow_add(a, b):
        calls.append((a, b))  # фиксируем КАЖДЫЙ реальный вызов тела
        return a + b

    assert slow_add(1, 2) == 3      # реальный вызов №1
    assert slow_add(1, 2) == 3      # из кэша — тело НЕ выполняется
    assert slow_add(1, 2) == 3      # снова из кэша
    assert slow_add(2, 3) == 5      # новые аргументы — реальный вызов №2
    assert slow_add(2, 3) == 5      # из кэша

    # Тело выполнилось ровно дважды, по одному разу на уникальный набор аргументов.
    assert calls == [(1, 2), (2, 3)]


def test_memoize_counts_single_arg():
    count = 0

    @memoize
    def f(n):
        nonlocal count
        count += 1
        return n + 100

    f(5)
    f(5)
    f(5)
    f(6)
    assert count == 2  # уникальных аргументов было два: 5 и 6


def test_memoize_preserves_name():
    @memoize
    def my_func(x):
        return x

    # functools.wraps должен сохранить имя оригинальной функции.
    assert my_func.__name__ == "my_func"
