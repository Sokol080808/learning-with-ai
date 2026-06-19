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
