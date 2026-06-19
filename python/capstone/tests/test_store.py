# Эти тесты трогать не нужно — это эталон поведения (приёмочные тесты капстоуна).
# Капстоун — ядро: класс TaskStore (хранилище, выборки, персистентность JSON).
# Запуск:  ./python/run.sh        (капстоун подхватывается вместе с модулями)
#    или:  ./python/.venv/bin/pytest python/capstone

import json

import pytest

from tasks.store import TaskStore


# --- Пустое хранилище --------------------------------------------------------

def test_new_store_is_empty():
    store = TaskStore()
    assert store.count() == 0
    assert store.all() == []
    assert store.pending() == []


# --- Добавление и возвращаемый id --------------------------------------------

def test_add_returns_int_id():
    store = TaskStore()
    new_id = store.add("купить молоко")
    assert isinstance(new_id, int)


def test_add_first_id_is_one():
    store = TaskStore()
    assert store.add("первая") == 1


def test_add_ids_are_increasing():
    store = TaskStore()
    assert store.add("a") == 1
    assert store.add("b") == 2
    assert store.add("c") == 3


def test_add_increments_count():
    store = TaskStore()
    store.add("a")
    store.add("b")
    assert store.count() == 2


def test_added_task_is_a_dict_with_expected_fields():
    store = TaskStore()
    tid = store.add("написать резюме")
    task = store.all()[0]
    assert task == {"id": tid, "title": "написать резюме", "done": False}


def test_add_keeps_title_verbatim():
    # Заголовок сохраняется как есть: регистр и пробелы не трогаем.
    store = TaskStore()
    store.add("  Купить  Молоко  ")
    assert store.all()[0]["title"] == "  Купить  Молоко  "


# --- all() сортирует по id ---------------------------------------------------

def test_all_sorted_by_id():
    store = TaskStore()
    store.add("a")
    store.add("b")
    store.add("c")
    ids = [t["id"] for t in store.all()]
    assert ids == [1, 2, 3]


def test_all_returns_all_tasks():
    store = TaskStore()
    store.add("a")
    store.add("b")
    assert len(store.all()) == 2


# --- complete() --------------------------------------------------------------

def test_complete_sets_done_true():
    store = TaskStore()
    tid = store.add("сделать")
    assert store.complete(tid) is True
    assert store.all()[0]["done"] is True


def test_complete_missing_returns_false():
    store = TaskStore()
    store.add("одна")
    assert store.complete(999) is False


def test_complete_twice_is_still_true():
    store = TaskStore()
    tid = store.add("дважды")
    assert store.complete(tid) is True
    assert store.complete(tid) is True
    assert store.all()[0]["done"] is True


def test_complete_only_targets_one_task():
    store = TaskStore()
    a = store.add("a")
    b = store.add("b")
    store.complete(a)
    by_id = {t["id"]: t["done"] for t in store.all()}
    assert by_id[a] is True
    assert by_id[b] is False


# --- remove() ----------------------------------------------------------------

def test_remove_existing_returns_true():
    store = TaskStore()
    tid = store.add("удалить")
    assert store.remove(tid) is True
    assert store.count() == 0


def test_remove_missing_returns_false():
    store = TaskStore()
    store.add("есть")
    assert store.remove(999) is False
    assert store.count() == 1


def test_remove_keeps_other_tasks():
    store = TaskStore()
    a = store.add("a")
    b = store.add("b")
    c = store.add("c")
    store.remove(b)
    ids = [t["id"] for t in store.all()]
    assert ids == [a, c]  # == [1, 3]


# --- id не переиспользуются ---------------------------------------------------

def test_ids_not_reused_after_remove():
    # Удалили задачу 2 — следующая всё равно получает БОЛЬШИЙ id, а не освободившийся.
    store = TaskStore()
    store.add("a")          # 1
    store.add("b")          # 2
    store.remove(2)
    assert store.add("c") == 3


def test_ids_not_reused_even_when_store_emptied():
    store = TaskStore()
    a = store.add("a")      # 1
    store.remove(a)
    assert store.count() == 0
    assert store.add("b") == 2   # счётчик не откатывается к 1


# --- pending() ---------------------------------------------------------------

def test_pending_excludes_done():
    store = TaskStore()
    store.add("a")           # 1
    b = store.add("b")       # 2
    store.add("c")           # 3
    store.complete(b)
    ids = [t["id"] for t in store.pending()]
    assert ids == [1, 3]


def test_pending_all_when_none_done():
    store = TaskStore()
    store.add("a")
    store.add("b")
    assert len(store.pending()) == 2


def test_pending_empty_when_all_done():
    store = TaskStore()
    a = store.add("a")
    b = store.add("b")
    store.complete(a)
    store.complete(b)
    assert store.pending() == []


def test_pending_sorted_by_id():
    store = TaskStore()
    store.add("a")
    store.add("b")
    store.add("c")
    ids = [t["id"] for t in store.pending()]
    assert ids == [1, 2, 3]


# --- Защита внутреннего состояния --------------------------------------------

def test_all_result_mutation_does_not_corrupt_store():
    # Менять полученный список можно без вреда для хранилища.
    store = TaskStore()
    store.add("a")
    store.add("b")
    snapshot = store.all()
    snapshot.clear()
    assert store.count() == 2  # хранилище не пострадало


# --- Персистентность JSON (save/load через tmp_path) -------------------------

def test_save_creates_file(tmp_path):
    store = TaskStore()
    store.add("a")
    path = tmp_path / "todo.json"
    store.save(path)
    assert path.exists()


def test_save_writes_valid_json(tmp_path):
    store = TaskStore()
    store.add("a")
    path = tmp_path / "todo.json"
    store.save(path)
    # Файл должен парситься как JSON (а не быть, например, repr() словаря).
    with open(path, "r", encoding="utf-8") as f:
        json.load(f)  # не должно бросить


def test_save_then_load_roundtrip_tasks(tmp_path):
    store = TaskStore()
    store.add("купить молоко")     # 1
    store.add("написать резюме")   # 2
    store.complete(2)
    path = tmp_path / "todo.json"
    store.save(path)

    other = TaskStore()
    other.load(path)
    assert other.all() == store.all()


def test_load_replaces_existing_content(tmp_path):
    # load() ЗАМЕНЯЕТ содержимое, а не дописывает к нему.
    src = TaskStore()
    src.add("из файла")            # 1
    path = tmp_path / "todo.json"
    src.save(path)

    store = TaskStore()
    store.add("свежая 1")
    store.add("свежая 2")
    store.load(path)
    titles = [t["title"] for t in store.all()]
    assert titles == ["из файла"]
    assert store.count() == 1


def test_load_restores_id_counter(tmp_path):
    # После load() новый add() продолжает нумерацию, а не пересекается с загруженными id.
    src = TaskStore()
    src.add("a")                   # 1
    src.add("b")                   # 2
    path = tmp_path / "todo.json"
    src.save(path)

    store = TaskStore()
    store.load(path)
    assert store.add("c") == 3     # не 1 и не 2 — счётчик восстановлен


def test_load_counter_survives_removed_tasks(tmp_path):
    # Удалили задачу перед сохранением — счётчик всё равно не откатывается после load().
    src = TaskStore()
    src.add("a")                   # 1
    src.add("b")                   # 2
    src.add("c")                   # 3
    src.remove(3)
    path = tmp_path / "todo.json"
    src.save(path)

    store = TaskStore()
    store.load(path)
    assert store.add("d") == 4     # следующий id — 4, а не 3


def test_save_load_empty_store(tmp_path):
    store = TaskStore()
    path = tmp_path / "empty.json"
    store.save(path)

    other = TaskStore()
    other.add("должна исчезнуть")
    other.load(path)
    assert other.count() == 0
    assert other.all() == []


def test_save_accepts_str_path(tmp_path):
    # Путь может прийти строкой, не только Path.
    store = TaskStore()
    store.add("a")
    path = str(tmp_path / "todo.json")
    store.save(path)
    other = TaskStore()
    other.load(path)
    assert other.all() == store.all()
