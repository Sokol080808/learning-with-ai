# Эти тесты трогать не нужно — это эталон поведения.
# Модуль 12 — Файлы и данные (текст, JSON, CSV).
# Файловые операции идут через фикстуру tmp_path — временную папку pytest.

import json

from datafiles import (
    load_json,
    parse_csv,
    read_lines,
    save_json,
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
