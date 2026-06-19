# Эти тесты трогать не нужно — это РАНДОМИЗИРОВАННЫЕ property-тесты токенизатора (слой 1).
#
# test_tokenizer.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — hypothesis сам подбирает сотни
# входов и проверяет не конкретный ответ, а ИНВАРИАНТЫ: какие токены обязаны получиться для
# ЛЮБОГО сгенерированного запроса, нечувствительность к пробелам, отделение пунктуаторов.
# Пока tokenize() в стабе кидает NotImplementedError — все они КРАСНЫЕ, это норма.

from hypothesis import given, settings
from hypothesis import strategies as st

from db.tokenizer import tokenize


# --- маленькие стратегии, ГЕНЕРИРУЮЩИЕ валидные куски SQL ---------------------

# «слово»: буквы/цифры/подчёркивания, начинается с буквы (как имена таблиц/колонок).
words = st.from_regex(r"[A-Za-z][A-Za-z0-9_]*", fullmatch=True)
# неотрицательное целое как слово-токен.
numbers = st.integers(min_value=0, max_value=10**9).map(str)
operators = st.sampled_from(["=", "!=", "<", ">", "<=", ">="])
word_or_number = st.one_of(words, numbers)


# --- инвариант: пробелы вокруг токенов роли не играют ------------------------

@given(ws=st.lists(word_or_number, min_size=0, max_size=8))
@settings(max_examples=100, deadline=None)
def test_words_separated_by_spaces_become_those_tokens(ws):
    # Слова/числа, разделённые пробелами, токенизируются ровно в самих себя.
    sql = " ".join(ws)
    assert tokenize(sql) == ws


@given(ws=st.lists(words, min_size=1, max_size=6),
       pad=st.integers(min_value=1, max_value=5))
@settings(max_examples=100, deadline=None)
def test_extra_whitespace_is_irrelevant(ws, pad):
    # Любое количество пробелов/табов между словами даёт тот же результат, что и один пробел.
    sep = " " * pad
    spaced = (sep + "\t").join(ws)
    assert tokenize(spaced) == tokenize(" ".join(ws)) == ws


@given(ws=st.lists(word_or_number, min_size=0, max_size=6))
@settings(max_examples=80, deadline=None)
def test_leading_and_trailing_whitespace_stripped(ws):
    sql = "   " + "  ".join(ws) + "   "
    assert tokenize(sql) == ws


# --- инвариант: пунктуаторы — отдельные токены, даже прилипшие ----------------

@given(vals=st.lists(word_or_number, min_size=1, max_size=6))
@settings(max_examples=100, deadline=None)
def test_parens_and_commas_split_even_when_glued(vals):
    # "(a,b,c)" без пробелов -> ["(", a, ",", b, ",", c, ")"].
    inner = ",".join(vals)
    glued = "(" + inner + ")"
    expected = ["("]
    for i, v in enumerate(vals):
        if i > 0:
            expected.append(",")
        expected.append(v)
    expected.append(")")
    assert tokenize(glued) == expected


@given(left=words, op=operators, right=word_or_number,
       lpad=st.integers(min_value=0, max_value=3),
       rpad=st.integers(min_value=0, max_value=3))
@settings(max_examples=150, deadline=None)
def test_operator_three_tokens_regardless_of_spacing(left, op, right, lpad, rpad):
    # "col>=18", "col >= 18", "col>=  18" — все дают ровно [col, op, right].
    sql = left + (" " * lpad) + op + (" " * rpad) + right
    assert tokenize(sql) == [left, op, right]


@given(op=operators)
@settings(max_examples=20, deadline=None)
def test_two_char_operators_never_split(op):
    # Двухсимвольные операторы не рвутся на два токена.
    out = tokenize("a " + op + " b")
    assert out == ["a", op, "b"]
    if len(op) == 2:
        assert op in out
        assert op[0] not in out  # одиночный '!' / '<' / '>' как отдельный токен не появляется
        # (для '!=' '!' сам по себе вообще невалиден, для '<=' одиночный '<' не должен отколоться)


# --- инвариант: лексер НЕ трогает регистр и содержимое слов ------------------

@given(ws=st.lists(words, min_size=1, max_size=6))
@settings(max_examples=80, deadline=None)
def test_case_is_preserved(ws):
    sql = " ".join(ws)
    out = tokenize(sql)
    assert out == ws  # ни одного приведения регистра


@given(n=st.integers(min_value=-10**9, max_value=10**9))
@settings(max_examples=80, deadline=None)
def test_star_is_a_word_token(n):
    # '*' — это слово-токен "*"; он не пунктуатор и не рвётся.
    assert tokenize("SELECT * FROM t") == ["SELECT", "*", "FROM", "t"]


# --- общий инвариант формы результата ----------------------------------------

@given(ws=st.lists(st.one_of(word_or_number, operators, st.sampled_from(["(", ")", ","])),
                   min_size=0, max_size=10))
@settings(max_examples=120, deadline=None)
def test_always_list_of_nonempty_strings(ws):
    sql = " ".join(ws)
    out = tokenize(sql)
    assert isinstance(out, list)
    assert all(isinstance(t, str) and t != "" for t in out)


# --- явные краевые случаи ----------------------------------------------------

def test_empty_and_whitespace_only_give_empty_list():
    assert tokenize("") == []
    assert tokenize("   \t \n ") == []


def test_full_insert_statement_shape():
    assert tokenize("INSERT INTO users VALUES (1, Alice, 30)") == [
        "INSERT", "INTO", "users", "VALUES", "(", "1", ",", "Alice", ",", "30", ")",
    ]
