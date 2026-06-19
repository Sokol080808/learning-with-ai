# Эти тесты трогать не нужно — это РАНДОМИЗИРОВАННЫЕ property-тесты парсера (слой 2).
#
# test_parser.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь hypothesis сам собирает случайные
# (но валидные по грамматике) AST-объекты, «печатает» их обратно в список токенов и проверяет
# главный инвариант: parse(render(ast)) == ast (round-trip). Отдельно проверяем, что мусор
# отвергается QueryError. Пока parse() в стабе — все тесты КРАСНЫЕ, это ожидаемо.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from db.parser import parse, Create, Insert, Select, Delete, Condition
from db.database import QueryError


# --- кирпичики: имена/значения/операторы -------------------------------------

# Имена колонок/таблиц: начинаются с буквы, без подчёркивания первым символом — чтобы
# случайно не совпасть с ключевым словом по форме (ключевые слова сравниваются в верхнем
# регистре, поэтому имя "select" опасно — исключаем такие именами в нижнем регистре).
KEYWORDS = {
    "CREATE", "TABLE", "INSERT", "INTO", "VALUES", "SELECT", "FROM",
    "WHERE", "AND", "ORDER", "BY", "ASC", "DESC", "DELETE",
}

identifiers = st.from_regex(r"[a-z][a-z0-9]*", fullmatch=True).filter(
    lambda s: s.upper() not in KEYWORDS
)
# значения: целые (как строки) либо слова — парсер хранит value как строку-токен.
value_tokens = st.one_of(
    st.integers(min_value=-10**6, max_value=10**6).map(str),
    st.from_regex(r"[A-Za-z][A-Za-z0-9]*", fullmatch=True).filter(
        lambda s: s.upper() not in KEYWORDS
    ),
)
operators = st.sampled_from(["=", "!=", "<", ">", "<=", ">="])


@st.composite
def conditions(draw):
    return Condition(draw(identifiers), draw(operators), draw(value_tokens))


def render_conditions(conds: list[Condition]) -> list[str]:
    out: list[str] = []
    for i, c in enumerate(conds):
        if i > 0:
            out.append("AND")
        out += [c.column, c.op, c.value]
    return out


# --- CREATE round-trip -------------------------------------------------------

@st.composite
def creates(draw):
    table = draw(identifiers)
    cols = draw(st.lists(identifiers, min_size=1, max_size=6))
    return Create(table=table, columns=cols)


def render_create(c: Create) -> list[str]:
    out = ["CREATE", "TABLE", c.table, "("]
    for i, col in enumerate(c.columns):
        if i > 0:
            out.append(",")
        out.append(col)
    out.append(")")
    return out


@given(c=creates())
@settings(max_examples=100, deadline=None)
def test_create_roundtrip(c):
    assert parse(render_create(c)) == c


# --- INSERT round-trip -------------------------------------------------------

@st.composite
def inserts(draw):
    table = draw(identifiers)
    vals = draw(st.lists(value_tokens, min_size=1, max_size=6))
    return Insert(table=table, values=vals)


def render_insert(ins: Insert) -> list[str]:
    out = ["INSERT", "INTO", ins.table, "VALUES", "("]
    for i, v in enumerate(ins.values):
        if i > 0:
            out.append(",")
        out.append(v)
    out.append(")")
    return out


@given(ins=inserts())
@settings(max_examples=100, deadline=None)
def test_insert_roundtrip(ins):
    assert parse(render_insert(ins)) == ins


# --- SELECT round-trip (проекция + WHERE + ORDER BY) -------------------------

@st.composite
def selects(draw):
    table = draw(identifiers)
    star = draw(st.booleans())
    if star:
        columns = ["*"]
    else:
        columns = draw(st.lists(identifiers, min_size=1, max_size=5))
    where = draw(st.lists(conditions(), min_size=0, max_size=3))
    has_order = draw(st.booleans())
    if has_order:
        order_by = draw(identifiers)
        descending = draw(st.booleans())
    else:
        order_by = None
        descending = False
    return Select(table=table, columns=columns, where=where,
                  order_by=order_by, descending=descending)


def render_select(s: Select, *, explicit_asc: bool = False) -> list[str]:
    out = ["SELECT"]
    for i, col in enumerate(s.columns):
        if i > 0:
            out.append(",")
        out.append(col)
    out += ["FROM", s.table]
    if s.where:
        out.append("WHERE")
        out += render_conditions(s.where)
    if s.order_by is not None:
        out += ["ORDER", "BY", s.order_by]
        if s.descending:
            out.append("DESC")
        elif explicit_asc:
            out.append("ASC")
    return out


@given(s=selects())
@settings(max_examples=200, deadline=None)
def test_select_roundtrip(s):
    assert parse(render_select(s)) == s


@given(s=selects())
@settings(max_examples=100, deadline=None)
def test_select_explicit_asc_equals_default(s):
    # Явный ASC должен дать тот же AST, что и без ASC (descending=False).
    if s.descending:
        return  # ASC имеет смысл только для возрастания
    assert parse(render_select(s, explicit_asc=True)) == s


# --- DELETE round-trip -------------------------------------------------------

@st.composite
def deletes(draw):
    table = draw(identifiers)
    where = draw(st.lists(conditions(), min_size=0, max_size=3))
    return Delete(table=table, where=where)


def render_delete(d: Delete) -> list[str]:
    out = ["DELETE", "FROM", d.table]
    if d.where:
        out.append("WHERE")
        out += render_conditions(d.where)
    return out


@given(d=deletes())
@settings(max_examples=100, deadline=None)
def test_delete_roundtrip(d):
    assert parse(render_delete(d)) == d


# --- инвариант: регистр ключевых слов не важен -------------------------------

@given(s=selects())
@settings(max_examples=100, deadline=None)
def test_keyword_case_insensitive(s):
    tokens = render_select(s)
    lowered = [t.lower() if t.upper() in KEYWORDS else t for t in tokens]
    assert parse(lowered) == s


# --- инвариант: пустой WHERE даёт пустой список условий -----------------------

@given(table=identifiers)
@settings(max_examples=50, deadline=None)
def test_no_where_means_empty_conditions(table):
    assert parse(["SELECT", "*", "FROM", table]).where == []
    assert parse(["DELETE", "FROM", table]).where == []


# --- отрицательные инварианты: синтаксический мусор -> QueryError -------------

@given(extra=st.lists(identifiers, min_size=1, max_size=3))
@settings(max_examples=80, deadline=None)
def test_trailing_garbage_rejected(extra):
    with pytest.raises(QueryError):
        parse(["SELECT", "*", "FROM", "users"] + extra)


@given(first=identifiers)
@settings(max_examples=80, deadline=None)
def test_unknown_command_rejected(first):
    # Первое слово, не являющееся одной из четырёх команд -> ошибка.
    assert first.upper() not in {"CREATE", "INSERT", "SELECT", "DELETE"}
    with pytest.raises(QueryError):
        parse([first, "FROM", "users"])


@given(c=conditions())
@settings(max_examples=80, deadline=None)
def test_incomplete_condition_rejected(c):
    # Условие без правой части -> ошибка.
    with pytest.raises(QueryError):
        parse(["SELECT", "*", "FROM", "t", "WHERE", c.column, c.op])


@given(ins=inserts())
@settings(max_examples=60, deadline=None)
def test_insert_missing_values_keyword_rejected(ins):
    # Убираем ключевое слово VALUES -> синтаксическая ошибка.
    bad = ["INSERT", "INTO", ins.table, "("]
    for i, v in enumerate(ins.values):
        if i > 0:
            bad.append(",")
        bad.append(v)
    bad.append(")")
    with pytest.raises(QueryError):
        parse(bad)


def test_empty_token_list_rejected():
    with pytest.raises(QueryError):
        parse([])
