# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 03 стали зелёными.
#
# Каждая функция сейчас кидает NotImplementedError — это «дырка», которую тебе нужно
# закрыть. Не меняй сигнатуры (имена, параметры, типы) — на них опираются тесты.
# Подсказки — в README.md этого модуля (раздел «### Подсказки»).


def unique_sorted(xs: list[int]) -> list[int]:
    """Вернуть уникальные значения из xs, отсортированные по возрастанию.

    Дубликаты убираются, порядок — строго по возрастанию.
    Пример: [3, 1, 2, 3, 1] -> [1, 2, 3]. Для [] -> [].
    """
    raise NotImplementedError("TODO: убери дубликаты и верни отсортированный список")


def word_lengths(words: list[str]) -> dict[str, int]:
    """Построить словарь «слово -> его длина».

    Пример: ["кот", "пёс"] -> {"кот": 3, "пёс": 3}. Для [] -> {}.
    """
    raise NotImplementedError("TODO: собери словарь слово -> len(слово)")


def flatten(matrix: list[list[int]]) -> list[int]:
    """«Расплющить» список списков в один плоский список, сохранив порядок.

    Пример: [[1, 2], [3], [4, 5]] -> [1, 2, 3, 4, 5]. Для [] -> [].
    """
    raise NotImplementedError("TODO: пройди по строкам, а внутри — по элементам")


def intersection(a: list[int], b: list[int]) -> set[int]:
    """Вернуть множество значений, которые есть и в a, и в b (пересечение).

    Результат — множество (без дубликатов, порядок не важен).
    Пример: a=[1, 2, 2, 3], b=[2, 3, 4] -> {2, 3}.
    """
    raise NotImplementedError("TODO: пересеки множества, построенные из a и b")


def invert_dict(d: dict) -> dict:
    """Поменять местами ключи и значения исходного словаря.

    Считаем, что значения d уникальны и хешируемы.
    Пример: {"a": 1, "b": 2} -> {1: "a", 2: "b"}. Для {} -> {}.
    """
    raise NotImplementedError("TODO: построй словарь, где значение становится ключом")


# ---------------------------------------------------------------------------
# Задание (существенное): Multiset — мультимножество
# ---------------------------------------------------------------------------

class Multiset:
    """Мультимножество: как set, но каждый элемент может встречаться несколько раз.

    Хранит целочисленное количество вхождений каждого элемента (>= 0).
    Элементы с нулевым счётчиком считаются отсутствующими — `__contains__`
    возвращает False, и они не выходят в итерации/most_common.

    API:
        ms = Multiset()                # пустое
        ms = Multiset([1, 1, 2, 3])    # из итерируемого

        ms.add(x, count=1)             # добавить x count раз (count >= 1)
        ms.discard(x, count=1)         # убрать x count раз; если x < count — ноль (не падает)
        ms.count(x) -> int             # сколько раз x в мультимножестве (0 если нет)
        ms.most_common(n) -> list[tuple[element, int]]
                                       # топ-n элементов по убыванию счётчика
                                       # при равных счётчиках порядок произвольный
                                       # n=None (по умолчанию) — все элементы

        len(ms)       -> int           # суммарное число вхождений
        x in ms       -> bool          # True если ms.count(x) > 0
        ms | other    -> Multiset      # объединение: max(a.count(x), b.count(x)) по всем x
        ms & other    -> Multiset      # пересечение: min(a.count(x), b.count(x)) по всем x
                                       # (элементы с нулём — не включаются)

    Краевые случаи:
        - add/discard с count <= 0: raise ValueError
        - discard на отсутствующем элементе — не падает, остаётся 0
        - most_common(0) -> []
        - most_common(n > фактическое число уникальных) -> все уникальные элементы
    """

    def __init__(self, iterable=None):
        raise NotImplementedError("TODO: инициализируй внутренний счётчик, добавь элементы из iterable")

    def add(self, element, count: int = 1) -> None:
        """Добавить element count раз.

        Raises:
            ValueError: если count <= 0.
        """
        raise NotImplementedError("TODO: прибавь count к счётчику element; проверь count > 0")

    def discard(self, element, count: int = 1) -> None:
        """Убрать element count раз. Если текущий счётчик < count, обнулить (не падать).

        Raises:
            ValueError: если count <= 0.
        """
        raise NotImplementedError("TODO: уменьши счётчик element на count (не ниже 0); проверь count > 0")

    def count(self, element) -> int:
        """Вернуть количество вхождений element (0 если не встречается)."""
        raise NotImplementedError("TODO: верни счётчик element или 0")

    def most_common(self, n: int | None = None) -> list[tuple]:
        """Вернуть список (element, count) топ-n по убыванию счётчика.

        Только элементы с count > 0.  n=None — все.  n=0 — [].
        """
        raise NotImplementedError("TODO: отсортируй пары (element, count) по убыванию count и возьми первые n")

    def __len__(self) -> int:
        """Суммарное число вхождений всех элементов."""
        raise NotImplementedError("TODO: сумма всех счётчиков")

    def __contains__(self, element) -> bool:
        """True если element встречается хотя бы один раз."""
        raise NotImplementedError("TODO: count(element) > 0")

    def __or__(self, other: "Multiset") -> "Multiset":
        """Объединение: новое мультимножество с max(self.count(x), other.count(x)) для каждого x."""
        raise NotImplementedError("TODO: пройди по всем уникальным ключам обоих, возьми максимум")

    def __and__(self, other: "Multiset") -> "Multiset":
        """Пересечение: новое мультимножество с min(self.count(x), other.count(x)) для каждого x,
        включая только те x, где min > 0.
        """
        raise NotImplementedError("TODO: пройди по ключам self, возьми минимум с other, выкинь нули")
