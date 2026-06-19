# Эти тесты трогать не нужно — это ЭТАЛОН поведения движка (слой 3) и всей мини-СУБД.
#
# Каждый тест берёт СВЕЖУЮ базу (фикстура db) — состояние не течёт между тестами, поэтому
# порядок и повтор прогона не влияют на результат (детерминизм). Сети/файлов/рандома нет.
# Пока Database.execute / coerce в стабе — все тесты КРАСНЫЕ; зеленеют по мере реализации.

import pytest

from db.database import Database, QueryError
from db import Database as DatabaseFromPackage  # проверяем, что реэкспорт пакета работает


@pytest.fixture
def db() -> Database:
    """Чистая база на каждый тест."""
    return Database()


# --- упаковка пакета -------------------------------------------------------

def test_package_reexports_same_class():
    assert DatabaseFromPackage is Database


def test_query_error_is_exception():
    assert issubclass(QueryError, Exception)


# --- CREATE ----------------------------------------------------------------

def test_create_returns_none(db):
    assert db.execute("CREATE TABLE users (id, name, age)") is None


def test_create_then_select_empty(db):
    db.execute("CREATE TABLE users (id, name, age)")
    assert db.execute("SELECT * FROM users") == []


def test_create_duplicate_raises(db):
    db.execute("CREATE TABLE users (id, name, age)")
    with pytest.raises(QueryError):
        db.execute("CREATE TABLE users (id, name, age)")


# --- INSERT ----------------------------------------------------------------

def test_insert_returns_none(db):
    db.execute("CREATE TABLE users (id, name, age)")
    assert db.execute("INSERT INTO users VALUES (1, Alice, 30)") is None


def test_insert_into_unknown_table_raises(db):
    with pytest.raises(QueryError):
        db.execute("INSERT INTO ghosts VALUES (1, Alice, 30)")


def test_insert_wrong_number_of_values_raises(db):
    db.execute("CREATE TABLE users (id, name, age)")
    with pytest.raises(QueryError):
        db.execute("INSERT INTO users VALUES (1, Alice)")  # 2 значения на 3 колонки


def test_insert_too_many_values_raises(db):
    db.execute("CREATE TABLE users (id, name, age)")
    with pytest.raises(QueryError):
        db.execute("INSERT INTO users VALUES (1, Alice, 30, extra)")


# --- типы значений: числа -> int, слова -> str -----------------------------

def test_numbers_become_int_words_stay_str(db):
    db.execute("CREATE TABLE users (id, name, age)")
    db.execute("INSERT INTO users VALUES (1, Alice, 30)")
    row = db.execute("SELECT * FROM users")[0]
    assert row == {"id": 1, "name": "Alice", "age": 30}
    assert isinstance(row["id"], int)
    assert isinstance(row["age"], int)
    assert isinstance(row["name"], str)


def test_negative_number_is_int(db):
    db.execute("CREATE TABLE t (x)")
    db.execute("INSERT INTO t VALUES (-7)")
    assert db.execute("SELECT * FROM t") == [{"x": -7}]


# --- SELECT: строки как словари + проекция ---------------------------------

def test_select_star_returns_list_of_dicts(db):
    db.execute("CREATE TABLE users (id, name, age)")
    db.execute("INSERT INTO users VALUES (1, Alice, 30)")
    db.execute("INSERT INTO users VALUES (2, Bob, 20)")
    rows = db.execute("SELECT * FROM users")
    assert rows == [
        {"id": 1, "name": "Alice", "age": 30},
        {"id": 2, "name": "Bob", "age": 20},
    ]


def test_select_projection_keeps_only_named_columns(db):
    db.execute("CREATE TABLE users (id, name, age)")
    db.execute("INSERT INTO users VALUES (1, Alice, 30)")
    assert db.execute("SELECT name, age FROM users") == [{"name": "Alice", "age": 30}]


def test_select_projection_column_order(db):
    db.execute("CREATE TABLE users (id, name, age)")
    db.execute("INSERT INTO users VALUES (1, Alice, 30)")
    row = db.execute("SELECT age, name FROM users")[0]
    assert list(row.keys()) == ["age", "name"]


def test_select_unknown_table_raises(db):
    with pytest.raises(QueryError):
        db.execute("SELECT * FROM ghosts")


def test_select_unknown_column_raises(db):
    db.execute("CREATE TABLE users (id, name, age)")
    db.execute("INSERT INTO users VALUES (1, Alice, 30)")
    with pytest.raises(QueryError):
        db.execute("SELECT height FROM users")


# --- SELECT: WHERE ---------------------------------------------------------

@pytest.fixture
def people(db) -> Database:
    """База с тремя людьми — общая основа для WHERE/ORDER BY тестов."""
    db.execute("CREATE TABLE users (id, name, age)")
    db.execute("INSERT INTO users VALUES (1, Alice, 30)")
    db.execute("INSERT INTO users VALUES (2, Bob, 20)")
    db.execute("INSERT INTO users VALUES (3, Carol, 25)")
    return db


def test_where_greater_than(people):
    rows = people.execute("SELECT name FROM users WHERE age > 25")
    assert rows == [{"name": "Alice"}]


def test_where_greater_or_equal(people):
    rows = people.execute("SELECT name FROM users WHERE age >= 25")
    assert rows == [{"name": "Alice"}, {"name": "Carol"}]


def test_where_equals_word(people):
    rows = people.execute("SELECT id FROM users WHERE name = Bob")
    assert rows == [{"id": 2}]


def test_where_not_equals(people):
    rows = people.execute("SELECT name FROM users WHERE id != 2")
    assert rows == [{"name": "Alice"}, {"name": "Carol"}]


def test_where_less_than(people):
    rows = people.execute("SELECT name FROM users WHERE age < 25")
    assert rows == [{"name": "Bob"}]


def test_where_less_or_equal(people):
    rows = people.execute("SELECT name FROM users WHERE age <= 25")
    assert rows == [{"name": "Bob"}, {"name": "Carol"}]


def test_where_no_match_is_empty(people):
    assert people.execute("SELECT * FROM users WHERE age > 100") == []


def test_where_and_combines_with_logical_and(people):
    # age >= 18 проходят все трое; name = Bob отсекает до одного.
    rows = people.execute("SELECT * FROM users WHERE age >= 18 AND name = Bob")
    assert rows == [{"id": 2, "name": "Bob", "age": 20}]


def test_where_and_no_row_satisfies_both(people):
    rows = people.execute("SELECT * FROM users WHERE age > 25 AND name = Bob")
    assert rows == []


# --- SELECT: ORDER BY ------------------------------------------------------

def test_order_by_asc_default(people):
    names = [r["name"] for r in people.execute("SELECT name FROM users ORDER BY age")]
    assert names == ["Bob", "Carol", "Alice"]  # 20, 25, 30


def test_order_by_desc(people):
    names = [r["name"] for r in people.execute("SELECT name FROM users ORDER BY age DESC")]
    assert names == ["Alice", "Carol", "Bob"]  # 30, 25, 20


def test_order_by_with_where(people):
    rows = people.execute("SELECT name FROM users WHERE age >= 25 ORDER BY age DESC")
    assert rows == [{"name": "Alice"}, {"name": "Carol"}]


def test_order_by_string_column(people):
    names = [r["name"] for r in people.execute("SELECT name FROM users ORDER BY name DESC")]
    assert names == ["Carol", "Bob", "Alice"]


# --- DELETE ----------------------------------------------------------------

def test_delete_where_returns_count(people):
    assert people.execute("DELETE FROM users WHERE id = 1") == 1
    remaining = [r["name"] for r in people.execute("SELECT name FROM users")]
    assert remaining == ["Bob", "Carol"]


def test_delete_no_match_returns_zero(people):
    assert people.execute("DELETE FROM users WHERE id = 999") == 0


def test_delete_all_returns_count_and_empties(people):
    assert people.execute("DELETE FROM users") == 3
    assert people.execute("SELECT * FROM users") == []


def test_delete_with_and(people):
    assert people.execute("DELETE FROM users WHERE age >= 18 AND name = Bob") == 1
    remaining = [r["name"] for r in people.execute("SELECT name FROM users")]
    assert remaining == ["Alice", "Carol"]


def test_delete_unknown_table_raises(db):
    with pytest.raises(QueryError):
        db.execute("DELETE FROM ghosts")


# --- изоляция экземпляров (детерминизм) ------------------------------------

def test_two_databases_are_independent():
    a = Database()
    b = Database()
    a.execute("CREATE TABLE t (x)")
    a.execute("INSERT INTO t VALUES (1)")
    # b ничего не знает про таблицу t из a.
    with pytest.raises(QueryError):
        b.execute("SELECT * FROM t")
