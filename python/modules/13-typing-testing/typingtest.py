# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Модуль 13 — Аннотации типов и тестирование.
# Обрати внимание на аннотации в сигнатурах: они часть контракта (хотя Python их в рантайме
# не проверяет — это лишь подсказки людям и инструментам вроде mypy).

from __future__ import annotations

from typing import Generic, Iterator, TypeVar

T = TypeVar("T")


def clamp(x: int, lo: int, hi: int) -> int:
    """Зажать число x в диапазон [lo, hi].

    Если x < lo — вернуть lo; если x > hi — вернуть hi; иначе — само x.
    Если границы заданы неверно (lo > hi) — брось ValueError: зажимать в пустой
    диапазон бессмысленно. (Случай lo == hi допустим — диапазон из одной точки.)
    """
    if lo > hi:
        raise ValueError(f"пустой диапазон: lo={lo} > hi={hi}")
    return max(lo, min(x, hi))


def safe_get(d: dict[str, int], key: str) -> int | None:
    """Вернуть значение по ключу key, либо None, если такого ключа нет.

    Не бросай KeyError на отсутствующем ключе — верни None. Возвращаемый тип
    int | None прямо говорит: ответа может и не быть.
    """
    return d.get(key)


def merge_counts(a: dict[str, int], b: dict[str, int]) -> dict[str, int]:
    """Сложить два словаря-счётчика по совпадающим ключам.

    В результате — каждый ключ из a и b; его значение — сумма значений из обоих
    словарей. Если ключ есть только в одном словаре — берётся его значение.
    Исходные словари a и b НЕ меняй — верни новый словарь.
    """
    result = dict(a)
    for key, value in b.items():
        result[key] = result.get(key, 0) + value
    return result


class Stack(Generic[T]):
    """LIFO-стек с дженерик-типизацией через TypeVar.

    Stack[int] и Stack[str] — специализации одного класса; IDE понимает,
    что pop()/peek() вернут тот же тип, что принимал push().
    """

    def __init__(self) -> None:
        self._data: list[T] = []

    def push(self, item: T) -> None:
        """Положить item на вершину стека."""
        self._data.append(item)

    def pop(self) -> T:
        """Снять и вернуть вершину. IndexError, если стек пуст."""
        if not self._data:
            raise IndexError("pop from an empty stack")
        return self._data.pop()

    def peek(self) -> T:
        """Посмотреть на вершину без удаления. IndexError, если стек пуст."""
        if not self._data:
            raise IndexError("peek at an empty stack")
        return self._data[-1]

    def __len__(self) -> int:
        return len(self._data)

    def is_empty(self) -> bool:
        return len(self) == 0

    def __iter__(self) -> Iterator[T]:
        """Обход от вершины к основанию, не изменяя стек."""
        return iter(reversed(self._data))

    def __repr__(self) -> str:
        top_str = repr(self._data[-1]) if self._data else "—"
        return f"Stack(len={len(self)}, top={top_str})"
