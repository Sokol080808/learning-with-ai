# Тесты для TypedCSVReader — контекст-менеджера с приведением типов колонок.
# Используй tmp_path: всё временное, в репо не остаётся.

import pytest

from datafiles import TypedCSVReader


# ---------------------------------------------------------------------------
# Вспомогательная функция: записать CSV во временный файл
# ---------------------------------------------------------------------------

def _write_csv(path, rows: list[list[str]]) -> None:
    """Записать строки CSV в файл (через '\\n', без кавычек)."""
    path.write_text("\n".join(",".join(r) for r in rows), encoding="utf-8")


# ---------------------------------------------------------------------------
# Базовое чтение
# ---------------------------------------------------------------------------

def test_typed_csv_reader_returns_rows_as_dicts(tmp_path):
    p = tmp_path / "data.csv"
    _write_csv(p, [["name", "age"], ["Ada", "36"], ["Bjarne", "70"]])
    with TypedCSVReader(p, schema={}) as reader:
        rows = list(reader)
    assert rows == [
        {"name": "Ada", "age": "36"},
        {"name": "Bjarne", "age": "70"},
    ]


def test_typed_csv_reader_converts_int_column(tmp_path):
    p = tmp_path / "nums.csv"
    _write_csv(p, [["name", "score"], ["Ada", "95"], ["Bjarne", "72"]])
    with TypedCSVReader(p, schema={"score": int}) as reader:
        rows = list(reader)
    assert rows[0]["score"] == 95
    assert rows[1]["score"] == 72
    assert isinstance(rows[0]["score"], int)


def test_typed_csv_reader_converts_float_column(tmp_path):
    p = tmp_path / "floats.csv"
    _write_csv(p, [["x", "y"], ["1", "2.5"], ["3", "4.0"]])
    with TypedCSVReader(p, schema={"x": int, "y": float}) as reader:
        rows = list(reader)
    assert rows[0] == {"x": 1, "y": 2.5}
    assert rows[1] == {"x": 3, "y": 4.0}


def test_typed_csv_reader_lambda_converter(tmp_path):
    p = tmp_path / "bool.csv"
    _write_csv(p, [["name", "active"], ["Ada", "true"], ["Bjarne", "false"]])
    with TypedCSVReader(p, schema={"active": lambda s: s == "true"}) as reader:
        rows = list(reader)
    assert rows[0]["active"] is True
    assert rows[1]["active"] is False


def test_typed_csv_reader_unconverted_columns_stay_strings(tmp_path):
    p = tmp_path / "mixed.csv"
    _write_csv(p, [["name", "score", "city"], ["Ada", "95", "London"]])
    with TypedCSVReader(p, schema={"score": int}) as reader:
        rows = list(reader)
    # name и city — не в schema — остаются строками
    assert isinstance(rows[0]["name"], str)
    assert isinstance(rows[0]["city"], str)
    assert rows[0]["name"] == "Ada"
    assert rows[0]["city"] == "London"


def test_typed_csv_reader_key_order_matches_header(tmp_path):
    p = tmp_path / "order.csv"
    _write_csv(p, [["z", "a", "m"], ["1", "2", "3"]])
    with TypedCSVReader(p, schema={}) as reader:
        rows = list(reader)
    assert list(rows[0].keys()) == ["z", "a", "m"]


# ---------------------------------------------------------------------------
# Краевые случаи: пустой файл, только заголовок
# ---------------------------------------------------------------------------

def test_typed_csv_reader_empty_file_gives_no_rows(tmp_path):
    p = tmp_path / "empty.csv"
    p.write_text("", encoding="utf-8")
    with TypedCSVReader(p, schema={}) as reader:
        rows = list(reader)
    assert rows == []


def test_typed_csv_reader_header_only_gives_no_rows(tmp_path):
    p = tmp_path / "header.csv"
    p.write_text("name,age,city", encoding="utf-8")
    with TypedCSVReader(p, schema={}) as reader:
        rows = list(reader)
    assert rows == []


def test_typed_csv_reader_single_row(tmp_path):
    p = tmp_path / "one.csv"
    _write_csv(p, [["x"], ["42"]])
    with TypedCSVReader(p, schema={"x": int}) as reader:
        rows = list(reader)
    assert rows == [{"x": 42}]


# ---------------------------------------------------------------------------
# Контекстный менеджер: файл не открывается до __enter__
# ---------------------------------------------------------------------------

def test_typed_csv_reader_file_not_opened_before_enter(tmp_path):
    p = tmp_path / "lazy.csv"
    _write_csv(p, [["a"], ["1"]])
    reader = TypedCSVReader(p, schema={})
    # До with — файл НЕ открыт; итерация должна поднять ValueError (закрыт / не открыт)
    with pytest.raises((ValueError, AttributeError)):
        list(reader)


def test_typed_csv_reader_returns_self_from_enter(tmp_path):
    p = tmp_path / "self.csv"
    _write_csv(p, [["a"], ["1"]])
    reader = TypedCSVReader(p, schema={})
    result = reader.__enter__()
    reader.__exit__(None, None, None)
    assert result is reader


def test_typed_csv_reader_file_closed_after_exit(tmp_path):
    p = tmp_path / "close.csv"
    _write_csv(p, [["a"], ["1"]])
    with TypedCSVReader(p, schema={}) as reader:
        pass  # просто войти и выйти
    # После выхода повторная итерация должна поднять ValueError
    with pytest.raises(ValueError):
        list(reader)


def test_typed_csv_reader_exit_closes_on_exception(tmp_path):
    """Файл закрывается даже если внутри блока выброшено исключение."""
    p = tmp_path / "exc.csv"
    _write_csv(p, [["a"], ["1"]])
    reader = TypedCSVReader(p, schema={})
    try:
        with reader:
            raise RuntimeError("внутри блока что-то пошло не так")
    except RuntimeError:
        pass
    # Файл должен быть закрыт
    with pytest.raises(ValueError):
        list(reader)


# ---------------------------------------------------------------------------
# Ошибочные схемы
# ---------------------------------------------------------------------------

def test_typed_csv_reader_raises_key_error_on_missing_schema_column(tmp_path):
    p = tmp_path / "miss.csv"
    _write_csv(p, [["name", "age"], ["Ada", "36"]])
    with pytest.raises(KeyError):
        with TypedCSVReader(p, schema={"nonexistent": int}) as reader:
            list(reader)


def test_typed_csv_reader_raises_key_error_shows_column_name(tmp_path):
    p = tmp_path / "miss2.csv"
    _write_csv(p, [["a", "b"], ["1", "2"]])
    with pytest.raises(KeyError) as exc_info:
        with TypedCSVReader(p, schema={"c": int}) as reader:
            list(reader)
    assert "c" in str(exc_info.value)


def test_typed_csv_reader_converter_value_error_propagates(tmp_path):
    p = tmp_path / "bad.csv"
    _write_csv(p, [["n"], ["not_a_number"]])
    with pytest.raises(ValueError):
        with TypedCSVReader(p, schema={"n": int}) as reader:
            list(reader)


def test_typed_csv_reader_nonexistent_file_raises_file_not_found(tmp_path):
    p = tmp_path / "ghost.csv"
    with pytest.raises(FileNotFoundError):
        with TypedCSVReader(p, schema={}) as reader:
            list(reader)


# ---------------------------------------------------------------------------
# Несколько колонок в schema, str-конвертер не меняет значение
# ---------------------------------------------------------------------------

def test_typed_csv_reader_str_schema_is_identity(tmp_path):
    p = tmp_path / "str.csv"
    _write_csv(p, [["a", "b"], ["hello", "world"]])
    with TypedCSVReader(p, schema={"a": str, "b": str}) as reader:
        rows = list(reader)
    assert rows == [{"a": "hello", "b": "world"}]


def test_typed_csv_reader_multiple_schema_columns(tmp_path):
    p = tmp_path / "multi.csv"
    _write_csv(p, [["name", "score", "grade"], ["Ada", "95", "5"], ["Bjarne", "72", "4"]])
    with TypedCSVReader(p, schema={"score": int, "grade": int}) as reader:
        rows = list(reader)
    assert rows[0] == {"name": "Ada", "score": 95, "grade": 5}
    assert rows[1] == {"name": "Bjarne", "score": 72, "grade": 4}


# ---------------------------------------------------------------------------
# Использование строкового пути (не только Path)
# ---------------------------------------------------------------------------

def test_typed_csv_reader_accepts_str_path(tmp_path):
    p = tmp_path / "strpath.csv"
    _write_csv(p, [["x"], ["7"]])
    with TypedCSVReader(str(p), schema={"x": int}) as reader:
        rows = list(reader)
    assert rows == [{"x": 7}]
