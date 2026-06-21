# Эти тесты трогать не нужно — это эталон поведения.
# Модуль 15 — Регулярные выражения.
#
# Фиксированные тесты проверяют конкретные примеры.
# property-тесты — в test_patterns_props.py.

import pytest
from patterns import find_all_numbers, is_valid_identifier, parse_log_line, slugify


# ==========================================================================
# find_all_numbers
# ==========================================================================

@pytest.mark.parametrize("text, expected", [
    ("цена: -10, скидка: 3, остаток: 500", [-10, 3, 500]),
    ("42", [42]),
    ("-7", [-7]),
    ("нет цифр здесь", []),
    ("", []),
    ("a1b2c3", [1, 2, 3]),
    ("0", [0]),
    ("-0", [0]),
    ("100 200 -300", [100, 200, -300]),
    ("1-2",  [1, -2]),   # тире между числами: «1» и «-2»
])
def test_find_all_numbers(text, expected):
    assert find_all_numbers(text) == expected


def test_find_all_numbers_returns_ints():
    result = find_all_numbers("42 и -7")
    assert all(isinstance(x, int) for x in result)


def test_find_all_numbers_order():
    """Числа возвращаются в порядке появления в тексте."""
    assert find_all_numbers("3 потом 1 потом 2") == [3, 1, 2]


# ==========================================================================
# is_valid_identifier
# ==========================================================================

@pytest.mark.parametrize("name, expected", [
    ("x", True),
    ("my_var", True),
    ("_private", True),
    ("CamelCase", True),
    ("var123", True),
    ("_", True),
    ("", False),
    ("2bad", False),
    ("has space", False),
    ("has-dash", False),
    ("123", False),
    ("a!", False),
])
def test_is_valid_identifier(name, expected):
    assert is_valid_identifier(name) is expected


def test_is_valid_identifier_returns_bool():
    """Возвращает именно bool, а не Match/None."""
    result = is_valid_identifier("ok")
    assert isinstance(result, bool)


# ==========================================================================
# slugify
# ==========================================================================

@pytest.mark.parametrize("title, expected", [
    ("Hello World", "hello-world"),
    ("  Hello, World!  ", "hello-world"),
    ("Python 3.12 Release", "python-3-12-release"),
    ("---", ""),
    ("", ""),
    ("already-slug", "already-slug"),
    ("UPPER CASE", "upper-case"),
    ("multiple   spaces", "multiple-spaces"),
    ("a", "a"),
    ("a!@#b", "a-b"),
])
def test_slugify(title, expected):
    assert slugify(title) == expected


def test_slugify_no_leading_trailing_dashes():
    result = slugify("  spaces  ")
    assert not result.startswith("-")
    assert not result.endswith("-")


def test_slugify_consecutive_special_chars_become_one_dash():
    assert slugify("a!!!b") == "a-b"


# ==========================================================================
# parse_log_line
# ==========================================================================

SAMPLE_ERROR = "2026-06-22 14:05:33 ERROR   [auth] Login failed for user alice@example.com"
SAMPLE_INFO  = "2026-06-22 14:06:01 INFO    [db]   Query executed in 42ms"
SAMPLE_WARN  = "2024-01-01 00:00:00 WARNING [scheduler] Task overdue"


def test_parse_log_error_fields():
    result = parse_log_line(SAMPLE_ERROR)
    assert result is not None
    assert result["date"]    == "2026-06-22"
    assert result["time"]    == "14:05:33"
    assert result["level"]   == "ERROR"
    assert result["module"]  == "auth"
    assert result["message"] == "Login failed for user alice@example.com"


def test_parse_log_info_fields():
    result = parse_log_line(SAMPLE_INFO)
    assert result is not None
    assert result["level"]  == "INFO"
    assert result["module"] == "db"
    assert result["message"] == "Query executed in 42ms"


def test_parse_log_warning_fields():
    result = parse_log_line(SAMPLE_WARN)
    assert result is not None
    assert result["level"]  == "WARNING"
    assert result["module"] == "scheduler"
    assert result["date"]   == "2024-01-01"
    assert result["time"]   == "00:00:00"


def test_parse_log_returns_dict_with_five_keys():
    result = parse_log_line(SAMPLE_ERROR)
    assert result is not None
    assert set(result.keys()) == {"date", "time", "level", "module", "message"}


def test_parse_log_invalid_returns_none():
    assert parse_log_line("не лог") is None
    assert parse_log_line("") is None
    assert parse_log_line("2026-06-22 14:00:00 INFO без скобок сообщение") is None


def test_parse_log_message_with_spaces_preserved():
    line = "2026-06-22 12:00:00 DEBUG [core] message with multiple   spaces"
    result = parse_log_line(line)
    assert result is not None
    assert result["message"] == "message with multiple   spaces"


def test_parse_log_strips_leading_trailing_whitespace():
    line = "  2026-06-22 12:00:00 INFO [app] hello  "
    result = parse_log_line(line)
    assert result is not None
    assert result["level"] == "INFO"


def test_parse_log_level_case_preserved():
    """level возвращается в том виде, как написано в строке."""
    line = "2026-06-22 09:00:00 ERROR [x] msg"
    result = parse_log_line(line)
    assert result is not None
    assert result["level"] == "ERROR"


def test_parse_log_single_word_message():
    line = "2026-06-22 09:00:00 INFO [x] ok"
    result = parse_log_line(line)
    assert result is not None
    assert result["message"] == "ok"
