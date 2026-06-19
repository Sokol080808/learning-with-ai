# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_datafiles.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — РАНДОМИЗИРОВАННЫЕ
# property-тесты: hypothesis сам подбирает сотни входов (включая злые — пустые файлы,
# пробелы, юникод, вложенные структуры) и проверяет не конкретный ответ, а ИНВАРИАНТЫ —
# свойства, которые обязаны выполняться для ЛЮБОГО входа. Главный инвариант сериализации:
# read(write(x)) == x и load(save(x)) == x (туда-обратно без потерь).

import json

from hypothesis import HealthCheck, given, settings
from hypothesis import strategies as st

from datafiles import (
    load_json,
    parse_csv,
    read_lines,
    save_json,
    write_lines,
)

# Строки файла НЕ содержат '\n' (его добавляет write_lines). '\r' тоже исключаем,
# чтобы не зависеть от платформенной нормализации переводов строк при чтении.
# Суррогаты (категория Cs, напр. '\ud800') исключаем: их в принципе нельзя записать
# в utf-8 файл — это ограничение кодировки, а не дефект реализации.
line_text = st.text(
    alphabet=st.characters(blacklist_characters="\n\r", blacklist_categories=("Cs",)),
    min_size=0,
    max_size=40,
)
lines_lists = st.lists(line_text, max_size=20)

# JSON-совместимые значения: скаляры + рекурсивно списки/словари (ключи — строки).
# Текст без суррогатов — иначе его нельзя записать в utf-8 файл (ограничение кодировки).
json_text = st.text(alphabet=st.characters(blacklist_categories=("Cs",)), max_size=30)
json_scalars = st.one_of(
    st.none(),
    st.booleans(),
    st.integers(min_value=-(10**9), max_value=10**9),
    st.floats(allow_nan=False, allow_infinity=False, min_value=-1e9, max_value=1e9),
    json_text,
)
json_values = st.recursive(
    json_scalars,
    lambda children: st.one_of(
        st.lists(children, max_size=5),
        st.dictionaries(
            st.text(alphabet=st.characters(blacklist_categories=("Cs",)), max_size=10),
            children,
            max_size=5,
        ),
    ),
    max_leaves=20,
)

# CSV-безопасный текст ячейки: без запятой (разделитель) и без ЛЮБЫХ символов,
# которые str.splitlines() считает границей строки (их больше, чем \n и \r:
# \v \f \x1c \x1d \x1e \x85     ...). Контракт CSV запрещает в ячейке
# запятые и переводы строк, поэтому такие символы в данные не попадают.
_LINE_BOUNDARY = set("\n\r\v\f\x1c\x1d\x1e\x85  ,")
csv_safe_char = st.characters(
    blacklist_categories=("Cs",),
).filter(lambda ch: ch not in _LINE_BOUNDARY and ch == "".join(ch.splitlines()))
cell_text = st.text(alphabet=csv_safe_char, min_size=1, max_size=15)


# --- write_lines / read_lines: главный инвариант — round-trip ---

@given(lines=lines_lists)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_write_then_read_roundtrips(tmp_path, lines):
    # read_lines обязана в точности отменять write_lines для любого списка строк.
    p = tmp_path / "rt.txt"
    write_lines(p, lines)
    assert read_lines(p) == lines


@given(lines=lines_lists)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_write_lines_file_format(tmp_path, lines):
    # Каждая строка — на своей строке, с завершающим '\n'; пустой список — пустой файл.
    p = tmp_path / "fmt.txt"
    write_lines(p, lines)
    content = p.read_text(encoding="utf-8")
    if lines:
        assert content == "".join(s + "\n" for s in lines)
        assert content.count("\n") == len(lines)
    else:
        assert content == ""


@given(first=lines_lists, second=lines_lists)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_write_lines_overwrites_not_appends(tmp_path, first, second):
    # Режим "w" обнуляет файл: после второй записи видно только второй список.
    p = tmp_path / "over.txt"
    write_lines(p, first)
    write_lines(p, second)
    assert read_lines(p) == second


@given(lines=lines_lists)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_write_lines_accepts_str_path(tmp_path, lines):
    # path может прийти строкой, не только Path — результат тот же.
    p = tmp_path / "asstr.txt"
    write_lines(str(p), lines)
    assert read_lines(str(p)) == lines


def test_read_lines_empty_file_is_empty_list(tmp_path):
    # Краевой случай: пустой файл -> [], а НЕ [""].
    p = tmp_path / "empty.txt"
    p.write_text("", encoding="utf-8")
    assert read_lines(p) == []


def test_write_lines_preserves_unicode_and_spaces(tmp_path):
    # Юникод и значимые пробелы по краям не теряются и не обрезаются.
    p = tmp_path / "uni.txt"
    lines = ["  привет мир  ", "日本語", "  ", "café"]
    write_lines(p, lines)
    assert read_lines(p) == lines


# --- save_json / load_json: главный инвариант — round-trip ---

@given(obj=json_values)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_json_roundtrips(tmp_path, obj):
    # load_json обязана отменять save_json для любого JSON-совместимого объекта.
    p = tmp_path / "rt.json"
    save_json(p, obj)
    assert load_json(p) == obj


@given(obj=json_values)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_save_json_produces_parseable_json(tmp_path, obj):
    # Файл — валидный JSON, читаемый штатным модулем json, и совпадает с исходным объектом.
    p = tmp_path / "valid.json"
    save_json(p, obj)
    assert json.loads(p.read_text(encoding="utf-8")) == obj


@given(obj=json_values)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_load_json_matches_stdlib_dump(tmp_path, obj):
    # load_json читает то же, что записал бы штатный json.dump.
    p = tmp_path / "std.json"
    p.write_text(json.dumps(obj), encoding="utf-8")
    assert load_json(p) == obj


def test_json_roundtrip_unicode_keys_and_values(tmp_path):
    # Юникод в ключах и значениях переживает round-trip (ensure_ascii не должен ломать смысл).
    p = tmp_path / "uni.json"
    obj = {"имя": "Ада", "город": "Лондон", "теги": ["питон", "café"]}
    save_json(p, obj)
    assert load_json(p) == obj


# --- parse_csv: разбор CSV в список словарей ---

@given(
    header=st.lists(cell_text, min_size=1, max_size=5, unique=True),
    data=st.data(),
)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_parse_csv_reconstructs_rows(header, data):
    # Собираем CSV из случайных строк-ячеек и проверяем, что parse_csv возвращает
    # ровно те же значения, разложенные по именам колонок.
    n_rows = data.draw(st.integers(min_value=0, max_value=6))
    rows = [
        data.draw(st.lists(cell_text, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    text = "\n".join([",".join(header)] + [",".join(r) for r in rows])
    result = parse_csv(text)

    assert len(result) == n_rows
    for got, original in zip(result, rows):
        assert got == dict(zip(header, original))
        # Все значения остаются строками — CSV не знает типов.
        assert all(isinstance(v, str) for v in got.values())
        # Ключи ровно те, что в заголовке.
        assert list(got.keys()) == header


@given(header=st.lists(cell_text, min_size=1, max_size=5, unique=True))
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_parse_csv_header_only_is_empty(header):
    # Только заголовок без данных -> [].
    assert parse_csv(",".join(header)) == []


def test_parse_csv_empty_text_is_empty():
    # Краевой случай: пустой текст -> [].
    assert parse_csv("") == []


@given(
    header=st.lists(cell_text, min_size=1, max_size=4, unique=True),
    data=st.data(),
)
@settings(
    max_examples=50,
    deadline=None,
    suppress_health_check=[HealthCheck.function_scoped_fixture],
)
def test_parse_csv_trailing_newline_ignored(header, data):
    # Хвостовой '\n' не создаёт лишней пустой строки данных.
    n_rows = data.draw(st.integers(min_value=1, max_value=4))
    rows = [
        data.draw(st.lists(cell_text, min_size=len(header), max_size=len(header)))
        for _ in range(n_rows)
    ]
    body = "\n".join([",".join(header)] + [",".join(r) for r in rows])
    assert parse_csv(body + "\n") == parse_csv(body)
    assert len(parse_csv(body + "\n")) == n_rows
