# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_loganalyzer.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — РАНДОМИЗИРОВАННЫЕ
# property-тесты: hypothesis сам генерирует логи (валидные, мусорные, с чужими уровнями,
# с двоеточиями внутри сообщения, юникодом) и проверяет ИНВАРИАНТЫ, а не конкретный ответ.
# Главные свойства: сумма счётчиков == числу подходящих строк; фильтр+подсчёт согласованы;
# подсчёт не зависит от порядка строк.

from collections import Counter

from hypothesis import given, settings
from hypothesis import strategies as st

from loganalyzer import LEVELS, count_levels, errors_only

# Сообщение может содержать что угодно, включая ':' (делим только по ПЕРВОМУ двоеточию).
message = st.text(alphabet=st.characters(blacklist_characters="\n"), max_size=30)

# Допустимые уровни — ровно те, что объявлены модулем.
valid_level = st.sampled_from(sorted(LEVELS))
# Чужие уровни, которых заведомо нет в LEVELS.
unknown_level = st.sampled_from(["DEBUG", "TRACE", "FATAL", "info", "Error", ""])


@st.composite
def valid_line(draw):
    """Строка вида 'LEVEL: message' с корректным уровнем."""
    lvl = draw(valid_level)
    msg = draw(message)
    return f"{lvl}:{msg}"


@st.composite
def unknown_level_line(draw):
    """Строка с двоеточием, но чужим уровнем — должна игнорироваться."""
    lvl = draw(unknown_level)
    msg = draw(message)
    return f"{lvl}:{msg}"


# Мусор без двоеточия — тоже игнорируется.
no_colon_line = st.text(alphabet=st.characters(blacklist_characters="\n:"), max_size=30)

any_line = st.one_of(valid_line(), unknown_level_line(), no_colon_line)
log_lines = st.lists(any_line, max_size=30)


def _expected_level(line: str):
    """Эталонный разбор уровня строки (None, если строка не подходит под формат)."""
    if ":" not in line:
        return None
    lvl = line.split(":", 1)[0]
    return lvl if lvl in LEVELS else None


# --- count_levels ---

@given(lines=log_lines)
@settings(max_examples=100, deadline=None)
def test_count_levels_sum_equals_wellformed_count(lines):
    # Сумма всех счётчиков == числу строк, прошедших фильтр формата.
    counts = count_levels(lines)
    expected_total = sum(1 for ln in lines if _expected_level(ln) is not None)
    assert sum(counts.values()) == expected_total


@given(lines=log_lines)
@settings(max_examples=100, deadline=None)
def test_count_levels_matches_reference(lines):
    # Полное совпадение с независимым эталонным подсчётом.
    expected = Counter(
        lvl for ln in lines if (lvl := _expected_level(ln)) is not None
    )
    assert count_levels(lines) == dict(expected)


@given(lines=log_lines)
@settings(max_examples=100, deadline=None)
def test_count_levels_keys_are_valid_and_nonzero(lines):
    # В ответе только реальные уровни из LEVELS, и нулей не бывает.
    counts = count_levels(lines)
    assert set(counts).issubset(LEVELS)
    assert all(v > 0 for v in counts.values())


@given(lines=log_lines)
@settings(max_examples=100, deadline=None)
def test_count_levels_order_independent(lines):
    # Подсчёт — это мультимножество, он не зависит от порядка строк.
    assert count_levels(list(reversed(lines))) == count_levels(lines)


@given(lines=log_lines)
@settings(max_examples=50, deadline=None)
def test_count_levels_concatenation_adds(lines):
    # Счётчики аддитивны: count(a+b) == count(a) + count(b) поэлементно.
    mid = len(lines) // 2
    left, right = lines[:mid], lines[mid:]
    combined = Counter(count_levels(left)) + Counter(count_levels(right))
    assert count_levels(lines) == dict(combined)


def test_count_levels_empty_is_empty_dict():
    # Краевой случай: пустой ввод -> {}.
    assert count_levels([]) == {}


def test_count_levels_colon_in_message_does_not_break():
    # Двоеточие внутри сообщения не мешает: делим только по первому ':'.
    lines = ["ERROR: time 12:30:00", "INFO: http://x", "WARN: a:b:c"]
    assert count_levels(lines) == {"ERROR": 1, "INFO": 1, "WARN": 1}


# --- errors_only ---

@given(lines=log_lines)
@settings(max_examples=100, deadline=None)
def test_errors_only_count_matches_count_levels(lines):
    # Длина errors_only == счётчику ERROR из count_levels (фильтр и подсчёт согласованы).
    assert len(errors_only(lines)) == count_levels(lines).get("ERROR", 0)


@given(lines=log_lines)
@settings(max_examples=100, deadline=None)
def test_errors_only_is_order_preserving_sublist(lines):
    # Результат — подпоследовательность исходных строк в том же порядке,
    # и каждый элемент действительно уровня ERROR, возвращён целиком.
    result = errors_only(lines)
    expected = [ln for ln in lines if _expected_level(ln) == "ERROR"]
    assert result == expected
    for line in result:
        assert line in lines
        assert _expected_level(line) == "ERROR"


@given(lines=log_lines)
@settings(max_examples=100, deadline=None)
def test_errors_only_idempotent_on_filtered(lines):
    # Повторный фильтр уже отфильтрованного списка ничего не меняет.
    once = errors_only(lines)
    assert errors_only(once) == once


def test_errors_only_empty_input():
    # Краевой случай: пустой ввод -> [].
    assert errors_only([]) == []


def test_errors_only_ignores_malformed_and_unknown():
    # Строки без ':' и с чужим уровнем не считаются ошибками.
    lines = ["мусор без двоеточия", "DEBUG: not error", "ERROR: настоящая"]
    assert errors_only(lines) == ["ERROR: настоящая"]
