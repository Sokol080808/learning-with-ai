# Это РАБОЧИЙ код, а не стаб. Трогать не обязательно — он «оживёт», когда ты
# реализуешь TaskStore (tasks/store.py) и execute (tasks/commands.py).
#
# app.py — тонкий слой ввода/вывода (REPL). Вся логика живёт в пакете tasks;
# здесь только: прочитать строку -> отдать в execute -> напечатать ответ -> повторить.
# Именно поэтому app.py не покрыт тестами: тестировать тут нечего, вся суть — в tasks.

from __future__ import annotations

import os
import sys

# Чтобы `from tasks ...` работал при запуске `python app.py` из любого каталога,
# добавим папку capstone (каталог этого файла) в начало путей поиска модулей.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from tasks import TaskStore, execute  # noqa: E402  (импорт после правки sys.path — это намеренно)

PROMPT = "> "
BANNER = (
    "Менеджер задач. Команды: ADD <title> | DONE <id> | RM <id> | "
    "LIST | PENDING | SAVE <path> | LOAD <path> | QUIT"
)


def repl(store: TaskStore) -> None:
    """Главный цикл Read–Eval–Print: читаем строку, выполняем, печатаем ответ.

    Завершается по команде QUIT, по концу ввода (Ctrl+D / EOF) или Ctrl+C.
    QUIT обрабатывается ЗДЕСЬ (это про жизнь цикла, а не про задачи), всё
    остальное уходит в execute().
    """
    print(BANNER)
    while True:
        try:
            line = input(PROMPT)
        except (EOFError, KeyboardInterrupt):
            print()  # перевод строки, чтобы prompt не «прилип» к выводу терминала
            break

        if line.strip().lower() == "quit":
            break

        answer = execute(store, line)
        print(answer)


def main() -> int:
    """Точка входа: создаём пустое хранилище и запускаем REPL."""
    store = TaskStore()
    repl(store)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
