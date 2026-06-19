# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.


def unique_sorted(xs: list[int]) -> list[int]:
    """Вернуть уникальные значения из xs, отсортированные по возрастанию.

    Дубликаты убираются, порядок — строго по возрастанию.
    Пример: [3, 1, 2, 3, 1] -> [1, 2, 3]. Для [] -> [].
    """
    return sorted(set(xs))


def word_lengths(words: list[str]) -> dict[str, int]:
    """Построить словарь «слово -> его длина».

    Пример: ["кот", "пёс"] -> {"кот": 3, "пёс": 3}. Для [] -> {}.
    """
    return {word: len(word) for word in words}


def flatten(matrix: list[list[int]]) -> list[int]:
    """«Расплющить» список списков в один плоский список, сохранив порядок.

    Пример: [[1, 2], [3], [4, 5]] -> [1, 2, 3, 4, 5]. Для [] -> [].
    """
    result: list[int] = []
    for row in matrix:
        for item in row:
            result.append(item)
    return result


def intersection(a: list[int], b: list[int]) -> set[int]:
    """Вернуть множество значений, которые есть и в a, и в b (пересечение).

    Результат — множество (без дубликатов, порядок не важен).
    Пример: a=[1, 2, 2, 3], b=[2, 3, 4] -> {2, 3}.
    """
    return set(a) & set(b)


def invert_dict(d: dict) -> dict:
    """Поменять местами ключи и значения исходного словаря.

    Считаем, что значения d уникальны и хешируемы.
    Пример: {"a": 1, "b": 2} -> {1: "a", 2: "b"}. Для {} -> {}.
    """
    return {value: key for key, value in d.items()}


class Multiset:
    """Мультимножество: хранит элементы с учётом кратности вхождения.

    В отличие от обычного set, каждый элемент можно добавить несколько раз —
    и структура помнит, сколько раз. Внутри — просто словарь element -> count.
    """

    def __init__(self, iterable=None):
        self._data: dict = {}
        if iterable is not None:
            for item in iterable:
                self.add(item)

    def add(self, element, *, count: int = 1) -> None:
        """Увеличить счётчик element на count (count > 0, иначе ValueError)."""
        if count <= 0:
            raise ValueError(f"count must be positive, got {count!r}")
        self._data[element] = self._data.get(element, 0) + count

    def discard(self, element, *, count: int = 1) -> None:
        """Уменьшить счётчик element на count, но не ниже 0 (count > 0, иначе ValueError).

        Если элемента нет в мультимножестве — ничего не происходит.
        """
        if count <= 0:
            raise ValueError(f"count must be positive, got {count!r}")
        current = self._data.get(element, 0)
        new_count = current - count
        if new_count <= 0:
            self._data.pop(element, None)
        else:
            self._data[element] = new_count

    def count(self, element) -> int:
        """Вернуть число вхождений element (0, если не встречался)."""
        return self._data.get(element, 0)

    def __len__(self) -> int:
        """Суммарное число вхождений всех элементов."""
        return sum(self._data.values())

    def __contains__(self, element) -> bool:
        """True, если element встречается хотя бы раз."""
        return self._data.get(element, 0) > 0

    def most_common(self, n: int | None = None) -> list[tuple]:
        """Вернуть список (element, count), отсортированный по убыванию count.

        Элементы с нулевым счётчиком не включаются.
        most_common()  — все элементы.
        most_common(k) — топ-k (или меньше, если уникальных элементов меньше k).
        most_common(0) — пустой список.
        """
        pairs = [(elem, cnt) for elem, cnt in self._data.items() if cnt > 0]
        pairs.sort(key=lambda t: t[1], reverse=True)
        if n is None:
            return pairs
        return pairs[:n]

    def __or__(self, other: "Multiset") -> "Multiset":
        """Объединение: count(x) = max(self.count(x), other.count(x))."""
        result = Multiset()
        all_keys = set(self._data) | set(other._data)
        for key in all_keys:
            merged = max(self.count(key), other.count(key))
            if merged > 0:
                result.add(key, count=merged)
        return result

    def __and__(self, other: "Multiset") -> "Multiset":
        """Пересечение: count(x) = min(self.count(x), other.count(x))."""
        result = Multiset()
        for key in set(self._data) | set(other._data):
            merged = min(self.count(key), other.count(key))
            if merged > 0:
                result.add(key, count=merged)
        return result
