# Эти тесты трогать не нужно — это эталон поведения.
# Микро-проект модуля 12 — Анализатор логов.

from loganalyzer import count_levels, errors_only


# --- count_levels ---

def test_count_levels_basic():
    lines = [
        "INFO: started",
        "WARN: low disk",
        "ERROR: disk full",
        "INFO: retry",
    ]
    assert count_levels(lines) == {"INFO": 2, "WARN": 1, "ERROR": 1}


def test_count_levels_empty_input():
    assert count_levels([]) == {}


def test_count_levels_only_present_levels():
    # отсутствующие уровни в словаре не появляются (нет нулей)
    lines = ["INFO: a", "INFO: b"]
    assert count_levels(lines) == {"INFO": 2}


def test_count_levels_ignores_lines_without_colon():
    # строка без ':' не подходит под формат — её пропускаем
    lines = ["INFO: ok", "просто текст без двоеточия", "ERROR: boom"]
    assert count_levels(lines) == {"INFO": 1, "ERROR": 1}


def test_count_levels_ignores_unknown_level():
    # DEBUG не входит в набор INFO/WARN/ERROR — игнорируем
    lines = ["DEBUG: trace", "ERROR: boom", "TRACE: x"]
    assert count_levels(lines) == {"ERROR": 1}


def test_count_levels_colon_in_message_is_fine():
    # двоеточие в самом сообщении не мешает: делим только по первому ':'
    lines = ["ERROR: time is 12:30", "INFO: url http://x"]
    assert count_levels(lines) == {"ERROR": 1, "INFO": 1}


def test_count_levels_all_three_levels_mixed():
    lines = [
        "ERROR: e1",
        "WARN: w1",
        "INFO: i1",
        "WARN: w2",
        "ERROR: e2",
        "ERROR: e3",
    ]
    assert count_levels(lines) == {"ERROR": 3, "WARN": 2, "INFO": 1}


# --- errors_only ---

def test_errors_only_basic():
    lines = [
        "INFO: started",
        "ERROR: disk full",
        "WARN: low disk",
        "ERROR: crash",
    ]
    assert errors_only(lines) == ["ERROR: disk full", "ERROR: crash"]


def test_errors_only_keeps_whole_line():
    # возвращаем строку целиком, не только сообщение
    lines = ["ERROR: boom"]
    assert errors_only(lines) == ["ERROR: boom"]


def test_errors_only_preserves_order():
    lines = ["ERROR: first", "INFO: x", "ERROR: second", "WARN: y", "ERROR: third"]
    assert errors_only(lines) == ["ERROR: first", "ERROR: second", "ERROR: third"]


def test_errors_only_none_match():
    lines = ["INFO: a", "WARN: b"]
    assert errors_only(lines) == []


def test_errors_only_empty_input():
    assert errors_only([]) == []


def test_errors_only_ignores_malformed():
    # строки без ':' или с чужим уровнем не считаются ошибками
    lines = ["мусор без двоеточия", "DEBUG: not an error", "ERROR: real"]
    assert errors_only(lines) == ["ERROR: real"]
