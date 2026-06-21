# Эти тесты трогать не нужно — это эталон поведения.
#
# Они описывают, КАК должен вести себя твой код в errors.py. Сейчас они красные
# (стаб кидает NotImplementedError). Реализуй функции — станут зелёными.
#
# Запуск: ./python/run.sh 08

import pytest

from errors import safe_divide, parse_int, get_or, Suppress, tag, MissingKeyError, wrap_lookup


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


# ---------- tag (генераторный контекст-менеджер) ----------

def test_tag_prints_open_and_close(capsys):
    with tag("span"):
        pass
    out = capsys.readouterr().out
    assert out == "<span>\n</span>\n"


def test_tag_close_printed_even_on_exception(capsys):
    # teardown (закрывающий тег) должен выполниться даже при исключении.
    with pytest.raises(ValueError):
        with tag("div"):
            raise ValueError("что-то пошло не так")
    out = capsys.readouterr().out
    assert "<div>" in out
    assert "</div>" in out


def test_tag_exception_propagates_after_teardown(capsys):
    # Исключение внутри блока не проглатывается — оно пробрасывается наружу.
    with pytest.raises(RuntimeError, match="boom"):
        with tag("p"):
            raise RuntimeError("boom")


def test_tag_nesting(capsys):
    # Вложенные теги работают корректно.
    with tag("outer"):
        with tag("inner"):
            pass
    out = capsys.readouterr().out
    assert out == "<outer>\n<inner>\n</inner>\n</outer>\n"


def test_tag_no_exception_normal_execution(capsys):
    # Без исключения тело блока выполняется полностью.
    results = []
    with tag("ul"):
        results.append("item")
    out = capsys.readouterr().out
    assert results == ["item"]
    assert out == "<ul>\n</ul>\n"


# ---------- wrap_lookup / MissingKeyError (связывание исключений) ----------

def test_wrap_lookup_found():
    assert wrap_lookup({"a": 1}, "a") == 1


def test_wrap_lookup_missing_raises_missing_key_error():
    with pytest.raises(MissingKeyError):
        wrap_lookup({"a": 1}, "z")


def test_wrap_lookup_cause_is_key_error():
    # __cause__ должен быть оригинальным KeyError (raise … from err).
    with pytest.raises(MissingKeyError) as exc_info:
        wrap_lookup({}, "missing")
    assert exc_info.value.__cause__ is not None
    assert isinstance(exc_info.value.__cause__, KeyError)


def test_wrap_lookup_message_contains_key():
    # Сообщение доменной ошибки должно содержать имя ключа.
    with pytest.raises(MissingKeyError, match="not found"):
        wrap_lookup({}, "my_key")


def test_wrap_lookup_missing_key_error_is_exception():
    # MissingKeyError — нормальное Exception, а не BaseException.
    assert issubclass(MissingKeyError, Exception)


def test_wrap_lookup_cause_key_matches():
    # Оригинальный KeyError содержит тот же ключ, что мы искали.
    with pytest.raises(MissingKeyError) as exc_info:
        wrap_lookup({"x": 10}, "y")
    original = exc_info.value.__cause__
    assert isinstance(original, KeyError)
    assert original.args[0] == "y"


def test_missing_key_error_can_be_caught_separately():
    # MissingKeyError можно поймать отдельно от KeyError — это доменная ошибка.
    caught = False
    try:
        wrap_lookup({}, "k")
    except MissingKeyError:
        caught = True
    assert caught


def test_wrap_lookup_does_not_raise_key_error_directly():
    # Вызывающий код получает MissingKeyError, а не голый KeyError.
    # Убеждаемся, что исключение ровно MissingKeyError — не KeyError и не Exception.
    exc_type = None
    try:
        wrap_lookup({}, "k")
    except MissingKeyError as e:
        exc_type = type(e)
    except KeyError:
        pytest.fail("wrap_lookup не должен пробрасывать KeyError напрямую")
    assert exc_type is MissingKeyError
