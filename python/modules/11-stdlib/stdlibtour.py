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
