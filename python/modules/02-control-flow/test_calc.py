# Эти тесты трогать не нужно — это эталон поведения.
# Запуск: ./python/run.sh 02
import pytest

from calc import calc


# --- штатные операции -------------------------------------------------------

def test_add():
    assert calc(2, "+", 3) == 5


def test_subtract():
    assert calc(10, "-", 4) == 6


def test_multiply():
    assert calc(6, "*", 7) == 42


def test_divide():
    assert calc(10, "/", 4) == 2.5


def test_divide_returns_float():
    assert calc(9, "/", 3) == 3.0


def test_works_with_floats():
    assert calc(0.1, "+", 0.2) == pytest.approx(0.3)
    assert calc(2.5, "*", 4) == 10.0


def test_negative_result():
    assert calc(3, "-", 8) == -5


# --- ошибки -----------------------------------------------------------------

def test_division_by_zero_raises_value_error():
    with pytest.raises(ValueError):
        calc(1, "/", 0)


def test_unknown_operation_raises_value_error():
    with pytest.raises(ValueError):
        calc(1, "?", 2)


def test_unknown_operation_word_raises_value_error():
    with pytest.raises(ValueError):
        calc(5, "plus", 5)
