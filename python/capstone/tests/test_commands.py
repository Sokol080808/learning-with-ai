# Эти тесты трогать не нужно — это эталон поведения (приёмочные тесты капстоуна).
# Капстоун — разбор команд: функция execute(store, line) -> строка-ответ.
# Запуск:  ./python/run.sh        (капстоун подхватывается вместе с модулями)
#    или:  ./python/.venv/bin/pytest python/capstone

import pytest

from tasks.store import TaskStore
from tasks.commands import execute


@pytest.fixture
def store():
    return TaskStore()


# --- ADD ---------------------------------------------------------------------

def test_add_returns_new_id_as_string(store):
    assert execute(store, "ADD купить молоко") == "1"
    assert execute(store, "ADD написать резюме") == "2"


def test_add_actually_stores_task(store):
    execute(store, "ADD задача")
    assert store.count() == 1
    assert store.all()[0]["title"] == "задача"


def test_add_command_name_is_case_insensitive(store):
    assert execute(store, "add нижний регистр") == "1"
    assert execute(store, "Add смешанный") == "2"


def test_add_keeps_title_with_spaces_and_case(store):
    execute(store, "ADD  Купить  Молоко  Срочно")
    # split(maxsplit=1) забирает заголовок «как остаток»: лидирующие пробелы между
    # командой и заголовком съедаются, но внутренние пробелы и регистр сохраняются.
    assert store.all()[0]["title"] == "Купить  Молоко  Срочно"


def test_add_without_title_is_error(store):
    assert execute(store, "ADD") == "ERR add requires an argument"
    assert store.count() == 0


def test_add_with_only_spaces_after_is_error(store):
    assert execute(store, "ADD    ") == "ERR add requires an argument"


# --- DONE --------------------------------------------------------------------

def test_done_marks_task(store):
    execute(store, "ADD задача")
    assert execute(store, "DONE 1") == "OK"
    assert store.all()[0]["done"] is True


def test_done_case_insensitive_command(store):
    execute(store, "ADD задача")
    assert execute(store, "done 1") == "OK"


def test_done_missing_task_is_error(store):
    execute(store, "ADD задача")
    assert execute(store, "DONE 999") == "ERR no such task: 999"


def test_done_non_number_is_error(store):
    assert execute(store, "DONE abc") == "ERR not a number: abc"


def test_done_without_argument_is_error(store):
    assert execute(store, "DONE") == "ERR done requires an argument"


# --- RM ----------------------------------------------------------------------

def test_rm_removes_task(store):
    execute(store, "ADD задача")
    assert execute(store, "RM 1") == "OK"
    assert store.count() == 0


def test_rm_missing_task_is_error(store):
    assert execute(store, "RM 5") == "ERR no such task: 5"


def test_rm_non_number_is_error(store):
    assert execute(store, "RM x1") == "ERR not a number: x1"


def test_rm_without_argument_is_error(store):
    assert execute(store, "RM") == "ERR rm requires an argument"


# --- LIST --------------------------------------------------------------------

def test_list_empty(store):
    assert execute(store, "LIST") == "(нет задач)"


def test_list_single_pending(store):
    execute(store, "ADD купить молоко")
    assert execute(store, "LIST") == "[ ] 1 купить молоко"


def test_list_single_done(store):
    execute(store, "ADD купить молоко")
    execute(store, "DONE 1")
    assert execute(store, "LIST") == "[x] 1 купить молоко"


def test_list_multiple_sorted_and_formatted(store):
    execute(store, "ADD купить молоко")      # 1
    execute(store, "ADD написать резюме")    # 2
    execute(store, "ADD позвонить маме")     # 3
    execute(store, "DONE 2")
    expected = "\n".join([
        "[ ] 1 купить молоко",
        "[x] 2 написать резюме",
        "[ ] 3 позвонить маме",
    ])
    assert execute(store, "LIST") == expected


def test_list_is_case_insensitive(store):
    execute(store, "ADD a")
    assert execute(store, "list") == "[ ] 1 a"


# --- PENDING -----------------------------------------------------------------

def test_pending_empty_store(store):
    assert execute(store, "PENDING") == "(нет невыполненных задач)"


def test_pending_empty_when_all_done(store):
    execute(store, "ADD a")
    execute(store, "DONE 1")
    assert execute(store, "PENDING") == "(нет невыполненных задач)"


def test_pending_lists_only_unfinished(store):
    execute(store, "ADD купить молоко")      # 1
    execute(store, "ADD написать резюме")    # 2
    execute(store, "ADD позвонить маме")     # 3
    execute(store, "DONE 2")
    expected = "\n".join([
        "[ ] 1 купить молоко",
        "[ ] 3 позвонить маме",
    ])
    assert execute(store, "PENDING") == expected


# --- SAVE / LOAD -------------------------------------------------------------

def test_save_returns_ok_and_creates_file(store, tmp_path):
    execute(store, "ADD a")
    path = tmp_path / "todo.json"
    assert execute(store, f"SAVE {path}") == "OK"
    assert path.exists()


def test_load_returns_ok_and_restores_state(store, tmp_path):
    execute(store, "ADD купить молоко")      # 1
    execute(store, "ADD написать резюме")    # 2
    execute(store, "DONE 2")
    path = tmp_path / "todo.json"
    execute(store, f"SAVE {path}")

    fresh = TaskStore()
    assert execute(fresh, f"LOAD {path}") == "OK"
    expected = "\n".join([
        "[ ] 1 купить молоко",
        "[x] 2 написать резюме",
    ])
    assert execute(fresh, "LIST") == expected


def test_save_without_path_is_error(store):
    assert execute(store, "SAVE") == "ERR save requires an argument"


def test_load_without_path_is_error(store):
    assert execute(store, "LOAD") == "ERR load requires an argument"


# --- Краевые случаи разбора --------------------------------------------------

def test_empty_line_is_error(store):
    assert execute(store, "") == "ERR empty command"


def test_whitespace_only_line_is_error(store):
    assert execute(store, "    ") == "ERR empty command"


def test_unknown_command_is_error(store):
    assert execute(store, "FOO bar") == "ERR unknown command: FOO"


def test_unknown_command_preserves_user_case(store):
    # Имя неизвестной команды возвращаем как ввёл пользователь (для понятной ошибки).
    assert execute(store, "Blah") == "ERR unknown command: Blah"


def test_leading_whitespace_is_tolerated(store):
    # Пробелы в начале строки не должны ломать разбор команды.
    assert execute(store, "   ADD задача") == "1"
