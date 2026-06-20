# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 06 стали зелёными.
#
# Модуль 06 — Нейросеть с нуля: forward pass.
# Никакого PyTorch — только numpy. Следи за формами тензоров (они в контрактах).
import numpy as np


def relu(x: np.ndarray) -> np.ndarray:
    """Поэлементный ReLU: max(0, x).

    Контракт:
      - x: np.ndarray любой формы.
      - Возвращает np.ndarray ТОЙ ЖЕ формы, где каждый отрицательный элемент
        заменён на 0, а неотрицательные — без изменений.
      - Исходный x НЕ меняется (верни новый массив).
    """
    return np.maximum(0, x)


def softmax(x: np.ndarray) -> np.ndarray:
    """Softmax по ПОСЛЕДНЕЙ оси, численно стабильный.

    Контракт:
      - x: np.ndarray формы (..., C). Softmax считается независимо вдоль
        последней оси.
      - Возвращает np.ndarray ТОЙ ЖЕ формы; каждый срез по последней оси
        неотрицателен и суммируется в 1.
      - Численная стабильность ОБЯЗАТЕЛЬНА: вычти максимум по последней оси
        (keepdims=True) перед exp, иначе большие логиты дадут inf.
    """
    m = x.max(axis=-1, keepdims=True)
    e = np.exp(x - m)
    return e / e.sum(axis=-1, keepdims=True)


def linear(x: np.ndarray, W: np.ndarray, b: np.ndarray) -> np.ndarray:
    """Линейное преобразование (полносвязный слой без активации): x·W + b.

    Контракт:
      - x: (N, in)  — батч из N примеров по in признаков.
      - W: (in, out) — матрица весов.
      - b: (out,)   — вектор сдвига, прибавляется к каждой строке (broadcasting).
      - Возвращает (N, out).
    """
    return x @ W + b


def forward2(
    x: np.ndarray,
    W1: np.ndarray,
    b1: np.ndarray,
    W2: np.ndarray,
    b2: np.ndarray,
) -> np.ndarray:
    """Forward двухслойной сети (один скрытый слой + выходной). Возвращает ЛОГИТЫ.

    Архитектура:
        h      = relu(x·W1 + b1)   # скрытый слой: линейка + ReLU
        logits = h·W2 + b2         # выходной слой: только линейка, БЕЗ нелинейности

    Контракт (формы):
      - x:  (N, D)   — вход.
      - W1: (D, H), b1: (H,) — скрытый слой.
      - W2: (H, C), b2: (C,) — выходной слой.
      - Возвращает логиты формы (N, C). На выходе нелинейности НЕТ.
      - Используй уже написанные linear и relu, не дублируй матричное умножение.
    """
    h = relu(linear(x, W1, b1))
    return linear(h, W2, b2)


def cross_entropy(logits: np.ndarray, y: np.ndarray) -> float:
    """Средняя кросс-энтропия по батчу (численно устойчиво).

    Для примера i с правильным классом y_i:
        loss_i = -log( softmax(logits_i)[y_i] )
               = -logits[i, y_i] + log( sum_k exp(logits[i, k]) )
    Возвращается СРЕДНЕЕ loss_i по всем N примерам.

    Контракт:
      - logits: (N, C) — сырые счета классов.
      - y: (N,) — целочисленные индексы правильных классов, значения в [0, C).
      - Возвращает обычный python float (НЕ np.ndarray и НЕ массив формы ()).
      - Считай через log-sum-exp со сдвигом на максимум по строке, НЕ через
        log(softmax(...)) напрямую — иначе log(0) даст -inf.
    """
    N = logits.shape[0]
    m = logits.max(axis=1, keepdims=True)
    lse = m.ravel() + np.log(np.exp(logits - m).sum(axis=1))
    correct = logits[np.arange(N), y]
    return float(np.mean(lse - correct))
