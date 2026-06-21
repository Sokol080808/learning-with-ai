# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 11 стали зелёными.
#
# Модуль 11 — Полезная стандартная библиотека.
# Тебе пригодятся: collections.Counter, collections.defaultdict, json, datetime.date.
# Готовых решений тут нет — только сигнатуры и контракт. Реализуй сам, сверяясь с тестами.

from __future__ import annotations

from typing import Any


def most_common_word(text: str) -> str:
    """Вернуть самое частое слово в тексте.

    Слова разделяются пробелами (str.split()); регистр и пунктуацию НЕ нормализуем —
    берём слова как есть. Используй collections.Counter.
    Если в тексте нет слов (пустая или пробельная строка) — верни пустую строку "".
    При нескольких одинаково частых словах достаточно вернуть любое из самых частых.
    """
    raise NotImplementedError("TODO: реализуй most_common_word через collections.Counter")


def group_by_first_letter(words: list[str]) -> dict[str, list[str]]:
    """Сгруппировать слова по их первой букве.

    Ключ — первая буква слова, значение — список слов с этой буквой в ИСХОДНОМ порядке.
    Пустые строки в списке игнорируй (у них нет первой буквы).
    Используй collections.defaultdict. Вернуть можно обычный dict.
    """
    raise NotImplementedError("TODO: реализуй group_by_first_letter через collections.defaultdict")


def to_json_str(obj: Any) -> str:
    """Сериализовать объект Python в JSON-строку (через json.dumps)."""
    raise NotImplementedError("TODO: реализуй to_json_str через json.dumps")


def from_json_str(s: str) -> Any:
    """Разобрать JSON-строку обратно в объект Python (через json.loads).

    Должно выполняться: from_json_str(to_json_str(x)) == x для JSON-совместимых x.
    """
    raise NotImplementedError("TODO: реализуй from_json_str через json.loads")


def days_between(a: str, b: str) -> int:
    """Число дней между двумя датами в ISO-формате "YYYY-MM-DD".

    Разбери обе строки через datetime.date.fromisoformat и верни (b - a).days.
    Знак сохраняется: если a позже b, результат отрицательный.
    """
    raise NotImplementedError("TODO: реализуй days_between через datetime.date.fromisoformat")


# ---------------------------------------------------------------------------
# Задание 6: running_totals (itertools.accumulate)
# ---------------------------------------------------------------------------

from itertools import accumulate as _accumulate, islice as _islice
from typing import Iterable


def running_totals(xs: list[float]) -> list[float]:
    """Вернуть список накопленных сумм (running sums) той же длины.

    Для [1, 2, 3, 4] результат [1, 3, 6, 10].
    Реализовано через itertools.accumulate.
    Для пустого списка возвращает [].
    """
    raise NotImplementedError("TODO: реализуй running_totals через itertools.accumulate")


# ---------------------------------------------------------------------------
# Задание 7: chunk (itertools.islice)
# ---------------------------------------------------------------------------


def chunk(it: Iterable, n: int) -> list[tuple]:
    """Разбить итерируемое на блоки (кортежи) по n элементов.

    Последний блок может быть меньше n, если элементов не хватает.
    Реализовано через itertools.islice.
    Для пустого итерируемого возвращает [].
    Для n <= 0 поднимает ValueError.
    """
    raise NotImplementedError("TODO: реализуй chunk через itertools.islice, для n<=0 кидай ValueError")


# ---------------------------------------------------------------------------
# Задание (существенное): RecordAggregator — типизированный агрегатор записей
# ---------------------------------------------------------------------------

import json as _json
from collections import Counter as _Counter, defaultdict as _defaultdict
from datetime import date as _date


class RecordAggregator:
    """Агрегатор записей вида {date, category, value}.

    Каждая запись — словарь с тремя обязательными полями:
      - "date"     : str  — дата в ISO-формате "YYYY-MM-DD" (проверяется через fromisoformat)
      - "category" : str  — непустая строка-категория
      - "value"    : int | float  — числовое значение (int или float, не NaN, не Inf)

    После добавления одной или нескольких записей через add() предоставляет аналитику:
      - summary()         — сводная статистика по всем записям
      - top_categories()  — топ-N категорий по количеству записей
      - daily_totals()    — суммы value по дням
      - to_json()         — сериализация состояния в JSON-строку
      - from_json()       — класс-метод: восстановление из JSON-строки (round-trip)

    Инвариант: на пустом агрегаторе summary() возвращает нули/None/пустые структуры
    (см. контракт метода), top_categories() — [], daily_totals() — {}.
    """

    def __init__(self) -> None:
        """Создать пустой агрегатор."""
        raise NotImplementedError("TODO: инициализируй внутреннее состояние (списки/словари)")

    def add(self, record: dict[str, Any]) -> None:
        """Добавить одну запись.

        Запись ОБЯЗАНА содержать:
          - "date"     : str в формате "YYYY-MM-DD" (иначе ValueError от fromisoformat)
          - "category" : непустую str (иначе ValueError)
          - "value"    : int или float, конечное число (иначе TypeError / ValueError)

        Валидируй ВСЕ три поля перед сохранением; при первом нарушении кидай исключение
        (можно ValueError или TypeError — смотри тесты).
        Используй datetime.date.fromisoformat для проверки даты.
        """
        raise NotImplementedError("TODO: добавь запись, провалидировав все три поля")

    def summary(self) -> dict[str, Any]:
        """Вернуть сводную статистику по всем добавленным записям.

        Возвращаемый словарь должен содержать ровно эти ключи:
          "count"          : int   — общее число добавленных записей
          "categories"     : dict[str, int]  — {категория: кол-во записей} через Counter
          "total_value"    : float — сумма всех value (0.0 если нет записей)
          "min_value"      : float | None    — минимум value (None если нет записей)
          "max_value"      : float | None    — максимум value (None если нет записей)
          "first_date"     : str | None      — самая ранняя дата ISO (None если нет записей)
          "last_date"      : str | None      — самая поздняя дата ISO (None если нет записей)

        Используй Counter для подсчёта категорий и min()/max() для дат и значений.
        """
        raise NotImplementedError("TODO: собери сводку через Counter и min/max")

    def top_categories(self, n: int = 3) -> list[tuple[str, int]]:
        """Вернуть топ-N категорий по числу записей (убывание).

        Результат — список пар (категория, кол-во), отсортированных по убыванию частоты,
        не длиннее n элементов. При n=0 или нет записей — пустой список.
        Используй Counter.most_common(n).
        """
        raise NotImplementedError("TODO: используй Counter.most_common(n)")

    def daily_totals(self) -> dict[str, float]:
        """Вернуть суммы value по датам {ISO-дата: сумма}.

        Используй collections.defaultdict(float) для накопления сумм.
        Ключи — строки ISO "YYYY-MM-DD", значения — float-суммы.
        Пустой агрегатор → пустой dict.
        """
        raise NotImplementedError("TODO: накопи суммы через defaultdict(float)")

    def to_json(self) -> str:
        """Сериализовать текущее состояние агрегатора в JSON-строку.

        Формат — на твоё усмотрение, но должен быть достаточен для from_json() round-trip:
        from_json(to_json()) должен дать агрегатор с идентичным summary() и daily_totals().
        Используй json.dumps.
        """
        raise NotImplementedError("TODO: сериализуй состояние через json.dumps")

    @classmethod
    def from_json(cls, s: str) -> "RecordAggregator":
        """Восстановить агрегатор из JSON-строки, ранее созданной to_json().

        Используй json.loads. После восстановления summary() и daily_totals() должны
        совпадать с оригиналом.
        """
        raise NotImplementedError("TODO: восстанови состояние через json.loads")
