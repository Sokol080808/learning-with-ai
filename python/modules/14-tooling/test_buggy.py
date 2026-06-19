# Эти тесты трогать не нужно — это эталон поведения.
# Модуль 14 — Инструменты и качество кода (модуль про «найди и почини баги»).
#
# Сейчас эти тесты КРАСНЫЕ: в buggy.py спрятаны баги. Когда починишь код, не трогая
# тесты, они должны позеленеть. Тесты — это спецификация ПРАВИЛЬНОГО поведения.

from buggy import (
    average,
    has_duplicate,
    max_in,
    repeat,
    sum_first_n,
)


# --- sum_first_n: off-by-one ---

def test_sum_first_n_basic():
    assert sum_first_n([10, 20, 30, 40], 2) == 30


def test_sum_first_n_takes_exactly_n():
    # ровно n элементов, не n-1 и не n+1
    assert sum_first_n([1, 2, 3, 4, 5], 3) == 6


def test_sum_first_n_single():
    assert sum_first_n([5], 1) == 5


def test_sum_first_n_zero():
    assert sum_first_n([1, 2, 3], 0) == 0


def test_sum_first_n_whole_list():
    assert sum_first_n([1, 2, 3], 3) == 6


# --- max_in: неверная инициализация ---

def test_max_in_positive():
    assert max_in([3, 1, 2]) == 3


def test_max_in_single():
    assert max_in([7]) == 7


def test_max_in_all_negative():
    # ключевой случай: старт с 0 ломается, если все числа отрицательные
    assert max_in([-5, -2, -9]) == -2


def test_max_in_negative_and_zero():
    assert max_in([0, -1, -3]) == 0


def test_max_in_max_is_last():
    assert max_in([-10, -20, -1]) == -1


# --- average: лишняя +1 / неверный делитель ---

def test_average_simple():
    assert average([2, 4]) == 3.0


def test_average_single():
    assert average([10]) == 10.0


def test_average_returns_float():
    result = average([1, 2, 3, 4])
    assert result == 2.5
    assert isinstance(result, float)


def test_average_negative():
    assert average([-2, -4]) == -3.0


def test_average_with_zero():
    # сумма 0, среднее ровно 0.0 — никаких лишних единиц
    assert average([0, 0, 0]) == 0.0


# --- has_duplicate: сравнение элемента с собой ---

def test_has_duplicate_no_dups():
    assert has_duplicate([1, 2, 3]) is False


def test_has_duplicate_has_dups():
    assert has_duplicate([1, 2, 2]) is True


def test_has_duplicate_empty():
    assert has_duplicate([]) is False


def test_has_duplicate_single():
    # один элемент сам с собой дубликатом НЕ считается
    assert has_duplicate([5]) is False


def test_has_duplicate_all_same():
    assert has_duplicate([1, 1]) is True


def test_has_duplicate_dup_far_apart():
    assert has_duplicate([7, 1, 2, 3, 7]) is True


# --- repeat: times-1 ---

def test_repeat_basic():
    assert repeat("ab", 3) == "ababab"


def test_repeat_once():
    assert repeat("x", 1) == "x"


def test_repeat_zero():
    assert repeat("ab", 0) == ""


def test_repeat_count():
    # ровно 5 повторений, не 4
    assert repeat("-", 5) == "-----"
    assert len(repeat("-", 5)) == 5
