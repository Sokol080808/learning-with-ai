# Эти тесты трогать не нужно — это ЭТАЛОН поведения двух необязательных, но проверяемых
# майлстоунов мини-СУБД: ПЕРСИСТЕНТНОСТЬ (save/load через JSON) и ТРАНЗАКЦИИ (контекст-
# менеджер с откатом). Они связывают капстоун с модулем 12 (файлы/JSON) и модулем 08
# (контекст-менеджеры) — теми «выходными» навыками, которых раньше в проекте не хватало.
#
# Стиль здесь как у остальных property-тестов капстоуна: hypothesis генерирует случайные,
# но валидные данные, а инвариант проверяется независимо от внутреннего устройства движка.
# Новые рандомизированные тесты используют derandomize=True — один и тот же набор примеров
# при каждом запуске (воспроизводимость: упавший пример не «уплывёт» между прогонами).
# Пока Database.save/load/transaction в стабе (ветка main) — эти тесты КРАСНЫЕ. На ветке
# reference (эталон) — зелёные.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from db.database import Database, QueryError


# --- генератор случайной осмысленной базы ------------------------------------
# Та же договорённость, что и в test_database_props.py: id/age — числовые колонки,
# name — строковая. Значения идут в SQL голыми токенами без кавычек.

int_token = st.integers(min_value=-50, max_value=50).map(str)
name_token = st.sampled_from(["Alice", "Bob", "Carol", "Dave", "Eve", "Mallory", "Zed"])


@st.composite
def rows(draw, max_rows=6):
    """Список строк (кортежи токенов id, name, age) для таблицы users."""
    n = draw(st.integers(min_value=0, max_value=max_rows))
    return [
        (draw(int_token), draw(name_token), draw(int_token))
        for _ in range(n)
    ]


def make_db(rs):
    db = Database()
    db.execute("CREATE TABLE users (id, name, age)")
    for (i, nm, ag) in rs:
        db.execute(f"INSERT INTO users VALUES ({i}, {nm}, {ag})")
    return db


# --- ПЕРСИСТЕНТНОСТЬ: round-trip load(save(db)) == db -------------------------

@given(rs=rows())
@settings(max_examples=120, deadline=None, derandomize=True)
def test_save_load_roundtrip_equal(rs, tmp_path):
    """Главный инвариант персистентности: сохранили базу — прочитали обратно — равная."""
    db = make_db(rs)
    path = tmp_path / "db.json"
    db.save(path)
    restored = Database.load(path)
    assert restored == db


@given(rs=rows())
@settings(max_examples=80, deadline=None, derandomize=True)
def test_loaded_db_returns_same_rows(rs, tmp_path):
    """После load та же выборка SELECT * даёт те же строки в том же порядке."""
    db = make_db(rs)
    path = tmp_path / "db.json"
    db.save(path)
    restored = Database.load(path)
    assert restored.execute("SELECT * FROM users") == db.execute("SELECT * FROM users")


@given(rs=rows())
@settings(max_examples=60, deadline=None, derandomize=True)
def test_loaded_db_is_live_and_independent(rs, tmp_path):
    """Загруженная база — полноценная: в неё можно писать, и это не трогает исходную."""
    db = make_db(rs)
    path = db.save(tmp_path / "db.json")  # save возвращает Path — удобно для цепочки
    restored = Database.load(path)
    restored.execute("INSERT INTO users VALUES (999, Trent, 99)")
    assert restored.execute("SELECT * FROM users") != db.execute("SELECT * FROM users")


def test_save_creates_file_and_uses_json(tmp_path):
    """Формат на диске — валидный JSON (его можно прочитать сторонним json.load)."""
    import json

    db = Database()
    db.execute("CREATE TABLE t (x, name)")
    db.execute("INSERT INTO t VALUES (1, Alice)")
    path = db.save(tmp_path / "out.json")
    assert path.exists()
    data = json.loads(path.read_text(encoding="utf-8"))
    assert data["t"]["columns"] == ["x", "name"]
    assert data["t"]["rows"] == [{"x": 1, "name": "Alice"}]


def test_empty_db_roundtrips(tmp_path):
    """Пустая база (без таблиц) тоже сохраняется и читается без сюрпризов."""
    db = Database()
    path = db.save(tmp_path / "empty.json")
    assert Database.load(path) == db


# --- ТРАНЗАКЦИИ: откат состояния при исключении ------------------------------

def test_transaction_rolls_back_on_exception():
    """Исключение внутри блока with откатывает ВСЕ изменения, сделанные в блоке."""
    db = make_db([("1", "Alice", "30")])
    before = db.execute("SELECT * FROM users")

    with pytest.raises(QueryError):
        with db.transaction():
            db.execute("INSERT INTO users VALUES (2, Bob, 20)")  # будет откачено
            db.execute("DELETE FROM users WHERE id = 1")          # тоже откачено
            db.execute("SELECT nope FROM users")  # QueryError -> откат всей пачки

    assert db.execute("SELECT * FROM users") == before


def test_transaction_commits_on_success():
    """Без исключения изменения внутри блока остаются (фиксация)."""
    db = make_db([("1", "Alice", "30")])
    with db.transaction():
        db.execute("INSERT INTO users VALUES (2, Bob, 20)")
    assert db.execute("SELECT * FROM users") == [
        {"id": 1, "name": "Alice", "age": 30},
        {"id": 2, "name": "Bob", "age": 20},
    ]


def test_transaction_reraises_the_exception():
    """Транзакция НЕ глотает ошибку: тот же exception виден снаружи."""
    db = Database()
    db.execute("CREATE TABLE t (x)")

    class Boom(RuntimeError):
        pass

    with pytest.raises(Boom):
        with db.transaction():
            db.execute("INSERT INTO t VALUES (1)")
            raise Boom("прерывание посреди пачки")
    # пользовательское исключение (не QueryError) тоже откатывает состояние
    assert db.execute("SELECT * FROM t") == []


@given(rs=rows(), extra=rows(max_rows=4))
@settings(max_examples=80, deadline=None, derandomize=True)
def test_transaction_rollback_restores_exact_state(rs, extra):
    """Property: после отката база побайтово та же, что была до входа в транзакцию."""
    db = make_db(rs)
    snapshot = db.execute("SELECT * FROM users")

    with pytest.raises(QueryError):
        with db.transaction():
            for (i, nm, ag) in extra:
                db.execute(f"INSERT INTO users VALUES ({i}, {nm}, {ag})")
            db.execute("DELETE FROM users")          # стираем всё...
            db.execute("INSERT INTO ghosts VALUES (1)")  # ...и роняем транзакцию

    assert db.execute("SELECT * FROM users") == snapshot


@given(rs=rows(), extra=rows(max_rows=4))
@settings(max_examples=80, deadline=None, derandomize=True)
def test_transaction_commit_keeps_all_inserts(rs, extra):
    """Property: при успешном выходе все вставки пачки видны в базе."""
    db = make_db(rs)
    with db.transaction():
        for (i, nm, ag) in extra:
            db.execute(f"INSERT INTO users VALUES ({i}, {nm}, {ag})")
    assert len(db.execute("SELECT * FROM users")) == len(rs) + len(extra)


def test_save_after_rollback_persists_pre_transaction_state(tmp_path):
    """Связка майлстоунов: откат + сохранение даёт «дотранзакционный» снимок на диске."""
    db = make_db([("1", "Alice", "30")])
    with pytest.raises(QueryError):
        with db.transaction():
            db.execute("INSERT INTO users VALUES (2, Bob, 20)")
            db.execute("DELETE FROM ghosts")  # откат
    path = db.save(tmp_path / "after.json")
    assert Database.load(path).execute("SELECT * FROM users") == [
        {"id": 1, "name": "Alice", "age": 30}
    ]
