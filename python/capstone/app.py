# РАБОЧИЙ файл (НЕ стаб). Это интерактивная оболочка (REPL) над твоей мини-СУБД.
#
# app.py трогать не нужно — он уже готов и сам заработает, КАК ТОЛЬКО ты реализуешь движок
# (tokenize/parse/Database.execute). До этого он будет честно ловить NotImplementedError из
# стабов и печатать «(не реализовано: ...)». Это удобный способ руками пощупать прогресс:
#     python app.py
#
# Что делает REPL:
#   - читает строки запросов из stdin (приглашение "db> ");
#   - ".quit" или ".exit" (а также Ctrl-D / пустой EOF) — выход;
#   - для SELECT печатает результат ТАБЛИЦЕЙ (выровненные колонки);
#   - для CREATE/INSERT печатает "OK", для DELETE — "удалено строк: N";
#   - QueryError печатает как "Ошибка: ..." и продолжает работу (программа не падает).
#
# Используем только стандартную библиотеку (модуль 11): sys для аргументов/потоков.

from __future__ import annotations

import sys
from typing import Any

from db import Database, QueryError


def format_table(rows: list[dict]) -> str:
    """Отрисовать список строк-словарей как выровненную текстовую таблицу.

    Колонки берём из первой строки (порядок ключей сохраняется — dict упорядочен).
    Пустой результат -> "(пусто)".
    """
    if not rows:
        return "(пусто)"

    columns = list(rows[0].keys())
    # Ширина колонки = максимум из длины заголовка и длин всех значений в ней.
    widths = {
        col: max(len(str(col)), *(len(str(r.get(col, ""))) for r in rows))
        for col in columns
    }

    def render_row(values: list[str]) -> str:
        return " | ".join(str(v).ljust(widths[col]) for col, v in zip(columns, values))

    header = render_row(columns)
    separator = "-+-".join("-" * widths[col] for col in columns)
    body = [render_row([row.get(col, "") for col in columns]) for row in rows]
    return "\n".join([header, separator, *body])


def format_result(result: Any) -> str:
    """Превратить результат Database.execute в строку для печати в REPL."""
    if result is None:
        return "OK"
    if isinstance(result, int):
        return f"удалено строк: {result}"
    if isinstance(result, list):
        return format_table(result)
    return str(result)


def run_line(db: Database | None, line: str) -> str | None:
    """Обработать одну введённую строку. Вернуть текст для печати или None (нечего печатать).

    Возвращает специальную метку None для пустых строк (их просто пропускаем).
    db может быть None, если движок ещё в стабах (Database() не построился) — тогда честно
    сообщаем «не реализовано», но REPL остаётся жив.
    """
    line = line.strip()
    if not line:
        return None
    if db is None:
        return "(не реализовано: Database ещё не реализован — допиши db/database.py)"
    try:
        return format_result(db.execute(line))
    except QueryError as exc:
        return f"Ошибка: {exc}"
    except NotImplementedError as exc:
        # Пока движок не дописан — не роняем REPL, а честно сообщаем, что не готово.
        return f"(не реализовано: {exc})"


def main(argv: list[str] | None = None) -> int:
    """Точка входа REPL. Читает строки до .quit/.exit или конца ввода."""
    try:
        db: Database | None = Database()
    except NotImplementedError:
        # Стаб-состояние: движок ещё не написан. Не падаем — даём ученику живой REPL,
        # который на каждый запрос честно скажет «не реализовано».
        db = None
    print("мини-СУБД. Введите запрос или .quit для выхода.")
    while True:
        try:
            line = input("db> ")
        except EOFError:
            print()
            break
        if line.strip() in (".quit", ".exit"):
            break
        out = run_line(db, line)
        if out is not None:
            print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
