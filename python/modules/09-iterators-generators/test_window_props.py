# test_window_props.py — property-тесты (hypothesis) для класса Window (модуль 09).
#
# Проверяют ИНВАРИАНТЫ, а не конкретные ответы. Hypothesis сам подбирает сотни
# входов: пустые, одиночные, большие списки, size от 1 до 20.
# derandomize=True (см. conftest.py) делает набор воспроизводимым.

import itertools

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from gens import Window

ints = st.integers(min_value=-(10**6), max_value=10**6)
int_lists = st.lists(ints, max_size=50)
valid_size = st.integers(min_value=1, max_value=20)


# --- каждое окно — список правильной длины -----------------------------------

@given(xs=int_lists, size=valid_size)
def test_window_prop_each_window_has_correct_length(xs, size):
    for w in Window(xs, size):
        assert isinstance(w, list)
        assert len(w) == size


# --- количество окон == max(0, len(xs) - size + 1) ---------------------------

@given(xs=int_lists, size=valid_size)
def test_window_prop_count(xs, size):
    windows = list(Window(xs, size))
    expected = max(0, len(xs) - size + 1)
    assert len(windows) == expected


# --- скользящий сдвиг: хвост[i] == голова[i+1] (перекрытие на size-1) -------

@given(xs=int_lists, size=valid_size)
def test_window_prop_consecutive_overlap(xs, size):
    windows = list(Window(xs, size))
    for i in range(len(windows) - 1):
        assert windows[i][1:] == windows[i + 1][:-1], (
            f"окно {i} хвост {windows[i][1:]} != голова окна {i+1} {windows[i+1][:-1]}"
        )


# --- первый элемент каждого окна совпадает с xs[i] ---------------------------

@given(xs=int_lists, size=valid_size)
def test_window_prop_first_element_matches_source(xs, size):
    for i, w in enumerate(Window(xs, size)):
        assert w[0] == xs[i]


# --- последний элемент каждого окна совпадает с xs[i + size - 1] -------------

@given(xs=int_lists, size=valid_size)
def test_window_prop_last_element_matches_source(xs, size):
    for i, w in enumerate(Window(xs, size)):
        assert w[-1] == xs[i + size - 1]


# --- оракул: совпадает с itertools sliding window (zip(*[islice]*n)) ---------

@given(xs=int_lists, size=valid_size)
def test_window_prop_matches_itertools_oracle(xs, size):
    # Справочная реализация через itertools (stdlib, заведомо верна).
    iterators = itertools.tee(iter(xs), size)
    for offset, it in enumerate(iterators):
        next(itertools.islice(it, offset, offset), None)
    oracle = [list(group) for group in zip(*iterators)]

    result = list(Window(xs, size))
    assert result == oracle


# --- round-trip: склейка содержимого первого столбца восстанавливает xs ------

@given(xs=st.lists(ints, min_size=1, max_size=50), size=valid_size)
def test_window_prop_first_elements_are_prefix_of_xs(xs, size):
    # Первые элементы окон — это xs[0], xs[1], ..., xs[n-size]
    # (т.е. xs без последних size-1 элементов, если окна есть).
    firsts = [w[0] for w in Window(xs, size)]
    expected = xs[:max(0, len(xs) - size + 1)]
    assert firsts == expected


# --- size=1: каждое окно — [xs[i]] ------------------------------------------

@given(xs=int_lists)
def test_window_prop_size1_wraps_each_element(xs):
    result = list(Window(xs, 1))
    assert result == [[x] for x in xs]


# --- size >= len(xs)+1: пусто ------------------------------------------------

@given(xs=int_lists)
def test_window_prop_size_exceeds_length_returns_empty(xs):
    result = list(Window(xs, len(xs) + 1))
    assert result == []


# --- iter(w) is w (итератор возвращает самого себя) --------------------------

@given(xs=int_lists, size=valid_size)
def test_window_prop_iter_returns_self(xs, size):
    w = Window(xs, size)
    assert iter(w) is w


# --- ValueError/NotImplementedError на size < 1 ------------------------------

@given(xs=int_lists, size=st.integers(min_value=-20, max_value=0))
def test_window_prop_nonpositive_size_raises(xs, size):
    with pytest.raises((ValueError, NotImplementedError)):
        list(Window(xs, size))
