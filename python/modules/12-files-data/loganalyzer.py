# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Микро-проект модуля 12 — Анализатор логов.
# На вход приходит список строк-логов вида "LEVEL: message" (как из read_lines).
# Уровни: INFO, WARN, ERROR. Строки не по формату (нет ':' или чужой уровень) — игнорируй.

from __future__ import annotations

# Допустимые уровни логов.
LEVELS = {"INFO", "WARN", "ERROR"}


def count_levels(lines: list[str]) -> dict[str, int]:
    """Посчитать, сколько строк каждого уровня.

    Строка имеет вид "LEVEL: message" — уровень это часть ДО первого ':'.
    В ответе только реально встретившиеся уровни (пустой ввод -> {}).
    Строки не по формату (нет ':' или уровень вне LEVELS) — игнорируй.
    """
    counts: dict[str, int] = {}
    for line in lines:
        if ":" not in line:
            continue
        level = line.split(":", 1)[0]
        if level in LEVELS:
            counts[level] = counts.get(level, 0) + 1
    return counts


def errors_only(lines: list[str]) -> list[str]:
    """Вернуть только строки уровня ERROR, целиком и в исходном порядке.

    Уровень определяется частью строки до первого ':'. Возвращай строку ЦЕЛИКОМ
    (как пришла), а не только текст сообщения.
    """
    result: list[str] = []
    for line in lines:
        if ":" not in line:
            continue
        if line.split(":", 1)[0] == "ERROR":
            result.append(line)
    return result
