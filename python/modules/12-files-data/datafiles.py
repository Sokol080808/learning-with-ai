# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Модуль 12 — Файлы и данные (текст, JSON, CSV).

from __future__ import annotations

import csv
import json
from pathlib import Path
from typing import Any, Callable, Iterator, Union

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


def write_csv(path: StrPath, rows: list[dict], fieldnames: list[str]) -> None:
    """Записать список словарей в CSV-файл с заголовком через csv.DictWriter.

    КРИТИЧНО: открывает файл с newline="" — иначе на Windows csv запишет лишний \\r.
    Первая строка файла — заголовок (fieldnames). Ячейки с запятой внутри автоматически
    берутся в кавычки: "London, UK".
    """
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def read_csv_typed(path: StrPath, schema: dict[str, Callable[[str], Any]]) -> list[dict]:
    """Прочитать CSV-файл через csv.DictReader с приведением типов согласно schema.

    Открывает файл с newline="" (требование модуля csv). Первая строка — заголовок
    (DictReader читает его автоматически). Для колонок из schema применяет конвертер;
    остальные колонки остаются строками. Пустой файл или только заголовок -> [].
    Правильно обрабатывает ячейки с запятой внутри ("London, UK"), в отличие от
    ручного split(",").
    """
    result: list[dict] = []
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            row_dict: dict[str, Any] = dict(row)
            for col, convert in schema.items():
                row_dict[col] = convert(row_dict[col])
            result.append(row_dict)
    return result


class TypedCSVReader:
    """Контекстный менеджер для чтения CSV-файла с приведением типов колонок.

    Открывает файл в __enter__, читает заголовок, проверяет схему.
    Итерация выдаёт строки данных как словари с применёнными конвертерами.
    После __exit__ итерация поднимает ValueError.
    """

    def __init__(self, path: StrPath, schema: dict[str, Callable[[str], Any]]) -> None:
        self._path = path
        self._schema = schema
        self._file = None
        self._header: list[str] = []

    def __enter__(self) -> "TypedCSVReader":
        self._file = open(self._path, "r", encoding="utf-8")
        first = self._file.readline()
        if first:
            self._header = first.rstrip("\n").split(",")
            for col in self._schema:
                if col not in self._header:
                    self._file.close()
                    self._file = None
                    raise KeyError(col)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> bool:
        if self._file is not None:
            self._file.close()
            self._file = None
        return False

    def __iter__(self) -> Iterator[dict[str, Any]]:
        if self._file is None or self._file.closed:
            raise ValueError("файл закрыт")
        for raw in self._file:
            values = raw.rstrip("\n").split(",")
            row: dict[str, Any] = dict(zip(self._header, values))
            for col, convert in self._schema.items():
                row[col] = convert(row[col])
            yield row
