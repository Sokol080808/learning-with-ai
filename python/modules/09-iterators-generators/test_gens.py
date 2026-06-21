# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 09 — Итераторы и генераторы.
# Тесты детерминированы: без сети, без рандома. Бесконечные генераторы здесь — это не
# «зависание», а проверка ленивости: их можно потреблять по одному элементу.

import inspect
import itertools

import pytest

from gens import countdown, take, running_total, chunks, flatten


# --- countdown -------------------------------------------------------------

def test_countdown_basic():
    assert list(countdown(3)) == [3, 2, 1]


def test_countdown_one():
    assert list(countdown(1)) == [1]


def test_countdown_zero_is_empty():
    assert list(countdown(0)) == []


def test_countdown_negative_is_empty():
    assert list(countdown(-5)) == []


def test_countdown_is_a_generator():
    # Вызов должен возвращать генератор (ленивый), а не готовый список.
    assert inspect.isgenerator(countdown(3))


def test_countdown_is_lazy_one_step():
    # next() должен отдавать по одному элементу, начиная с n.
    g = countdown(4)
    assert next(g) == 4
    assert next(g) == 3


def test_countdown_exhausts_with_stopiteration():
    g = countdown(2)
    assert next(g) == 2
    assert next(g) == 1
    with pytest.raises(StopIteration):
        next(g)


# --- take ------------------------------------------------------------------

def test_take_from_list():
    assert take([1, 2, 3, 4, 5], 3) == [1, 2, 3]


def test_take_returns_list_type():
    assert isinstance(take([1, 2, 3], 2), list)


def test_take_more_than_available():
    assert take([1, 2], 5) == [1, 2]


def test_take_zero():
    assert take([1, 2, 3], 0) == []


def test_take_negative():
    assert take([1, 2, 3], -1) == []


def test_take_from_empty():
    assert take([], 3) == []


def test_take_from_generator():
    assert take(countdown(10), 4) == [10, 9, 8, 7]


def test_take_from_infinite_source():
    # Бесконечный счётчик: take обязан остановиться, иначе тест зависнет.
    assert take(itertools.count(0), 5) == [0, 1, 2, 3, 4]


def test_take_does_not_consume_more_than_needed():
    # После take(it, 2) в итераторе должны остаться следующие элементы.
    it = iter([10, 20, 30, 40])
    assert take(it, 2) == [10, 20]
    assert list(it) == [30, 40]


# --- running_total ---------------------------------------------------------

def test_running_total_basic():
    assert list(running_total([1, 2, 3, 4])) == [1, 3, 6, 10]


def test_running_total_single():
    assert list(running_total([7])) == [7]


def test_running_total_empty():
    assert list(running_total([])) == []


def test_running_total_with_negatives():
    # 5, 5+(-2)=3, 3+(-2)=1, 1+10=11
    assert list(running_total([5, -2, -2, 10])) == [5, 3, 1, 11]


def test_running_total_is_a_generator():
    assert inspect.isgenerator(running_total([1, 2, 3]))


def test_running_total_is_lazy_one_step():
    g = running_total([10, 20, 30])
    assert next(g) == 10
    assert next(g) == 30


# --- chunks ----------------------------------------------------------------

def test_chunks_uneven_last():
    assert list(chunks([1, 2, 3, 4, 5], 2)) == [[1, 2], [3, 4], [5]]


def test_chunks_exact_fit():
    assert list(chunks([1, 2, 3, 4], 2)) == [[1, 2], [3, 4]]


def test_chunks_size_one():
    assert list(chunks([1, 2, 3], 1)) == [[1], [2], [3]]


def test_chunks_size_bigger_than_list():
    assert list(chunks([1, 2, 3], 10)) == [[1, 2, 3]]


def test_chunks_empty_list():
    assert list(chunks([], 3)) == []


def test_chunks_is_a_generator():
    assert inspect.isgenerator(chunks([1, 2, 3], 2))


def test_chunks_yields_lists():
    first = next(chunks([1, 2, 3, 4], 2))
    assert first == [1, 2]
    assert isinstance(first, list)


def test_chunks_with_strings():
    assert list(chunks(["a", "b", "c"], 2)) == [["a", "b"], ["c"]]


# --- flatten ---------------------------------------------------------------

def test_flatten_flat_list():
    assert list(flatten([1, 2, 3])) == [1, 2, 3]


def test_flatten_one_level():
    assert list(flatten([1, [2, 3], 4])) == [1, 2, 3, 4]


def test_flatten_two_levels():
    assert list(flatten([1, [2, [3, 4]], 5])) == [1, 2, 3, 4, 5]


def test_flatten_three_levels():
    assert list(flatten([[[1]], [[2, [3]]]]) ) == [1, 2, 3]


def test_flatten_empty():
    assert list(flatten([])) == []


def test_flatten_nested_empty_lists():
    assert list(flatten([[], [[], []], []])) == []


def test_flatten_strings_are_atomic():
    # Строки — атомы: не разворачиваются побуквенно.
    assert list(flatten(["hello", [1, "world"]])) == ["hello", 1, "world"]


def test_flatten_tuple_input():
    assert list(flatten((1, (2, 3), 4))) == [1, 2, 3, 4]


def test_flatten_range_input():
    assert list(flatten([range(3), range(3, 5)])) == [0, 1, 2, 3, 4]


def test_flatten_mixed_containers():
    assert list(flatten([1, (2, 3), range(4, 6)])) == [1, 2, 3, 4, 5]


def test_flatten_is_a_generator():
    assert inspect.isgenerator(flatten([1, [2, 3]]))
