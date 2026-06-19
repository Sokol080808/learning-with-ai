# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Модуль 12 — Файлы и данные (текст, JSON, CSV).

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Union

# Путь может прийти строкой или Path — обе формы понимает open().
StrPath = Union[str, Path]


def write_lines(path: StrPath, lines: list[str]) -> None:
    """Записать строки в файл, по одной на строку (после каждой добавить '\\n').

    Открывай файл через with в режиме "w" (encoding="utf-8"). Сами элементы lines
    переводов строк не содержат — '\\n' добавляешь ты. Пустой список -> пустой файл.
    """
    with open(path, "w", encoding="utf-8") as f:
        for line in lines:
            f.write(line + "\n")


def read_lines(path: StrPath) -> list[str]:
    """Прочитать файл и вернуть список строк БЕЗ завершающего '\\n'.

    Это обратная операция к write_lines: после write_lines(p, lines) должно быть
    read_lines(p) == lines. Пустой файл -> пустой список [] (а не [""]).
    """
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    if text == "":
        return []
    # splitlines режет по '\n'; хвостовой '\n' после write_lines не даёт лишней пустой строки.
    return text.split("\n")[:-1] if text.endswith("\n") else text.split("\n")


def save_json(path: StrPath, obj: Any) -> None:
    """Сохранить объект в файл как JSON (через json.dump).

    Открывай файл через with в режиме "w" (encoding="utf-8"). Объект должен быть
    JSON-совместимым: dict/list/str/число/bool/None.
    """
    with open(path, "w", encoding="utf-8") as f:
        json.dump(obj, f)


def load_json(path: StrPath) -> Any:
    """Прочитать JSON-файл обратно в объект Python (через json.load).

    Обратная операция к save_json: после save_json(p, obj) должно быть
    load_json(p) == obj для JSON-совместимых obj.
    """
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def parse_csv(text: str) -> list[dict]:
    """Разобрать CSV-текст в список словарей.

    Первая строка — заголовок (имена колонок). Каждая следующая строка данных
    превращается в словарь {имя_колонки: значение}. Разделитель — запятая; кавычек и
    запятых внутри ячеек нет. Значения остаются СТРОКАМИ (CSV не знает типов).
    Если данных нет (пустой текст или только заголовок) — вернуть [].
    """
    if text == "":
        return []
    lines = text.splitlines()
    if not lines:
        return []
    header = lines[0].split(",")
    rows = []
    for line in lines[1:]:
        values = line.split(",")
        rows.append(dict(zip(header, values)))
    return rows
