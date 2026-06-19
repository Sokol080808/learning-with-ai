# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 12 стали зелёными.
#
# Модуль 12 — Файлы и данные (текст, JSON, CSV).
# Тебе пригодятся: open + with, json.dump/json.load, str.splitlines, str.split, zip.
# Пути приходят и строкой, и pathlib.Path — open() и Path() принимают оба варианта.
# Готовых решений тут нет — только сигнатуры и контракт. Реализуй сам, сверяясь с тестами.

from __future__ import annotations

from pathlib import Path
from typing import Any, Union

# Путь может прийти строкой или Path — обе формы понимает open().
StrPath = Union[str, Path]


def write_lines(path: StrPath, lines: list[str]) -> None:
    """Записать строки в файл, по одной на строку (после каждой добавить '\\n').

    Открывай файл через with в режиме "w" (encoding="utf-8"). Сами элементы lines
    переводов строк не содержат — '\\n' добавляешь ты. Пустой список -> пустой файл.
    """
    raise NotImplementedError("TODO: реализуй write_lines через with open(path, 'w')")


def read_lines(path: StrPath) -> list[str]:
    """Прочитать файл и вернуть список строк БЕЗ завершающего '\\n'.

    Это обратная операция к write_lines: после write_lines(p, lines) должно быть
    read_lines(p) == lines. Пустой файл -> пустой список [] (а не [""]).
    """
    raise NotImplementedError("TODO: реализуй read_lines через with open(path, 'r')")


def save_json(path: StrPath, obj: Any) -> None:
    """Сохранить объект в файл как JSON (через json.dump).

    Открывай файл через with в режиме "w" (encoding="utf-8"). Объект должен быть
    JSON-совместимым: dict/list/str/число/bool/None.
    """
    raise NotImplementedError("TODO: реализуй save_json через json.dump")


def load_json(path: StrPath) -> Any:
    """Прочитать JSON-файл обратно в объект Python (через json.load).

    Обратная операция к save_json: после save_json(p, obj) должно быть
    load_json(p) == obj для JSON-совместимых obj.
    """
    raise NotImplementedError("TODO: реализуй load_json через json.load")


def parse_csv(text: str) -> list[dict]:
    """Разобрать CSV-текст в список словарей.

    Первая строка — заголовок (имена колонок). Каждая следующая строка данных
    превращается в словарь {имя_колонки: значение}. Разделитель — запятая; кавычек и
    запятых внутри ячеек нет. Значения остаются СТРОКАМИ (CSV не знает типов).
    Если данных нет (пустой текст или только заголовок) — вернуть [].
    """
    raise NotImplementedError("TODO: реализуй parse_csv через splitlines/split и zip")
