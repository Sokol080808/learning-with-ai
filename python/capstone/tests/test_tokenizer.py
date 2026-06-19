# Эти тесты трогать не нужно — это ЭТАЛОН поведения токенизатора (слой 1).
#
# Пока tokenize() в стабе кидает NotImplementedError — все они КРАСНЫЕ. Это норма:
# тесты задают цель, а реализуешь её ты. Тесты детерминированы: без сети, без рандома,
# без файлов — чистые функции от строки.

from db.tokenizer import tokenize


# --- базовая нарезка -------------------------------------------------------

def test_simple_select():
    assert tokenize("SELECT * FROM users") == ["SELECT", "*", "FROM", "users"]


def test_empty_string_is_no_tokens():
    assert tokenize("") == []


def test_only_spaces_is_no_tokens():
    assert tokenize("    \t  ") == []


def test_keeps_word_case():
    # Лексер НЕ приводит регистр: и ключевые слова, и значения остаются как есть.
    assert tokenize("Select Name") == ["Select", "Name"]


def test_extra_whitespace_collapses():
    assert tokenize("  SELECT    *   FROM     users  ") == ["SELECT", "*", "FROM", "users"]


def test_numbers_are_word_tokens():
    assert tokenize("INSERT INTO users VALUES 1 Alice 30")[-3:] == ["1", "Alice", "30"]


# --- пунктуаторы: скобки и запятые отдельными токенами ---------------------

def test_parens_and_commas_are_separate_tokens():
    assert tokenize("(1, Alice, 30)") == ["(", "1", ",", "Alice", ",", "30", ")"]


def test_parens_glued_to_words_still_split():
    assert tokenize("(id,name,age)") == ["(", "id", ",", "name", ",", "age", ")"]


def test_create_table_full():
    assert tokenize("CREATE TABLE users (id, name, age)") == [
        "CREATE", "TABLE", "users", "(", "id", ",", "name", ",", "age", ")",
    ]


def test_insert_full():
    assert tokenize("INSERT INTO users VALUES (1, Alice, 30)") == [
        "INSERT", "INTO", "users", "VALUES", "(", "1", ",", "Alice", ",", "30", ")",
    ]


# --- операторы сравнения ---------------------------------------------------

def test_single_char_operators():
    assert tokenize("age > 25") == ["age", ">", "25"]
    assert tokenize("age < 25") == ["age", "<", "25"]
    assert tokenize("name = Bob") == ["name", "=", "Bob"]


def test_two_char_operators_not_split():
    assert tokenize("age >= 18") == ["age", ">=", "18"]
    assert tokenize("age <= 18") == ["age", "<=", "18"]
    assert tokenize("id != 1") == ["id", "!=", "1"]


def test_operator_glued_to_operands():
    # Нет пробелов вокруг оператора — результат тот же, что и с пробелами.
    assert tokenize("age>=18") == ["age", ">=", "18"]
    assert tokenize("id!=1") == ["id", "!=", "1"]


def test_where_with_and():
    assert tokenize("SELECT * FROM users WHERE age >= 18 AND name = Bob") == [
        "SELECT", "*", "FROM", "users", "WHERE",
        "age", ">=", "18", "AND", "name", "=", "Bob",
    ]


def test_order_by_desc():
    assert tokenize("SELECT * FROM users ORDER BY age DESC") == [
        "SELECT", "*", "FROM", "users", "ORDER", "BY", "age", "DESC",
    ]


def test_returns_list_of_strings():
    out = tokenize("DELETE FROM users WHERE id = 1")
    assert isinstance(out, list)
    assert all(isinstance(t, str) for t in out)
