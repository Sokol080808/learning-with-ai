# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 13 стали зелёными.
#
# Модуль 13 — Аннотации типов и тестирование.
# Обрати внимание на аннотации в сигнатурах: они часть контракта (хотя Python их в рантайме
# и не проверяет — это лишь подсказки людям и инструментам вроде mypy).
# Готовых решений тут нет — только сигнатуры и контракт. Реализуй сам, сверяясь с тестами.

from __future__ import annotations

from typing import Generic, Iterator, TypeVar

T = TypeVar("T")


class Stack(Generic[T]):
    """Типизированный стек (LIFO) на основе Generic[T].

    Полная сигнатура:
        Stack()                      — создаёт пустой стек
        push(item: T) -> None        — кладёт элемент на вершину
        pop() -> T                   — снимает и возвращает вершину; IndexError, если пуст
        peek() -> T                  — смотрит на вершину БЕЗ удаления; IndexError, если пуст
        __len__() -> int             — количество элементов
        is_empty() -> bool           — True тогда и только тогда, когда len == 0
        __iter__() -> Iterator[T]    — обход от вершины к основанию (не меняет стек)
        __repr__() -> str            — человекочитаемое представление

    Инварианты:
        - После push(x): peek() == x и len увеличивается на 1.
        - pop() возвращает элементы в порядке, обратном push (последний зашёл — первый вышел).
        - pop() и peek() на пустом стеке бросают IndexError.
        - Итерация не изменяет стек (len до и после одинаков).
    """

    def push(self, item: T) -> None:
        """Положить item на вершину стека."""
        raise NotImplementedError("TODO: реализуй push — добавь item на вершину")

    def pop(self) -> T:
        """Снять и вернуть верхний элемент. Бросает IndexError, если стек пуст."""
        raise NotImplementedError("TODO: реализуй pop — сними верхний элемент или бросай IndexError")

    def peek(self) -> T:
        """Вернуть верхний элемент БЕЗ удаления. Бросает IndexError, если стек пуст."""
        raise NotImplementedError("TODO: реализуй peek — посмотри на вершину, не трогая стек")

    def __len__(self) -> int:
        """Количество элементов в стеке."""
        raise NotImplementedError("TODO: реализуй __len__ — верни текущий размер")

    def is_empty(self) -> bool:
        """True, если стек пуст (len == 0)."""
        raise NotImplementedError("TODO: реализуй is_empty — делегируй к __len__")

    def __iter__(self) -> Iterator[T]:
        """Итератор от вершины к основанию. Стек НЕ изменяется."""
        raise NotImplementedError("TODO: реализуй __iter__ — обход без изменений, от вершины")

    def __repr__(self) -> str:
        """Человекочитаемое представление, например Stack([3, 2, 1])."""
        raise NotImplementedError("TODO: реализуй __repr__")


def clamp(x: int, lo: int, hi: int) -> int:
    """Зажать число x в диапазон [lo, hi].

    Если x < lo — вернуть lo; если x > hi — вернуть hi; иначе — само x.
    Если границы заданы неверно (lo > hi) — брось ValueError: зажимать в пустой
    диапазон бессмысленно. (Случай lo == hi допустим — диапазон из одной точки.)
    """
    raise NotImplementedError("TODO: реализуй clamp — проверь границы и прижми x к [lo, hi]")


def safe_get(d: dict[str, int], key: str) -> int | None:
    """Вернуть значение по ключу key, либо None, если такого ключа нет.

    Не бросай KeyError на отсутствующем ключе — верни None. Возвращаемый тип
    int | None прямо говорит: ответа может и не быть.
    """
    raise NotImplementedError("TODO: реализуй safe_get — верни значение или None, без KeyError")


def merge_counts(a: dict[str, int], b: dict[str, int]) -> dict[str, int]:
    """Сложить два словаря-счётчика по совпадающим ключам.

    В результате — каждый ключ из a и b; его значение — сумма значений из обоих
    словарей. Если ключ есть только в одном словаре — берётся его значение.
    Исходные словари a и b НЕ меняй — верни новый словарь.
    """
    raise NotImplementedError("TODO: реализуй merge_counts — сумма значений по ключам, новый dict")
