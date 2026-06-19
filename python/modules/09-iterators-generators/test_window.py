# test_window.py — детерминированные тесты для класса Window (модуль 09).
#
# Проверяют КОНКРЕТНОЕ поведение: фиксированные входы → ожидаемые выходы.
# Трогать не нужно — это эталон поведения. Реализуй Window в gens.py.

import pytest

from gens import Window


# --- базовое поведение -------------------------------------------------------

def test_window_basic_size3():
    assert list(Window([1, 2, 3, 4, 5], 3)) == [[1, 2, 3], [2, 3, 4], [3, 4, 5]]


def test_window_basic_size2():
    assert list(Window([10, 20, 30, 40], 2)) == [[10, 20], [20, 30], [30, 40]]


def test_window_size1_equals_wrapped_elements():
    assert list(Window([5, 6, 7], 1)) == [[5], [6], [7]]


def test_window_size_equals_length():
    assert list(Window([1, 2, 3], 3)) == [[1, 2, 3]]


# --- пустой результат --------------------------------------------------------

def test_window_size_larger_than_input_returns_empty():
    assert list(Window([1, 2], 5)) == []


def test_window_empty_iterable_returns_empty():
    assert list(Window([], 3)) == []


def test_window_empty_iterable_size1_returns_empty():
    assert list(Window([], 1)) == []


# --- результат — списки ------------------------------------------------------

def test_window_each_element_is_list():
    for window in Window([1, 2, 3, 4], 2):
        assert isinstance(window, list)


def test_window_each_element_has_correct_length():
    size = 3
    for window in Window(list(range(10)), size):
        assert len(window) == size


# --- протокол итератора: __iter__ возвращает self ----------------------------

def test_window_iter_returns_self():
    w = Window([1, 2, 3], 2)
    assert iter(w) is w


def test_window_is_not_a_generator_function():
    # Window — это класс, а не функция-генератор. Явная проверка протокола.
    import inspect
    w = Window([1, 2, 3], 2)
    assert not inspect.isgenerator(w)
    # но он итерируем — __iter__ и __next__ присутствуют
    assert hasattr(w, "__iter__")
    assert hasattr(w, "__next__")


def test_window_next_step_by_step():
    w = Window([1, 2, 3, 4], 2)
    assert next(w) == [1, 2]
    assert next(w) == [2, 3]
    assert next(w) == [3, 4]
    with pytest.raises(StopIteration):
        next(w)


# --- работает с нессписочными итерируемыми -----------------------------------

def test_window_on_string():
    result = list(Window("abcd", 2))
    assert result == [["a", "b"], ["b", "c"], ["c", "d"]]


def test_window_on_range():
    assert list(Window(range(5), 2)) == [[0, 1], [1, 2], [2, 3], [3, 4]]


def test_window_on_generator_input():
    # Window должен работать с генератором (одноразовым итерируемым)
    gen = (x * x for x in range(5))  # 0 1 4 9 16
    result = list(Window(gen, 3))
    assert result == [[0, 1, 4], [1, 4, 9], [4, 9, 16]]


# --- ValueError при size < 1 -------------------------------------------------

def test_window_zero_size_raises_value_error():
    with pytest.raises((ValueError, NotImplementedError)):
        list(Window([1, 2, 3], 0))


def test_window_negative_size_raises_value_error():
    with pytest.raises((ValueError, NotImplementedError)):
        list(Window([1, 2, 3], -1))


# --- количество окон ---------------------------------------------------------

def test_window_count_basic():
    result = list(Window(list(range(7)), 3))
    # max(0, 7 - 3 + 1) == 5
    assert len(result) == 5


def test_window_count_exact_fit():
    result = list(Window([1, 2, 3, 4], 2))
    # max(0, 4 - 2 + 1) == 3
    assert len(result) == 3


# --- перекрытие соседних окон ------------------------------------------------

def test_window_overlap():
    windows = list(Window([1, 2, 3, 4, 5], 3))
    for i in range(len(windows) - 1):
        # хвост текущего окна == голова следующего
        assert windows[i][1:] == windows[i + 1][:-1]


# --- одиночный элемент -------------------------------------------------------

def test_window_single_element_size1():
    assert list(Window([42], 1)) == [[42]]


def test_window_single_element_size2_returns_empty():
    assert list(Window([42], 2)) == []
