# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Модуль 09 — Итераторы и генераторы.
# Две из четырёх функций (countdown, running_total, chunks) — генераторы: внутри них
# должен быть yield, а не return со списком. take — обычная функция, возвращает список.

from collections import deque
from collections.abc import Iterable as IterableABC
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


def flatten(nested: Iterable[Any]) -> Iterator[Any]:
    """Генератор: рекурсивно разворачивает вложенные итерируемые в плоский поток.

    Строки считаются атомарными значениями и НЕ разворачиваются поэлементно.
    Поддерживает произвольную глубину вложенности и любые iterable-контейнеры
    (списки, кортежи, range, генераторы).

    Примеры:
        list(flatten([1, [2, [3]], 4]))            == [1, 2, 3, 4]
        list(flatten(["hi", [1, "world"]]))        == ["hi", 1, "world"]
        list(flatten([1, (2, 3), range(4, 6)]))   == [1, 2, 3, 4, 5]
        list(flatten([]))                          == []
    """
    for item in nested:
        # Строки — iterable, но мы считаем их атомами: выдаём целиком.
        if isinstance(item, str):
            yield item
        elif isinstance(item, IterableABC):
            # yield from делегирует производство значений рекурсивному вызову:
            # внешний генератор «замолкает» до исчерпания вложенного.
            yield from flatten(item)
        else:
            yield item


class Window:
    """Скользящее окно (sliding window) над любым итерируемым.

    Выдаёт последовательные списки фиксированной ширины size, каждый раз
    сдвигаясь на один элемент вправо. Реализован через протокол итератора
    (__iter__ / __next__) без единого yield.

    Примеры:
        list(Window([1, 2, 3, 4, 5], 3)) == [[1, 2, 3], [2, 3, 4], [3, 4, 5]]
        list(Window("abcd", 2))          == [['a','b'], ['b','c'], ['c','d']]
        list(Window([1, 2], 5))          == []
    """

    def __init__(self, iterable: Iterable[Any], size: int) -> None:
        if size < 1:
            raise ValueError(f"size должен быть >= 1, получено {size!r}")
        self._size = size
        self._src: Iterator[Any] = iter(iterable)
        self._buf: deque[Any] = deque(maxlen=size)
        self._exhausted = False

        # Предварительно заполняем буфер первыми size элементами.
        # Если источник исчерпается раньше — помечаем как пустой.
        for _ in range(size):
            try:
                self._buf.append(next(self._src))
            except StopIteration:
                self._exhausted = True
                break

    def __iter__(self) -> "Window":
        return self

    def __next__(self) -> list:
        # Если буфер не заполнен (источник кончился до первого полного окна)
        # или итератор уже выгорел — сигнализируем конец.
        if self._exhausted or len(self._buf) < self._size:
            raise StopIteration

        snapshot = list(self._buf)

        # Подтягиваем следующий элемент для сдвига; если источник исчерпан —
        # помечаем флаг, но текущий snapshot уже сформирован корректно.
        try:
            self._buf.append(next(self._src))
        except StopIteration:
            self._exhausted = True

        return snapshot
