# Эти тесты трогать не нужно — это ЭТАЛОН поведения парсера (слой 2).
#
# Парсер собирает из токенов dataclass-объект запроса (Create/Insert/Select/Delete) и
# поднимает QueryError на синтаксический мусор. Сравнение идёт через == (dataclass даёт
# __eq__ бесплатно). Пока parse() в стабе — все тесты КРАСНЫЕ, это ожидаемо.
#
# Удобный приём: подаём в parse() готовый список токенов напрямую (не гоняем строку через
# tokenize), чтобы тестировать парсер ИЗОЛИРОВАННО от лексера. Детерминированно, без сети.

import pytest

from db.parser import parse, Create, Insert, Select, Delete, Condition
from db.database import QueryError


# --- CREATE ----------------------------------------------------------------

def test_parse_create():
    tokens = ["CREATE", "TABLE", "users", "(", "id", ",", "name", ",", "age", ")"]
    assert parse(tokens) == Create(table="users", columns=["id", "name", "age"])


def test_parse_create_single_column():
    tokens = ["CREATE", "TABLE", "t", "(", "x", ")"]
    assert parse(tokens) == Create(table="t", columns=["x"])


def test_parse_create_keyword_case_insensitive():
    tokens = ["create", "table", "users", "(", "id", ")"]
    assert parse(tokens) == Create(table="users", columns=["id"])


# --- INSERT ----------------------------------------------------------------

def test_parse_insert():
    tokens = ["INSERT", "INTO", "users", "VALUES", "(", "1", ",", "Alice", ",", "30", ")"]
    assert parse(tokens) == Insert(table="users", values=["1", "Alice", "30"])


# --- SELECT: проекция ------------------------------------------------------

def test_parse_select_star():
    assert parse(["SELECT", "*", "FROM", "users"]) == Select(table="users", columns=["*"])


def test_parse_select_columns():
    tokens = ["SELECT", "name", ",", "age", "FROM", "users"]
    assert parse(tokens) == Select(table="users", columns=["name", "age"])


# --- SELECT: WHERE ---------------------------------------------------------

def test_parse_select_where_one_condition():
    tokens = ["SELECT", "name", ",", "age", "FROM", "users", "WHERE", "age", ">", "25"]
    assert parse(tokens) == Select(
        table="users",
        columns=["name", "age"],
        where=[Condition("age", ">", "25")],
    )


def test_parse_select_where_and():
    tokens = [
        "SELECT", "*", "FROM", "users",
        "WHERE", "age", ">=", "18", "AND", "name", "=", "Bob",
    ]
    assert parse(tokens) == Select(
        table="users",
        columns=["*"],
        where=[Condition("age", ">=", "18"), Condition("name", "=", "Bob")],
    )


def test_parse_select_no_where_has_empty_conditions():
    result = parse(["SELECT", "*", "FROM", "users"])
    assert result.where == []


# --- SELECT: ORDER BY ------------------------------------------------------

def test_parse_select_order_by_default_asc():
    tokens = ["SELECT", "*", "FROM", "users", "ORDER", "BY", "age"]
    assert parse(tokens) == Select(
        table="users", columns=["*"], order_by="age", descending=False,
    )


def test_parse_select_order_by_desc():
    tokens = ["SELECT", "*", "FROM", "users", "ORDER", "BY", "age", "DESC"]
    assert parse(tokens) == Select(
        table="users", columns=["*"], order_by="age", descending=True,
    )


def test_parse_select_where_and_order_by():
    tokens = [
        "SELECT", "name", "FROM", "users",
        "WHERE", "age", ">", "10", "ORDER", "BY", "age", "DESC",
    ]
    assert parse(tokens) == Select(
        table="users",
        columns=["name"],
        where=[Condition("age", ">", "10")],
        order_by="age",
        descending=True,
    )


# --- DELETE ----------------------------------------------------------------

def test_parse_delete_all():
    assert parse(["DELETE", "FROM", "users"]) == Delete(table="users", where=[])


def test_parse_delete_where():
    tokens = ["DELETE", "FROM", "users", "WHERE", "id", "=", "1"]
    assert parse(tokens) == Delete(table="users", where=[Condition("id", "=", "1")])


# --- синтаксические ошибки -> QueryError ------------------------------------

def test_parse_empty_raises():
    with pytest.raises(QueryError):
        parse([])


def test_parse_unknown_command_raises():
    with pytest.raises(QueryError):
        parse(["DROP", "TABLE", "users"])


def test_parse_create_missing_table_keyword_raises():
    with pytest.raises(QueryError):
        parse(["CREATE", "users", "(", "id", ")"])


def test_parse_create_unbalanced_parens_raises():
    with pytest.raises(QueryError):
        parse(["CREATE", "TABLE", "users", "(", "id", ",", "name"])


def test_parse_select_missing_from_raises():
    with pytest.raises(QueryError):
        parse(["SELECT", "*", "users"])


def test_parse_select_empty_columns_raises():
    with pytest.raises(QueryError):
        parse(["SELECT", "FROM", "users"])


def test_parse_where_incomplete_condition_raises():
    with pytest.raises(QueryError):
        parse(["SELECT", "*", "FROM", "users", "WHERE", "age", ">"])


def test_parse_order_by_missing_by_raises():
    with pytest.raises(QueryError):
        parse(["SELECT", "*", "FROM", "users", "ORDER", "age"])


def test_parse_trailing_garbage_raises():
    with pytest.raises(QueryError):
        parse(["SELECT", "*", "FROM", "users", "extra", "junk"])
