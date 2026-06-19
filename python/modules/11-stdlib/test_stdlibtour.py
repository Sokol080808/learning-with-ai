# Эти тесты трогать не нужно — это эталон поведения.
# Модуль 11 — Полезная стандартная библиотека.

import json

import pytest

from stdlibtour import (
    days_between,
    from_json_str,
    group_by_first_letter,
    most_common_word,
    to_json_str,
)


# --- Задание 1: most_common_word (collections.Counter) ---

def test_most_common_word_simple():
    assert most_common_word("a b a c a") == "a"


def test_most_common_word_single_word():
    assert most_common_word("hello") == "hello"


def test_most_common_word_collapses_extra_spaces():
    # лишние пробелы не создают пустых "слов" — split() их схлопывает
    assert most_common_word("  dog   cat dog  ") == "dog"


def test_most_common_word_case_and_punct_not_normalized():
    # регистр и пунктуацию НЕ трогаем: "The" и "the" — разные слова
    assert most_common_word("the the The") == "the"


def test_most_common_word_empty_text_returns_empty_string():
    assert most_common_word("") == ""


def test_most_common_word_whitespace_only_returns_empty_string():
    assert most_common_word("   \t  \n ") == ""


# --- Задание 2: group_by_first_letter (collections.defaultdict) ---

def test_group_by_first_letter_basic():
    words = ["apple", "avocado", "banana", "cherry", "blue"]
    result = group_by_first_letter(words)
    assert result == {
        "a": ["apple", "avocado"],
        "b": ["banana", "blue"],
        "c": ["cherry"],
    }


def test_group_by_first_letter_preserves_order():
    words = ["bob", "alice", "ben", "amy", "anna"]
    result = group_by_first_letter(words)
    assert result["b"] == ["bob", "ben"]
    assert result["a"] == ["alice", "amy", "anna"]


def test_group_by_first_letter_ignores_empty_strings():
    words = ["", "fox", "", "fish"]
    result = group_by_first_letter(words)
    assert result == {"f": ["fox", "fish"]}


def test_group_by_first_letter_empty_input():
    assert group_by_first_letter([]) == {}


# --- Задания 3 и 4: json туда и обратно ---

def test_to_json_str_returns_valid_json():
    obj = {"name": "Ada", "age": 36}
    s = to_json_str(obj)
    assert isinstance(s, str)
    # строка должна разбираться штатным json и давать тот же объект
    assert json.loads(s) == obj


def test_from_json_str_parses():
    assert from_json_str('{"x": 1, "y": [2, 3]}') == {"x": 1, "y": [2, 3]}


def test_json_roundtrip_dict():
    obj = {"name": "Ada", "age": 36, "active": True, "tags": ["py", "cpp"], "note": None}
    assert from_json_str(to_json_str(obj)) == obj


def test_json_roundtrip_list():
    obj = [1, 2, 3, "four", False]
    assert from_json_str(to_json_str(obj)) == obj


def test_json_roundtrip_scalars():
    assert from_json_str(to_json_str(42)) == 42
    assert from_json_str(to_json_str("hi")) == "hi"
    assert from_json_str(to_json_str(True)) is True
    assert from_json_str(to_json_str(None)) is None


# --- Задание 5: days_between (datetime.date.fromisoformat) ---

def test_days_between_same_month():
    assert days_between("2024-01-01", "2024-01-31") == 30


def test_days_between_same_date_is_zero():
    assert days_between("2024-06-19", "2024-06-19") == 0


def test_days_between_negative_when_a_after_b():
    assert days_between("2024-01-31", "2024-01-01") == -30


def test_days_between_full_year():
    # 2023 — не високосный, ровно 365 дней
    assert days_between("2023-01-01", "2024-01-01") == 365


def test_days_between_counts_leap_day():
    # 2020 — високосный: между 28 февраля и 1 марта целых 2 дня (есть 29 февраля)
    assert days_between("2020-02-28", "2020-03-01") == 2


def test_days_between_no_leap_day():
    # 2021 — не високосный: между 28 февраля и 1 марта только 1 день
    assert days_between("2021-02-28", "2021-03-01") == 1


def test_days_between_rejects_bad_format():
    # fromisoformat строгий: не-ISO формат должен приводить к ValueError
    with pytest.raises(ValueError):
        days_between("01/03/2024", "2024-03-02")
