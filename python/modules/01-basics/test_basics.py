# Эти тесты трогать не нужно — это эталон поведения.
#
# Они описывают, КАК должны вести себя функции из basics.py. Пока стаб не реализован,
# тесты падают (NotImplementedError). Реализуешь функцию правильно — её тесты зеленеют.

import pytest

from basics import (
    to_fahrenheit,
    is_even,
    min_of_three,
    average3,
    greet,
    parse_number,
)


# --- to_fahrenheit ---

def test_to_fahrenheit_freezing():
    assert to_fahrenheit(0) == pytest.approx(32.0)


def test_to_fahrenheit_boiling():
    assert to_fahrenheit(100) == pytest.approx(212.0)


def test_to_fahrenheit_body_temperature():
    # 37 * 9 / 5 + 32 = 98.6 (с точностью до погрешности float)
    assert to_fahrenheit(37) == pytest.approx(98.6)


def test_to_fahrenheit_negative():
    # Знаменитая точка, где обе шкалы совпадают: -40 C == -40 F
    assert to_fahrenheit(-40) == pytest.approx(-40.0)


def test_to_fahrenheit_returns_float():
    # Деление / гарантирует float даже для «круглого» результата
    assert isinstance(to_fahrenheit(0), float)


def test_to_fahrenheit_fractional_input():
    # 36.6 * 9 / 5 + 32 = 97.88
    assert to_fahrenheit(36.6) == pytest.approx(97.88)


# --- is_even ---

def test_is_even_true_for_even():
    assert is_even(4) is True


def test_is_even_false_for_odd():
    assert is_even(7) is False


def test_is_even_zero_is_even():
    assert is_even(0) is True


def test_is_even_negative_even():
    assert is_even(-2) is True


def test_is_even_negative_odd():
    assert is_even(-3) is False


def test_is_even_returns_bool():
    assert isinstance(is_even(10), bool)


# --- min_of_three ---

def test_min_of_three_first_smallest():
    assert min_of_three(1, 2, 3) == 1


def test_min_of_three_middle_smallest():
    assert min_of_three(5, 1, 9) == 1


def test_min_of_three_last_smallest():
    assert min_of_three(8, 4, 2) == 2


def test_min_of_three_all_equal():
    assert min_of_three(2, 2, 2) == 2


def test_min_of_three_negatives():
    assert min_of_three(-1, -7, -3) == -7


def test_min_of_three_duplicates_at_min():
    assert min_of_three(5, 5, 9) == 5


# --- average3 ---

def test_average3_whole_result():
    # (1 + 2 + 3) / 3 = 2.0
    assert average3(1, 2, 3) == pytest.approx(2.0)


def test_average3_fractional_result():
    # (1 + 2 + 2) / 3 = 1.666...  — точное дробное, НЕ 1
    assert average3(1, 2, 2) == pytest.approx(5 / 3)


def test_average3_zeros():
    assert average3(0, 0, 0) == pytest.approx(0.0)


def test_average3_returns_float():
    # Среднее обязано быть float (деление /, а не //)
    assert isinstance(average3(2, 3, 4), float)


def test_average3_not_floored():
    # Ловушка: при // здесь получилось бы 1, а правильный ответ — 1.666...
    assert average3(1, 2, 2) != 1


def test_average3_negatives():
    # (-2 + -4 + -6) / 3 = -4.0
    assert average3(-2, -4, -6) == pytest.approx(-4.0)


# --- greet ---

def test_greet_basic():
    assert greet("Аня") == "Привет, Аня!"


def test_greet_another_name():
    assert greet("Мир") == "Привет, Мир!"


def test_greet_returns_str():
    assert isinstance(greet("Боб"), str)


def test_greet_empty_name():
    # Контракт прямой: имя подставляется как есть
    assert greet("") == "Привет, !"


# --- parse_number: фиксированные краевые случаи ---

def test_parse_number_positive_int():
    assert parse_number("42") == 42
    assert isinstance(parse_number("42"), int)


def test_parse_number_negative_int():
    assert parse_number("-7") == -7
    assert isinstance(parse_number("-7"), int)


def test_parse_number_zero_int():
    assert parse_number("0") == 0
    assert isinstance(parse_number("0"), int)


def test_parse_number_positive_float():
    result = parse_number("3.14")
    assert result == pytest.approx(3.14)
    assert isinstance(result, float)


def test_parse_number_negative_float():
    result = parse_number("-0.5")
    assert result == pytest.approx(-0.5)
    assert isinstance(result, float)


def test_parse_number_scientific_notation():
    # "1e3" — это 1000.0: float с экспонентой
    result = parse_number("1e3")
    assert result == pytest.approx(1000.0)
    assert isinstance(result, float)


def test_parse_number_with_whitespace():
    # Пробелы по краям — допустимо игнорировать
    assert parse_number("  42  ") == 42
    assert isinstance(parse_number("  42  "), int)


def test_parse_number_garbage_returns_none():
    assert parse_number("abc") is None


def test_parse_number_empty_string_returns_none():
    assert parse_number("") is None


def test_parse_number_mixed_garbage_returns_none():
    assert parse_number("12abc") is None
    assert parse_number("3.1.4") is None


def test_parse_number_int_string_not_float():
    # "42" должен давать int, а не float
    result = parse_number("42")
    assert type(result) is int, f"ожидали int, получили {type(result)}"
