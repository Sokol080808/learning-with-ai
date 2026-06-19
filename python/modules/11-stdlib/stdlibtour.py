# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Модуль 11 — Полезная стандартная библиотека.
# Тебе пригодятся: collections.Counter, collections.defaultdict, json, datetime.date.

from __future__ import annotations

import json
from collections import Counter, defaultdict
from datetime import date
from typing import Any


def most_common_word(text: str) -> str:
    """Вернуть самое частое слово в тексте.

    Слова разделяются пробелами (str.split()); регистр и пунктуацию НЕ нормализуем —
    берём слова как есть. Используй collections.Counter.
    Если в тексте нет слов (пустая или пробельная строка) — верни пустую строку "".
    При нескольких одинаково частых словах достаточно вернуть любое из самых частых.
    """
    words = text.split()
    if not words:
        return ""
    return Counter(words).most_common(1)[0][0]


def group_by_first_letter(words: list[str]) -> dict[str, list[str]]:
    """Сгруппировать слова по их первой букве.

    Ключ — первая буква слова, значение — список слов с этой буквой в ИСХОДНОМ порядке.
    Пустые строки в списке игнорируй (у них нет первой буквы).
    Используй collections.defaultdict. Вернуть можно обычный dict.
    """
    groups: defaultdict[str, list[str]] = defaultdict(list)
    for word in words:
        if not word:
            continue
        groups[word[0]].append(word)
    return dict(groups)


def to_json_str(obj: Any) -> str:
    """Сериализовать объект Python в JSON-строку (через json.dumps)."""
    return json.dumps(obj)


def from_json_str(s: str) -> Any:
    """Разобрать JSON-строку обратно в объект Python (через json.loads).

    Должно выполняться: from_json_str(to_json_str(x)) == x для JSON-совместимых x.
    """
    return json.loads(s)


def days_between(a: str, b: str) -> int:
    """Число дней между двумя датами в ISO-формате "YYYY-MM-DD".

    Разбери обе строки через datetime.date.fromisoformat и верни (b - a).days.
    Знак сохраняется: если a позже b, результат отрицательный.
    """
    start = date.fromisoformat(a)
    end = date.fromisoformat(b)
    return (end - start).days


# ---------------------------------------------------------------------------
# Существенное задание: RecordAggregator
# ---------------------------------------------------------------------------

import math


class RecordAggregator:
    """Типизированный агрегатор записей вида {date, category, value}.

    Объединяет Counter, defaultdict, json и datetime в один инструмент:
    принимает поток событий, хранит их, отдаёт сводку и топ-категории,
    умеет сериализовываться в JSON и восстанавливаться из него.
    """

    def __init__(self) -> None:
        self._records: list[dict] = []

    # ------------------------------------------------------------------
    # Валидация и добавление
    # ------------------------------------------------------------------

    @staticmethod
    def _validate(record: dict) -> None:
        """Проверить запись; при нарушении поднять ValueError / TypeError."""
        # date — разбираемая ISO-строка
        date.fromisoformat(record["date"])

        # category — непустая строка
        if not record["category"]:
            raise ValueError("category must be a non-empty string")

        # value — числовой тип (int или float, но не bool), конечное число
        v = record["value"]
        if isinstance(v, bool) or not isinstance(v, (int, float)):
            raise TypeError(f"value must be int or float, got {type(v).__name__}")
        if not math.isfinite(v):
            raise ValueError(f"value must be finite, got {v!r}")

    def add(self, record: dict) -> None:
        """Добавить запись после полной валидации всех трёх полей."""
        self._validate(record)
        self._records.append(record)

    # ------------------------------------------------------------------
    # Агрегация
    # ------------------------------------------------------------------

    def summary(self) -> dict:
        """Сводная статистика по всем добавленным записям.

        Ключи: count, categories, total_value, min_value, max_value,
               first_date, last_date.
        На пустом агрегаторе числовые поля — 0.0, остальные — None / {}.
        """
        if not self._records:
            return {
                "count": 0,
                "categories": {},
                "total_value": 0.0,
                "min_value": None,
                "max_value": None,
                "first_date": None,
                "last_date": None,
            }

        values = [float(r["value"]) for r in self._records]
        dates = [r["date"] for r in self._records]
        categories = dict(Counter(r["category"] for r in self._records))

        return {
            "count": len(self._records),
            "categories": categories,
            "total_value": float(sum(values)),
            "min_value": min(values),
            "max_value": max(values),
            "first_date": min(dates),
            "last_date": max(dates),
        }

    def top_categories(self, n: int = 3) -> list[tuple[str, int]]:
        """Топ-N категорий по убыванию частоты (Counter.most_common).

        При n=0 или отсутствии записей возвращает [].
        При n > числа категорий возвращает все категории.
        """
        if n == 0 or not self._records:
            return []
        return Counter(r["category"] for r in self._records).most_common(n)

    def daily_totals(self) -> dict[str, float]:
        """{ISO-дата: сумма value} по всем записям.

        Одна дата в нескольких записях — суммы накапливаются.
        """
        totals: defaultdict[str, float] = defaultdict(float)
        for r in self._records:
            totals[r["date"]] += float(r["value"])
        return dict(totals)

    # ------------------------------------------------------------------
    # Сериализация
    # ------------------------------------------------------------------

    def to_json(self) -> str:
        """Сериализовать весь список записей в JSON-строку."""
        return json.dumps(self._records)

    @classmethod
    def from_json(cls, s: str) -> "RecordAggregator":
        """Восстановить агрегатор из JSON-строки, созданной to_json().

        После from_json(to_json(agg)) метода summary() и daily_totals()
        возвращают те же значения, что и у оригинала.
        """
        instance = cls()
        for record in json.loads(s):
            instance.add(record)
        return instance
