# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 09 стали зелёными.
#
# Модуль 09 — Итераторы и генераторы.
# Две из четырёх функций (countdown, running_total, chunks) — генераторы: внутри них
# должен быть yield, а не return со списком. take — обычная функция, возвращает список.
#
# Готовых решений здесь нет: тело каждой функции пока кидает NotImplementedError.
# Сотри raise и напиши свою реализацию. Подсказки — в README.md.

from collections import deque
from typing import Iterable, Iterator, Any


def countdown(n: int) -> Iterator[int]:
    """Генератор: выдаёт числа n, n-1, …, 1 (по убыванию, включая 1).

    Если n <= 0 — не выдаёт ничего (пустая последовательность).
    Пример: list(countdown(3)) == [3, 2, 1]
    """
    raise NotImplementedError("TODO: реализуй генератор countdown через yield")


def take(it: Iterable[Any], k: int) -> list:
    """Вернуть список из первых k элементов итерируемого it.

    it может быть списком, генератором или даже бесконечной последовательностью.
    Если элементов меньше k — вернуть, сколько есть. Если k <= 0 — пустой список.
    Пример: take([1, 2, 3, 4], 2) == [1, 2]
    """
    raise NotImplementedError("TODO: набери первые k элементов из it, не упираясь в бесконечность")


def running_total(xs: list[int]) -> Iterator[int]:
    """Генератор накопительных (нарастающих) сумм элементов xs.

    Для [1, 2, 3, 4] выдаёт 1, 3, 6, 10. Для пустого списка не выдаёт ничего.
    Пример: list(running_total([1, 2, 3])) == [1, 3, 6]
    """
    raise NotImplementedError("TODO: веди накопитель и yield его текущее значение на каждом шаге")


def chunks(xs: list, size: int) -> Iterator[list]:
    """Генератор: режет список xs на куски-списки по size элементов.

    Последний кусок может быть короче. Считай, что size >= 1.
    Пример: list(chunks([1, 2, 3, 4, 5], 2)) == [[1, 2], [3, 4], [5]]
    """
    raise NotImplementedError("TODO: шагай по индексам с шагом size и yield срезы xs[i:i+size]")


class Window:
    """Класс-итератор скользящего окна над произвольным итерируемым объектом.

    Реализует протокол итератора через ``__iter__`` и ``__next__`` (НЕ генератор).
    Каждый вызов ``__next__`` возвращает очередной срез-окно в виде списка длиной
    ``size``, сдвигаясь на один элемент вправо. Если оставшихся элементов меньше
    ``size`` — итерация завершается (``StopIteration``).

    Примеры::

        list(Window([1, 2, 3, 4, 5], 3))
        # [[1, 2, 3], [2, 3, 4], [3, 4, 5]]

        list(Window("abcd", 2))
        # [['a', 'b'], ['b', 'c'], ['c', 'd']]

        list(Window([1, 2], 3))
        # []  — ни одного полного окна

        list(Window([], 1))
        # []

    Args:
        iterable: Любой итерируемый объект (список, строка, генератор, …).
        size: Размер окна, целое число >= 1.

    Raises:
        ValueError: Если ``size < 1``.
        StopIteration: Когда полных окон больше нет.

    Контракт (инварианты):
        - Каждый выданный элемент — список ровно из ``size`` значений.
        - Соседние окна перекрываются на ``size - 1`` элементов (скользящий сдвиг).
        - Результат ``list(Window(xs, 1))`` == ``[[x] for x in xs]``.
        - Результат ``list(Window(xs, size))`` для ``size > len(xs)`` — пустой список.
        - Количество окон: ``max(0, len(xs) - size + 1)`` (когда ``xs`` — список).
    """

    def __init__(self, iterable: Iterable[Any], size: int) -> None:
        raise NotImplementedError("TODO: сохрани size, создай deque(maxlen=size), получи iter(iterable)")

    def __iter__(self) -> "Window":
        raise NotImplementedError("TODO: итератор возвращает самого себя")

    def __next__(self) -> list:
        raise NotImplementedError(
            "TODO: advance iterator by one step — pop from source into deque, "
            "raise StopIteration when source is empty before deque is full, "
            "return list(self._buf) when deque reaches size"
        )
