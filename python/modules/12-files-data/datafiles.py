# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 12 стали зелёными.
#
# Модуль 12 — Файлы и данные (текст, JSON, CSV).
# Тебе пригодятся: open + with, json.dump/json.load, str.splitlines, str.split, zip.
# Пути приходят и строкой, и pathlib.Path — open() и Path() принимают оба варианта.
# Готовых решений тут нет — только сигнатуры и контракт. Реализуй сам, сверяясь с тестами.

from __future__ import annotations

from pathlib import Path
from typing import Any, Callable, Iterator, Union

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


# ---------------------------------------------------------------------------
# Задание (существенное): TypedCSVReader
# ---------------------------------------------------------------------------

class TypedCSVReader:
    """Контекст-менеджер для чтения CSV-файла с приведением типов колонок.

    Открывает файл при входе в блок with и закрывает при выходе — даже если внутри
    было исключение. При итерации возвращает строки CSV уже с приведёнными значениями
    согласно переданной схеме.

    Параметры
    ----------
    path : StrPath
        Путь к CSV-файлу (строка или pathlib.Path). Первая строка файла — заголовок.
    schema : dict[str, Callable[[str], Any]]
        Словарь {имя_колонки: функция-конвертер}. Конвертер вызывается как
        ``converter(raw_string)`` и может быть любым вызываемым (int, float, str,
        пользовательская функция). Колонки, не упомянутые в schema, остаются
        строками. Если schema содержит имя, которого нет в заголовке файла, при
        входе в with-блок поднимается KeyError с именем отсутствующей колонки.

    Поведение итератора
    -------------------
    Каждый элемент итерации — dict {имя_колонки: значение}. Для колонок из schema
    значение уже приведено конвертером; остальные — строки. Порядок ключей совпадает
    с заголовком файла. Если конвертер поднимает ValueError — исключение прокидывается
    наружу как есть.

    Примеры
    -------
    Файл ``grades.csv``::

        name,score,passed
        Ada,95,true
        Bjarne,72,false

    >>> with TypedCSVReader("grades.csv", schema={"score": int, "passed": lambda s: s == "true"}) as reader:
    ...     rows = list(reader)
    >>> rows[0]
    {'name': 'Ada', 'score': 95, 'passed': True}

    Контракт краевых случаев
    ------------------------
    * Пустой файл (ноль строк) — итерация не даёт ни одной записи.
    * Только заголовок (одна строка) — итерация не даёт ни одной записи.
    * ``schema={}`` — все значения остаются строками (аналог parse_csv, но из файла).
    * Файл не открывается до вызова ``__enter__``; после ``__exit__`` обращение к
      итератору должно поднимать ValueError (файл закрыт).
    """

    def __init__(self, path: StrPath, schema: dict[str, Callable[[str], Any]]) -> None:
        raise NotImplementedError("TODO: сохрани path и schema как атрибуты; файл пока не открывай")

    def __enter__(self) -> "TypedCSVReader":
        """Открыть файл, прочитать заголовок; вернуть self.

        Raises
        ------
        KeyError
            Если schema содержит имя колонки, которой нет в заголовке файла.
        FileNotFoundError
            Если файл не существует (прокидывается от open()).
        """
        raise NotImplementedError("TODO: открой файл, считай заголовок, проверь schema против него")

    def __exit__(self, exc_type: Any, exc_val: Any, exc_tb: Any) -> bool:
        """Закрыть файл. Исключения не подавляет (вернуть False)."""
        raise NotImplementedError("TODO: закрой файл; верни False (не подавляй исключения)")

    def __iter__(self) -> Iterator[dict[str, Any]]:
        """Итерировать строки данных, применяя конвертеры из schema.

        Yields
        ------
        dict
            Словарь {имя_колонки: значение} для каждой строки данных. Колонки из
            schema уже приведены соответствующим конвертером, остальные — строки.

        Raises
        ------
        ValueError
            Если файл уже закрыт или если конвертер поднял ValueError.
        """
        raise NotImplementedError("TODO: итерируй строки файла, разбивай по запятой, применяй конвертеры")
