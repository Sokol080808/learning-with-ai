# Эти тесты трогать не нужно — это эталон поведения.
# Модуль 12 — Файлы и данные (текст, JSON, CSV).
# Файловые операции идут через фикстуру tmp_path — временную папку pytest.

import csv
import json

from datafiles import (
    load_json,
    parse_csv,
    read_csv_typed,
    read_lines,
    save_json,
    write_csv,
    write_lines,
)


# --- Задание 1: write_lines ---

def test_write_lines_writes_each_line_with_newline(tmp_path):
    p = tmp_path / "notes.txt"
    write_lines(p, ["alpha", "beta", "gamma"])
    # каждая строка — на своей строке, с завершающим \n
    assert p.read_text(encoding="utf-8") == "alpha\nbeta\ngamma\n"


def test_write_lines_empty_list_makes_empty_file(tmp_path):
    p = tmp_path / "empty.txt"
    write_lines(p, [])
    assert p.read_text(encoding="utf-8") == ""


def test_write_lines_overwrites_existing(tmp_path):
    p = tmp_path / "over.txt"
    p.write_text("старое содержимое\n", encoding="utf-8")
    write_lines(p, ["new"])
    # режим "w" обнуляет файл, старого текста быть не должно
    assert p.read_text(encoding="utf-8") == "new\n"


def test_write_lines_accepts_str_path(tmp_path):
    # path может прийти и строкой, не только Path
    p = tmp_path / "asstr.txt"
    write_lines(str(p), ["one", "two"])
    assert p.read_text(encoding="utf-8") == "one\ntwo\n"


# --- Задание 2: read_lines ---

def test_read_lines_strips_trailing_newline(tmp_path):
    p = tmp_path / "lines.txt"
    p.write_text("a\nb\nc\n", encoding="utf-8")
    assert read_lines(p) == ["a", "b", "c"]


def test_read_lines_empty_file_returns_empty_list(tmp_path):
    p = tmp_path / "empty.txt"
    p.write_text("", encoding="utf-8")
    assert read_lines(p) == []


def test_read_lines_keeps_inner_spaces(tmp_path):
    # снимаем только '\n', значимые пробелы внутри/по краям строк не трогаем
    p = tmp_path / "spaces.txt"
    p.write_text("  hello world  \n  bye  \n", encoding="utf-8")
    assert read_lines(p) == ["  hello world  ", "  bye  "]


def test_write_then_read_roundtrip(tmp_path):
    # read_lines отменяет write_lines
    p = tmp_path / "round.txt"
    lines = ["первая", "вторая", "третья"]
    write_lines(p, lines)
    assert read_lines(p) == lines


# --- Задания 3 и 4: JSON в файл и из файла ---

def test_save_json_writes_valid_json(tmp_path):
    p = tmp_path / "data.json"
    obj = {"name": "Ada", "age": 36}
    save_json(p, obj)
    # файл должен содержать корректный JSON, читаемый штатным модулем
    assert json.loads(p.read_text(encoding="utf-8")) == obj


def test_load_json_reads_object(tmp_path):
    p = tmp_path / "in.json"
    p.write_text('{"x": 1, "y": [2, 3]}', encoding="utf-8")
    assert load_json(p) == {"x": 1, "y": [2, 3]}


def test_json_roundtrip_dict(tmp_path):
    p = tmp_path / "rt.json"
    obj = {"name": "Ada", "age": 36, "active": True, "tags": ["py", "cpp"], "note": None}
    save_json(p, obj)
    assert load_json(p) == obj


def test_json_roundtrip_list(tmp_path):
    p = tmp_path / "rtlist.json"
    obj = [1, 2, 3, "four", False]
    save_json(p, obj)
    assert load_json(p) == obj


def test_json_roundtrip_nested(tmp_path):
    p = tmp_path / "nested.json"
    obj = {"users": [{"id": 1, "name": "Ada"}, {"id": 2, "name": "Bjarne"}], "count": 2}
    save_json(p, obj)
    assert load_json(p) == obj


# --- Задание 5: parse_csv ---

def test_parse_csv_basic():
    text = "name,age,city\nAda,36,London\nBjarne,70,Aarhus"
    assert parse_csv(text) == [
        {"name": "Ada", "age": "36", "city": "London"},
        {"name": "Bjarne", "age": "70", "city": "Aarhus"},
    ]


def test_parse_csv_values_stay_strings():
    # CSV не знает типов: возраст остаётся строкой "36", не числом 36
    rows = parse_csv("name,age\nAda,36")
    assert rows == [{"name": "Ada", "age": "36"}]
    assert rows[0]["age"] == "36"


def test_parse_csv_single_data_row():
    assert parse_csv("a,b,c\n1,2,3") == [{"a": "1", "b": "2", "c": "3"}]


def test_parse_csv_header_only_returns_empty():
    assert parse_csv("name,age,city") == []


def test_parse_csv_empty_text_returns_empty():
    assert parse_csv("") == []


def test_parse_csv_trailing_newline_ignored():
    # хвостовой перевод строки не создаёт лишней пустой строки данных
    text = "x,y\n1,2\n"
    assert parse_csv(text) == [{"x": "1", "y": "2"}]


def test_parse_csv_single_column():
    text = "word\nhello\nworld"
    assert parse_csv(text) == [{"word": "hello"}, {"word": "world"}]


# --- Задание 6: write_csv и read_csv_typed через стандартный модуль csv ---

def test_write_csv_creates_file_with_header(tmp_path):
    p = tmp_path / "out.csv"
    rows = [{"name": "Ada", "age": "36"}, {"name": "Bjarne", "age": "70"}]
    write_csv(p, rows, fieldnames=["name", "age"])
    # Читаем обратно через стандартный csv.DictReader — должны получить те же значения
    with open(p, newline="", encoding="utf-8") as f:
        result = list(csv.DictReader(f))
    assert [dict(r) for r in result] == rows


def test_write_csv_comma_in_cell_is_quoted(tmp_path):
    # Ячейка "London, UK" содержит запятую — csv.DictWriter обязана взять её в кавычки.
    # Ручной split(",") прочитал бы её неправильно (два поля вместо одного).
    p = tmp_path / "cities.csv"
    rows = [{"name": "Ada", "city": "London, UK"}]
    write_csv(p, rows, fieldnames=["name", "city"])
    raw = p.read_text(encoding="utf-8")
    # В сыром тексте должны быть кавычки вокруг значения с запятой
    assert '"London, UK"' in raw or '"London, UK"' in raw


def test_read_csv_typed_basic(tmp_path):
    p = tmp_path / "data.csv"
    p.write_text("name,score\nAda,95\nBjarne,72\n", encoding="utf-8")
    rows = read_csv_typed(p, schema={"score": int})
    assert rows == [{"name": "Ada", "score": 95}, {"name": "Bjarne", "score": 72}]
    assert isinstance(rows[0]["score"], int)


def test_read_csv_typed_no_schema_all_strings(tmp_path):
    p = tmp_path / "plain.csv"
    p.write_text("a,b\n1,2\n", encoding="utf-8")
    rows = read_csv_typed(p, schema={})
    assert rows == [{"a": "1", "b": "2"}]
    assert isinstance(rows[0]["a"], str)


def test_read_csv_typed_comma_in_cell(tmp_path):
    # Ключевая проверка: ячейка "London, UK" — ручной split(",") ломается,
    # csv.DictReader читает правильно.
    p = tmp_path / "cities.csv"
    # Записываем через write_csv, чтобы кавычки появились корректно
    write_csv(p, [{"name": "Ada", "city": "London, UK"}], fieldnames=["name", "city"])
    rows = read_csv_typed(p, schema={})
    assert rows == [{"name": "Ada", "city": "London, UK"}]
    # Убедимся, что значение — цельное, без разрезания по запятой
    assert rows[0]["city"] == "London, UK"


def test_write_then_read_csv_roundtrip(tmp_path):
    # round-trip: write_csv → read_csv_typed должен вернуть исходные данные
    p = tmp_path / "rt.csv"
    original = [
        {"name": "Ada", "score": "95", "city": "London"},
        {"name": "Bjarne", "score": "72", "city": "Aarhus"},
    ]
    write_csv(p, original, fieldnames=["name", "score", "city"])
    back = read_csv_typed(p, schema={})
    assert back == original


def test_write_then_read_csv_roundtrip_with_comma_in_cell(tmp_path):
    # round-trip с ячейкой, содержащей запятую
    p = tmp_path / "rt_comma.csv"
    original = [{"name": "Ada", "city": "London, UK"}, {"name": "Bjarne", "city": "Aarhus"}]
    write_csv(p, original, fieldnames=["name", "city"])
    back = read_csv_typed(p, schema={})
    assert back == original


def test_read_csv_typed_empty_file_returns_empty(tmp_path):
    p = tmp_path / "empty.csv"
    p.write_text("", encoding="utf-8")
    assert read_csv_typed(p, schema={}) == []


def test_read_csv_typed_header_only_returns_empty(tmp_path):
    p = tmp_path / "hdr.csv"
    p.write_text("name,age\n", encoding="utf-8")
    assert read_csv_typed(p, schema={}) == []


def test_read_csv_typed_float_converter(tmp_path):
    p = tmp_path / "floats.csv"
    p.write_text("x,y\n1.5,2.7\n", encoding="utf-8")
    rows = read_csv_typed(p, schema={"x": float, "y": float})
    assert rows[0] == {"x": 1.5, "y": 2.7}


def test_write_csv_accepts_str_path(tmp_path):
    p = tmp_path / "strpath.csv"
    write_csv(str(p), [{"k": "v"}], fieldnames=["k"])
    rows = read_csv_typed(str(p), schema={})
    assert rows == [{"k": "v"}]
