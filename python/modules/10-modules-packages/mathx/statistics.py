# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Подмодуль mathx.statistics — описательная статистика.
# Снаружи функции видны как `from mathx import mean, median, mode, variance`.
#
# Нельзя делать `import statistics` — алгоритмы написаны вручную.
# Разрешены: math, collections.

from __future__ import annotations

import math
from collections import Counter
from typing import Sequence


def mean(data: Sequence[float]) -> float:
    """Среднее арифметическое элементов последовательности.

    Вычисляет среднее относительно первого элемента (сдвиг до нуля), что
    устраняет потерю значимости при делении суммы крупных близких значений:
        mean(data) = data[0] + mean(xi - data[0]).
    Так результат всегда попадает в диапазон [min(data), max(data)].

    Контракт:
      mean([1, 2, 3]) == 2.0
      mean([]) → ValueError
    """
    if len(data) == 0:
        raise ValueError("mean требует непустую последовательность")
    pivot = data[0]
    return pivot + math.fsum(x - pivot for x in data) / len(data)


def median(data: Sequence[float]) -> float:
    """Медиана: средний элемент (нечётный размер) или среднее двух центральных (чётный).

    Исходный список не изменяется — работает с отсортированной копией.

    Контракт:
      median([1, 2, 3]) == 2.0
      median([1, 2, 3, 4]) == 2.5
      median([]) → ValueError
    """
    if len(data) == 0:
        raise ValueError("median требует непустую последовательность")
    s = sorted(data)
    n = len(s)
    mid = n // 2
    if n % 2 == 1:
        return float(s[mid])
    return (s[mid - 1] + s[mid]) / 2.0


def mode(data: Sequence[float]) -> float:
    """Наиболее частый элемент. При ничьей — наименьший из самых частых.

    Контракт:
      mode([1, 2, 2, 3]) == 2
      mode([1, 1, 2, 2]) == 1   # ничья → наименьший
      mode([]) → ValueError
    """
    if len(data) == 0:
        raise ValueError("mode требует непустую последовательность")
    counts = Counter(data)
    peak = max(counts.values())
    # Собираем всех победителей и возвращаем наименьшего — чтобы результат был
    # детерминирован независимо от порядка вставки в Counter.
    winners = [k for k, v in counts.items() if v == peak]
    return min(winners)


def variance(data: Sequence[float], ddof: int = 0) -> float:
    """Дисперсия: среднее квадратов отклонений от среднего.

    ddof=0 — генеральная (делим на N).
    ddof=1 — выборочная с поправкой Бесселя (делим на N-1).

    Контракт:
      variance([2,4,4,4,5,5,7,9]) == 4.0
      variance([2,4,4,4,5,5,7,9], ddof=1) ≈ 32/7
      variance([]) → ValueError
      variance([x], ddof=1) → ValueError  (деление на 0)
      variance(data, ddof=2) → ValueError  (недопустимый ddof)
    """
    if ddof not in (0, 1):
        raise ValueError(f"ddof должен быть 0 или 1, получено: {ddof!r}")
    if len(data) == 0:
        raise ValueError("variance требует непустую последовательность")
    n = len(data)
    if ddof == 1 and n < 2:
        raise ValueError("variance с ddof=1 требует не менее двух элементов")
    m = mean(data)
    sq_deviations = [(x - m) ** 2 for x in data]
    return math.fsum(sq_deviations) / (n - ddof)
