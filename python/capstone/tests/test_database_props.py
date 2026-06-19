# Эти тесты трогать не нужно — это РАНДОМИЗИРОВАННЫЕ property-тесты движка (слой 3).
#
# test_database.py проверяет ФИКСИРОВАННЫЕ сессии. Здесь hypothesis генерирует случайные
# таблицы и строки и проверяет ИНВАРИАНТЫ СУБД через независимый «оракул» на чистом Python:
#   INSERT затем SELECT возвращает вставленные строки; WHERE-фильтр совпадает с предикатом-
#   оракулом; DELETE удаляет ровно подходящие строки; SELECT без совпадений пуст; порядок
#   операций там, где контракт это разрешает, не влияет на результат.
# Пока Database.execute/coerce в стабе — все тесты КРАСНЫЕ, это норма.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from db.database import Database, QueryError


# --- независимый оракул типизации и сравнения (мирно дублирует контракт) ------

def oracle_coerce(token: str):
    """То же, что coerce в database.py: целое -> int, иначе str. Независимая реализация."""
    try:
        return int(token)
    except ValueError:
        return token


OPS = {
    "=":  lambda a, b: a == b,
    "!=": lambda a, b: a != b,
    "<":  lambda a, b: a < b,
    ">":  lambda a, b: a > b,
    "<=": lambda a, b: a <= b,
    ">=": lambda a, b: a >= b,
}


# --- генераторы значений по столбцам -----------------------------------------
# Чтобы сравнения < > <= >= были осмысленными (int с int, str со str), договоримся:
# колонка "id" и "age" — ЧИСЛОВЫЕ, колонка "name" — СТРОКОВАЯ. Значения генерим в этих типах,
# но в SQL они всё равно идут как голые токены без кавычек.

int_token = st.integers(min_value=-50, max_value=50).map(str)
# короткие слова из букв (без кавычек, не похожие на число и не ключевые)
name_token = st.sampled_from(["Alice", "Bob", "Carol", "Dave", "Eve", "Mallory", "Zed"])

COLUMNS = ["id", "name", "age"]


@st.composite
def rows(draw, n=None):
    """Список строк (каждая — кортеж токенов id, name, age) для таблицы users."""
    if n is None:
        n = draw(st.integers(min_value=0, max_value=6))
    out = []
    for _ in range(n):
        out.append((draw(int_token), draw(name_token), draw(int_token)))
    return out


def make_db(rows):
    db = Database()
    db.execute("CREATE TABLE users (id, name, age)")
    for (i, nm, ag) in rows:
        db.execute(f"INSERT INTO users VALUES ({i}, {nm}, {ag})")
    return db


def expected_full_rows(rows):
    """Ожидаемый результат SELECT * (типизированные словари в порядке вставки)."""
    return [
        {"id": oracle_coerce(i), "name": oracle_coerce(nm), "age": oracle_coerce(ag)}
        for (i, nm, ag) in rows
    ]


# --- инвариант: INSERT затем SELECT * возвращает вставленное в порядке вставки -

@given(rs=rows())
@settings(max_examples=80, deadline=None)
def test_insert_then_select_star_roundtrip(rs):
    db = make_db(rs)
    assert db.execute("SELECT * FROM users") == expected_full_rows(rs)


@given(rs=rows())
@settings(max_examples=60, deadline=None)
def test_select_count_matches_inserted(rs):
    db = make_db(rs)
    assert len(db.execute("SELECT * FROM users")) == len(rs)


# --- инвариант: значения типизируются (число -> int, слово -> str) -----------

@given(i=int_token, nm=name_token, ag=int_token)
@settings(max_examples=80, deadline=None)
def test_value_types(i, nm, ag):
    db = make_db([(i, nm, ag)])
    row = db.execute("SELECT * FROM users")[0]
    assert isinstance(row["id"], int)
    assert isinstance(row["age"], int)
    assert isinstance(row["name"], str)
    assert row == {"id": int(i), "name": nm, "age": int(ag)}


# --- инвариант: WHERE-фильтр совпадает с предикатом-оракулом ------------------

@given(rs=rows(), col=st.sampled_from(["id", "age"]), op=st.sampled_from(list(OPS)),
       threshold=st.integers(min_value=-50, max_value=50))
@settings(max_examples=200, deadline=None)
def test_where_numeric_matches_oracle(rs, col, op, threshold):
    db = make_db(rs)
    got = db.execute(f"SELECT * FROM users WHERE {col} {op} {threshold}")
    want = [
        {"id": oracle_coerce(i), "name": oracle_coerce(nm), "age": oracle_coerce(ag)}
        for (i, nm, ag) in rs
        if OPS[op](oracle_coerce({"id": i, "age": ag}[col]), threshold)
    ]
    assert got == want


@given(rs=rows(), op=st.sampled_from(["=", "!="]), target=name_token)
@settings(max_examples=150, deadline=None)
def test_where_string_eq_matches_oracle(rs, op, target):
    db = make_db(rs)
    got = db.execute(f"SELECT name FROM users WHERE name {op} {target}")
    want = [
        {"name": nm}
        for (i, nm, ag) in rs
        if OPS[op](nm, target)
    ]
    assert got == want


# --- инвариант: WHERE ... AND ... — это логическое И двух предикатов ----------

@given(rs=rows(),
       op1=st.sampled_from(list(OPS)), t1=st.integers(min_value=-50, max_value=50),
       target=name_token)
@settings(max_examples=150, deadline=None)
def test_where_and_is_logical_and(rs, op1, t1, target):
    db = make_db(rs)
    got = db.execute(f"SELECT * FROM users WHERE age {op1} {t1} AND name = {target}")
    want = [
        {"id": oracle_coerce(i), "name": nm, "age": oracle_coerce(ag)}
        for (i, nm, ag) in rs
        if OPS[op1](oracle_coerce(ag), t1) and nm == target
    ]
    assert got == want


# --- инвариант: проекция оставляет ровно указанные колонки в их порядке -------

@given(rs=rows(),
       proj=st.lists(st.sampled_from(COLUMNS), min_size=1, max_size=3, unique=True))
@settings(max_examples=120, deadline=None)
def test_projection_keeps_named_columns_in_order(rs, proj):
    db = make_db(rs)
    cols_sql = ", ".join(proj)
    got = db.execute(f"SELECT {cols_sql} FROM users")
    full = expected_full_rows(rs)
    want = [{c: r[c] for c in proj} for r in full]
    assert got == want
    for r in got:
        assert list(r.keys()) == proj


# --- инвариант: ORDER BY сортирует, как sorted() оракула ----------------------

@given(rs=rows(), col=st.sampled_from(["id", "age"]), desc=st.booleans())
@settings(max_examples=150, deadline=None)
def test_order_by_numeric_matches_sorted(rs, col, desc):
    db = make_db(rs)
    direction = "DESC" if desc else "ASC"
    got = db.execute(f"SELECT {col} FROM users ORDER BY {col} {direction}")
    want_vals = sorted(
        (oracle_coerce({"id": i, "age": ag}[col]) for (i, nm, ag) in rs),
        reverse=desc,
    )
    assert [r[col] for r in got] == want_vals


# --- инвариант: DELETE удаляет ровно подходящие строки и возвращает их число --

@given(rs=rows(), col=st.sampled_from(["id", "age"]), op=st.sampled_from(list(OPS)),
       threshold=st.integers(min_value=-50, max_value=50))
@settings(max_examples=150, deadline=None)
def test_delete_removes_exactly_matching(rs, col, op, threshold):
    db = make_db(rs)
    matched = [
        (i, nm, ag) for (i, nm, ag) in rs
        if OPS[op](oracle_coerce({"id": i, "age": ag}[col]), threshold)
    ]
    n = db.execute(f"DELETE FROM users WHERE {col} {op} {threshold}")
    assert n == len(matched)
    # оставшиеся = те, что НЕ подошли (в исходном порядке; строки могут дублироваться,
    # поэтому считаем выживших построчно, а не множеством).
    keep = []
    for (i, nm, ag) in rs:
        if not OPS[op](oracle_coerce({"id": i, "age": ag}[col]), threshold):
            keep.append({"id": oracle_coerce(i), "name": nm, "age": oracle_coerce(ag)})
    assert db.execute("SELECT * FROM users") == keep


@given(rs=rows())
@settings(max_examples=60, deadline=None)
def test_delete_all_empties_and_counts(rs):
    db = make_db(rs)
    assert db.execute("DELETE FROM users") == len(rs)
    assert db.execute("SELECT * FROM users") == []


# --- инвариант: SELECT без совпадений — пустой список -------------------------

@given(rs=rows())
@settings(max_examples=60, deadline=None)
def test_no_match_is_empty(rs):
    db = make_db(rs)
    # age строго больше любого возможного значения (<= 50) -> совпадений нет.
    assert db.execute("SELECT * FROM users WHERE age > 1000") == []


@given(rs=rows())
@settings(max_examples=60, deadline=None)
def test_all_match_returns_everything(rs):
    db = make_db(rs)
    # age >= -1000 проходят все строки -> результат равен SELECT *.
    assert db.execute("SELECT * FROM users WHERE age >= -1000") == expected_full_rows(rs)


# --- семантические ошибки -> QueryError ---------------------------------------

@given(table=st.from_regex(r"[a-z]{1,6}", fullmatch=True))
@settings(max_examples=40, deadline=None)
def test_unknown_table_select_raises(table):
    db = Database()  # пустая база, никаких таблиц
    with pytest.raises(QueryError):
        db.execute(f"SELECT * FROM {table}")


@given(table=st.from_regex(r"[a-z]{1,6}", fullmatch=True))
@settings(max_examples=40, deadline=None)
def test_unknown_table_insert_raises(table):
    db = Database()
    with pytest.raises(QueryError):
        db.execute(f"INSERT INTO {table} VALUES (1)")


def test_duplicate_create_raises():
    db = Database()
    db.execute("CREATE TABLE t (x)")
    with pytest.raises(QueryError):
        db.execute("CREATE TABLE t (x)")


@given(extra=st.integers(min_value=1, max_value=3))
@settings(max_examples=30, deadline=None)
def test_wrong_arity_insert_raises(extra):
    db = Database()
    db.execute("CREATE TABLE t (a, b)")
    vals = ", ".join(str(k) for k in range(2 + extra))  # слишком много значений
    with pytest.raises(QueryError):
        db.execute(f"INSERT INTO t VALUES ({vals})")


def test_unknown_column_select_raises():
    db = Database()
    db.execute("CREATE TABLE t (a)")
    db.execute("INSERT INTO t VALUES (1)")
    with pytest.raises(QueryError):
        db.execute("SELECT nope FROM t")


# --- инвариант: два экземпляра Database независимы ----------------------------

@given(rs=rows())
@settings(max_examples=30, deadline=None)
def test_two_databases_independent(rs):
    a = make_db(rs)
    b = Database()
    with pytest.raises(QueryError):
        b.execute("SELECT * FROM users")
    # a не пострадала
    assert a.execute("SELECT * FROM users") == expected_full_rows(rs)


# --- явный краевой случай: дублирующиеся строки ------------------------------

def test_duplicate_rows_all_returned():
    db = Database()
    db.execute("CREATE TABLE t (x)")
    db.execute("INSERT INTO t VALUES (5)")
    db.execute("INSERT INTO t VALUES (5)")
    assert db.execute("SELECT * FROM t") == [{"x": 5}, {"x": 5}]
    assert db.execute("DELETE FROM t WHERE x = 5") == 2


def test_empty_table_select_and_delete():
    db = Database()
    db.execute("CREATE TABLE t (x)")
    assert db.execute("SELECT * FROM t") == []
    assert db.execute("DELETE FROM t") == 0
    assert db.execute("DELETE FROM t WHERE x = 1") == 0
