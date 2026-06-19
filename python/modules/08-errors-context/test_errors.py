# Эти тесты трогать не нужно — это эталон поведения.
#
# Они описывают, КАК должен вести себя твой код в errors.py. Сейчас они красные
# (стаб кидает NotImplementedError). Реализуй функции — станут зелёными.
#
# Запуск: ./python/run.sh 08

import pytest

from errors import safe_divide, parse_int, get_or, Suppress


# ---------- safe_divide ----------

def test_safe_divide_normal():
    assert safe_divide(10, 2) == 5.0


def test_safe_divide_returns_float():
    # Деление в Python даёт float даже для целых, делящихся нацело.
    result = safe_divide(7, 2)
    assert result == 3.5
    assert isinstance(result, float)


def test_safe_divide_by_zero_is_none():
    assert safe_divide(1, 0) is None


def test_safe_divide_zero_numerator():
    # 0 / 5 == 0.0 — это валидное деление, не край.
    assert safe_divide(0, 5) == 0.0


def test_safe_divide_negative():
    assert safe_divide(-6, 3) == -2.0


def test_safe_divide_does_not_raise_on_zero():
    # Никакого ZeroDivisionError наружу — функция обязана его погасить.
    try:
        result = safe_divide(5, 0)
    except ZeroDivisionError:
        pytest.fail("safe_divide не должна пробрасывать ZeroDivisionError")
    assert result is None


# ---------- parse_int ----------

def test_parse_int_simple():
    assert parse_int("42") == 42


def test_parse_int_negative():
    assert parse_int("-7") == -7


def test_parse_int_with_surrounding_spaces():
    # int(" 10 ") == 10: пробелы по краям int() обрезает сам.
    assert parse_int(" 10 ") == 10


def test_parse_int_not_a_number():
    assert parse_int("abc") is None


def test_parse_int_float_string_is_none():
    # "3.14" — это НЕ целое, int("3.14") падает → ждём None.
    assert parse_int("3.14") is None


def test_parse_int_empty_string_is_none():
    assert parse_int("") is None


def test_parse_int_trailing_garbage_is_none():
    assert parse_int("12x") is None


def test_parse_int_zero():
    assert parse_int("0") == 0


# ---------- get_or ----------

def test_get_or_valid_index():
    assert get_or([10, 20, 30], 1, -1) == 20


def test_get_or_first_and_last():
    xs = ["a", "b", "c"]
    assert get_or(xs, 0, "?") == "a"
    assert get_or(xs, 2, "?") == "c"


def test_get_or_negative_index():
    # Отрицательные индексы валидны в Python: xs[-1] — последний элемент.
    assert get_or([10, 20, 30], -1, "default") == 30


def test_get_or_out_of_range_returns_default():
    assert get_or([1, 2, 3], 5, "нет") == "нет"


def test_get_or_negative_out_of_range_returns_default():
    assert get_or([1, 2, 3], -4, 0) == 0


def test_get_or_empty_list_returns_default():
    assert get_or([], 0, 999) == 999


def test_get_or_default_can_be_none():
    assert get_or([1, 2], 7, None) is None


# ---------- Suppress (контекст-менеджер) ----------

def test_suppress_swallows_named_exception():
    with Suppress(ValueError):
        raise ValueError("упс")
    # Если дошли сюда без падения — исключение подавлено. Тест проходит.


def test_suppress_lets_other_exception_through():
    # Подавляем только ValueError → KeyError должен пробросить наружу.
    with pytest.raises(KeyError):
        with Suppress(ValueError):
            raise KeyError("другой тип")


def test_suppress_no_exception_is_fine():
    # Когда исключения нет, блок просто отрабатывает.
    flag = []
    with Suppress(ValueError):
        flag.append("дошли")
    assert flag == ["дошли"]


def test_suppress_multiple_types():
    # Можно перечислить несколько типов — все они подавляются.
    with Suppress(ValueError, KeyError):
        raise KeyError("ключ")
    with Suppress(ValueError, KeyError):
        raise ValueError("значение")


def test_suppress_subclass_is_suppressed():
    # IndexError и KeyError — подклассы LookupError. Suppress(LookupError)
    # обязан ловить и их (как обычный except по базовому классу).
    with Suppress(LookupError):
        raise IndexError("вне диапазона")


def test_suppress_enter_returns_manager():
    # __enter__ принято возвращать сам менеджер (для возможного `as`).
    mgr = Suppress(ValueError)
    with mgr as entered:
        pass
    assert entered is mgr


def test_suppress_does_not_swallow_unrelated_in_real_use():
    # Боевой сценарий: подавляем ValueError при parse, но TypeError проходит.
    with pytest.raises(TypeError):
        with Suppress(ValueError):
            int(None)  # TypeError, а не ValueError
