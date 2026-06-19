# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Модуль 09 — Итераторы и генераторы.
# Две из четырёх функций (countdown, running_total, chunks) — генераторы: внутри них
# должен быть yield, а не return со списком. take — обычная функция, возвращает список.

from typing import Iterable, Iterator, Any


def countdown(n: int) -> Iterator[int]:
    """Генератор: выдаёт числа n, n-1, …, 1 (по убыванию, включая 1).

    Если n <= 0 — не выдаёт ничего (пустая последовательность).
    Пример: list(countdown(3)) == [3, 2, 1]
    """
    current = n
    while current >= 1:
        yield current
        current -= 1


def take(it: Iterable[Any], k: int) -> list:
    """Вернуть список из первых k элементов итерируемого it.

    it может быть списком, генератором или даже бесконечной последовательностью.
    Если элементов меньше k — вернуть, сколько есть. Если k <= 0 — пустой список.
    Пример: take([1, 2, 3, 4], 2) == [1, 2]
    """
    if k <= 0:
        return []
    result: list = []
    for item in it:
        result.append(item)
        if len(result) == k:
            break
    return result


def running_total(xs: list[int]) -> Iterator[int]:
    """Генератор накопительных (нарастающих) сумм элементов xs.

    Для [1, 2, 3, 4] выдаёт 1, 3, 6, 10. Для пустого списка не выдаёт ничего.
    Пример: list(running_total([1, 2, 3])) == [1, 3, 6]
    """
    total = 0
    for x in xs:
        total += x
        yield total


def chunks(xs: list, size: int) -> Iterator[list]:
    """Генератор: режет список xs на куски-списки по size элементов.

    Последний кусок может быть короче. Считай, что size >= 1.
    Пример: list(chunks([1, 2, 3, 4, 5], 2)) == [[1, 2], [3, 4], [5]]
    """
    for i in range(0, len(xs), size):
        yield xs[i:i + size]
