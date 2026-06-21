# Эти тесты трогать не нужно — это эталон поведения, но другого рода.
#
# test_patterns.py проверяет ФИКСИРОВАННЫЕ примеры.
# Здесь — РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам генерирует сотни
# входов (включая пустые строки, юникод, длинные повторы) и проверяет не
# конкретный ответ, а ИНВАРИАНТЫ — свойства, обязанные выполняться для любого
# входа. derandomize=True фиксирует seed: тесты воспроизводимы в CI.

import re
import string

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from patterns import find_all_numbers, is_valid_identifier, parse_log_line, slugify

# ---------------------------------------------------------------------------
# Стратегии
# ---------------------------------------------------------------------------

# Строки с цифрами и разными разделителями
text_with_numbers = st.text(
    alphabet=string.ascii_letters + string.digits + " ,.-+_:/!",
    min_size=0, max_size=80,
)

# Валидные идентификаторы: начинаются с [a-zA-Z_], продолжаются [a-zA-Z0-9_]
valid_id_start = st.sampled_from(string.ascii_letters + "_")
valid_id_rest  = st.text(
    alphabet=string.ascii_letters + string.digits + "_",
    min_size=0, max_size=20,
)

# Произвольные строки — для проверки «не падает»
arbitrary_text = st.text(min_size=0, max_size=100)

# Компоненты лог-строки
log_dates    = st.from_regex(r"\d{4}-\d{2}-\d{2}", fullmatch=True)
log_times    = st.from_regex(r"\d{2}:\d{2}:\d{2}", fullmatch=True)
log_levels   = st.sampled_from(["INFO", "ERROR", "WARNING", "DEBUG", "CRITICAL"])
log_modules  = st.from_regex(r"\w+", fullmatch=True).filter(lambda s: len(s) >= 1)
log_messages = st.text(
    alphabet=string.ascii_letters + string.digits + " .:,_/-@",
    min_size=1, max_size=60,
)

# ---------------------------------------------------------------------------
# find_all_numbers — инвариант: количество результатов совпадает с числом
# вставленных чисел; round-trip через str.
# ---------------------------------------------------------------------------

@settings(derandomize=True, max_examples=200)
@given(nums=st.lists(st.integers(min_value=-9999, max_value=9999), min_size=0, max_size=10))
def test_find_all_numbers_count_matches_inserted(nums):
    """Если вставить N чисел в строку через буквенные разделители,
    find_all_numbers вернёт ровно N элементов."""
    # Используем буквенные разделители, чтобы минусы не «прилипали» к числам
    text = "x".join(str(n) for n in nums)
    result = find_all_numbers(text)
    assert len(result) == len(nums)
    assert result == nums


@settings(derandomize=True, max_examples=200)
@given(nums=st.lists(
    st.integers(min_value=0, max_value=99999),   # только неотрицательные: нет ложных минусов
    min_size=1, max_size=8,
))
def test_find_all_numbers_roundtrip_non_negative(nums):
    """Числа, встроенные в строку через нецифровые разделители, извлекаются обратно."""
    text = " и ".join(str(n) for n in nums)
    assert find_all_numbers(text) == nums


@settings(derandomize=True, max_examples=150)
@given(text=text_with_numbers)
def test_find_all_numbers_returns_only_ints(text):
    """Все возвращённые элементы — int."""
    result = find_all_numbers(text)
    assert all(isinstance(x, int) for x in result)


# ---------------------------------------------------------------------------
# is_valid_identifier — инварианты:
# * валидные идентификаторы всегда принимаются
# * строки с «плохими» символами всегда отвергаются
# * результат всегда bool
# ---------------------------------------------------------------------------

@settings(derandomize=True, max_examples=300)
@given(start=valid_id_start, rest=valid_id_rest)
def test_is_valid_identifier_accepts_valid(start, rest):
    """Любая строка вида [a-zA-Z_][a-zA-Z0-9_]* — валидный идентификатор."""
    name = start + rest
    assert is_valid_identifier(name) is True


@settings(derandomize=True, max_examples=200)
@given(st.text(alphabet=" !@#$%^&*()-=+[]{}|;':\",./<>?", min_size=1, max_size=5))
def test_is_valid_identifier_rejects_special_chars(junk):
    """Строки, состоящие только из спецсимволов, не являются идентификаторами."""
    assert is_valid_identifier(junk) is False


@settings(derandomize=True, max_examples=200)
@given(arbitrary_text)
def test_is_valid_identifier_returns_bool(text):
    """is_valid_identifier всегда возвращает bool."""
    result = is_valid_identifier(text)
    assert isinstance(result, bool)


@settings(derandomize=True, max_examples=200)
@given(start=valid_id_start, rest=valid_id_rest)
def test_is_valid_identifier_idempotent_via_fullmatch(start, rest):
    """Для валидного идентификатора fullmatch самим паттерном подтверждает ответ."""
    name = start + rest
    # независимая проверка — тот же результат
    expected = bool(re.fullmatch(r"[a-zA-Z_][a-zA-Z0-9_]*", name))
    assert is_valid_identifier(name) == expected


# ---------------------------------------------------------------------------
# slugify — инварианты:
# * slug не содержит пробелов
# * slug состоит только из [a-z0-9-]
# * нет ведущих/конечных дефисов
# * идемпотентен (уже slug → slug)
# ---------------------------------------------------------------------------

@settings(derandomize=True, max_examples=300)
@given(arbitrary_text)
def test_slugify_no_spaces(text):
    """Результат slugify не содержит пробелов."""
    assert " " not in slugify(text)


@settings(derandomize=True, max_examples=300)
@given(arbitrary_text)
def test_slugify_only_allowed_chars(text):
    """Slug состоит только из [a-z0-9-] или пуст."""
    result = slugify(text)
    assert re.fullmatch(r"[a-z0-9-]*", result) is not None


@settings(derandomize=True, max_examples=300)
@given(arbitrary_text)
def test_slugify_no_leading_trailing_dashes(text):
    """Slug не начинается и не заканчивается на дефис."""
    result = slugify(text)
    assert not result.startswith("-")
    assert not result.endswith("-")


@settings(derandomize=True, max_examples=300)
@given(arbitrary_text)
def test_slugify_idempotent(text):
    """slugify(slugify(x)) == slugify(x) — функция идемпотентна."""
    once = slugify(text)
    twice = slugify(once)
    assert once == twice


@settings(derandomize=True, max_examples=200)
@given(arbitrary_text)
def test_slugify_lowercase(text):
    """Результат всегда строчной."""
    assert slugify(text) == slugify(text).lower()


# ---------------------------------------------------------------------------
# parse_log_line — round-trip: собрать строку из компонентов → распарсить →
# убедиться, что компоненты вернулись без изменений.
# ---------------------------------------------------------------------------

@settings(derandomize=True, max_examples=300)
@given(
    date=log_dates,
    time_=log_times,
    level=log_levels,
    module=log_modules,
    message=log_messages,
)
def test_parse_log_line_roundtrip(date, time_, level, module, message):
    """Строка, собранная из компонентов, парсится обратно в те же компоненты."""
    line = f"{date} {time_} {level} [{module}] {message}"
    result = parse_log_line(line)
    assert result is not None, f"Не распарсилось: {line!r}"
    assert result["date"]    == date
    assert result["time"]    == time_
    assert result["level"]   == level
    assert result["module"]  == module
    assert result["message"] == message


@settings(derandomize=True, max_examples=200)
@given(arbitrary_text)
def test_parse_log_line_does_not_raise(text):
    """parse_log_line никогда не бросает исключение, только None или dict."""
    result = parse_log_line(text)
    assert result is None or isinstance(result, dict)


@settings(derandomize=True, max_examples=200)
@given(
    date=log_dates,
    time_=log_times,
    level=log_levels,
    module=log_modules,
    message=log_messages,
)
def test_parse_log_line_result_has_five_keys(date, time_, level, module, message):
    """Успешный результат всегда содержит ровно 5 ключей."""
    line = f"{date} {time_} {level} [{module}] {message}"
    result = parse_log_line(line)
    assert result is not None
    assert set(result.keys()) == {"date", "time", "level", "module", "message"}
