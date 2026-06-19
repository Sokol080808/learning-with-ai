# Property-тесты для TypedCSVReader — hypothesis генерирует случайные CSV-файлы
# и проверяет инварианты, а не конкретные ответы. Файлы живут в tmp_path.

from pathlib import Path

import pytest
from hypothesis import HealthCheck, given, settings
from hypothesis import strategies as st

from datafiles import TypedCSVReader

# ---------------------------------------------------------------------------
# Стратегии: CSV-безопасный текст (без запятых, без символов перевода строки).
# Повторяем ограничения из test_datafiles_props.py для единообразия.
# ---------------------------------------------------------------------------

_LINE_BOUNDARY = set("\n\r\v\f\x1c\x1d\x1e\x85  ,")
csv_safe_char = st.characters(
    blacklist_categories=("Cs",),
).filter(lambda ch: ch not in _LINE_BOUNDARY and ch == "".join(ch.splitlines()))
cell_text = st.text(alphabet=csv_safe_char, min_size=1, max_size=15)

# Список уникальных имён колонок (заголовок).
header_st = st.lists(cell_text, min_size=1, max_size=5, unique=True)


def _write_csv_file(path: Path, header: list[str], rows: list[list[str]]) -> None:
    lines = [",".join(header)] + [",".join(r) for r in rows]
    path.write_text("\n".join(lines), encoding="utf-8")


# ---------------------------------------------------------------------------
# Инвариант 1: без schema значения совпадают с исходными строками
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=60,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_no_schema_matches_raw_strings(tmp_path, header, data):
    n_rows = data.draw(st.integers(min_value=0, max_value=8))
    rows = [
        data.draw(st.lists(cell_text, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    p = tmp_path / "raw.csv"
    _write_csv_file(p, header, rows)

    with TypedCSVReader(p, schema={}) as reader:
        result = list(reader)

    assert len(result) == n_rows
    for got, original in zip(result, rows):
        assert got == dict(zip(header, original))
        assert all(isinstance(v, str) for v in got.values())


# ---------------------------------------------------------------------------
# Инвариант 2: ключи каждой строки совпадают с заголовком (порядок сохраняется)
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=60,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_keys_match_header(tmp_path, header, data):
    n_rows = data.draw(st.integers(min_value=0, max_value=6))
    rows = [
        data.draw(st.lists(cell_text, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    p = tmp_path / "keys.csv"
    _write_csv_file(p, header, rows)

    with TypedCSVReader(p, schema={}) as reader:
        result = list(reader)

    for row in result:
        assert list(row.keys()) == header


# ---------------------------------------------------------------------------
# Инвариант 3: str-конвертер в schema — тождественный, значения не меняются
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_str_schema_is_identity(tmp_path, header, data):
    n_rows = data.draw(st.integers(min_value=1, max_value=6))
    rows = [
        data.draw(st.lists(cell_text, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    p = tmp_path / "strschema.csv"
    _write_csv_file(p, header, rows)

    schema = {col: str for col in header}
    with TypedCSVReader(p, schema=schema) as reader:
        with_schema = list(reader)
    with TypedCSVReader(p, schema={}) as reader:
        without_schema = list(reader)

    assert with_schema == without_schema


# ---------------------------------------------------------------------------
# Инвариант 4: int-конвертер на числовых данных даёт int-значения
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_int_schema_gives_int_values(tmp_path, header, data):
    n_rows = data.draw(st.integers(min_value=1, max_value=6))
    # Числа от 0 до 9999 — гарантированно конвертируются в int без ошибок
    int_cell = st.integers(min_value=0, max_value=9999).map(str)
    rows = [
        data.draw(st.lists(int_cell, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    p = tmp_path / "ints.csv"
    _write_csv_file(p, header, rows)

    # Применяем int-конвертер ко всем колонкам
    schema = {col: int for col in header}
    with TypedCSVReader(p, schema=schema) as reader:
        result = list(reader)

    assert len(result) == n_rows
    for row in result:
        assert all(isinstance(v, int) for v in row.values())


# ---------------------------------------------------------------------------
# Инвариант 5: количество строк всегда равно числу строк данных (не заголовок)
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=60,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_row_count(tmp_path, header, data):
    n_rows = data.draw(st.integers(min_value=0, max_value=10))
    rows = [
        data.draw(st.lists(cell_text, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    p = tmp_path / "count.csv"
    _write_csv_file(p, header, rows)

    with TypedCSVReader(p, schema={}) as reader:
        result = list(reader)

    assert len(result) == n_rows


# ---------------------------------------------------------------------------
# Инвариант 6: только schema-колонки приведены; остальные — строки
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_only_schema_cols_converted(tmp_path, header, data):
    if len(header) < 2:
        return  # нужно хотя бы две колонки: одна в schema, другая нет
    int_cell = st.integers(min_value=0, max_value=999).map(str)
    n_rows = data.draw(st.integers(min_value=1, max_value=5))
    rows = [
        data.draw(st.lists(int_cell, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    p = tmp_path / "partial.csv"
    _write_csv_file(p, header, rows)

    # Только первая колонка в schema
    converted_col = header[0]
    rest = header[1:]
    with TypedCSVReader(p, schema={converted_col: int}) as reader:
        result = list(reader)

    for row in result:
        assert isinstance(row[converted_col], int)
        for col in rest:
            assert isinstance(row[col], str)


# ---------------------------------------------------------------------------
# Инвариант 7: schema с несуществующей колонкой → KeyError при входе в with
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=30,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_unknown_schema_col_raises(tmp_path, header, data):
    p = tmp_path / "err.csv"
    _write_csv_file(p, header, [])
    # Добавляем к имени последней колонки суффикс '_NOSUCHCOL', которого заведомо нет
    bad_col = header[-1] + "_NOSUCHCOL"
    with pytest.raises(KeyError):
        with TypedCSVReader(p, schema={bad_col: int}) as reader:
            list(reader)


# ---------------------------------------------------------------------------
# Инвариант 8: после __exit__ итерация поднимает ValueError (файл закрыт)
# ---------------------------------------------------------------------------

@given(
    header=header_st,
    data=st.data(),
)
@settings(
    max_examples=30,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_typed_csv_reader_iter_after_exit_raises(tmp_path, header, data):
    p = tmp_path / "closed.csv"
    _write_csv_file(p, header, [])

    reader = TypedCSVReader(p, schema={})
    with reader:
        pass  # вышли из контекста — файл закрыт

    with pytest.raises(ValueError):
        list(reader)
