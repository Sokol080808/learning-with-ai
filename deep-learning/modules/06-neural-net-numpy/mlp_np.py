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
    raise NotImplementedError("TODO: реализуй relu через np.maximum(0, x)")


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
    raise NotImplementedError("TODO: реализуй стабильный softmax по оси -1")


def linear(x: np.ndarray, W: np.ndarray, b: np.ndarray) -> np.ndarray:
    """Линейное преобразование (полносвязный слой без активации): x·W + b.

    Контракт:
      - x: (N, in)  — батч из N примеров по in признаков.
      - W: (in, out) — матрица весов.
      - b: (out,)   — вектор сдвига, прибавляется к каждой строке (broadcasting).
      - Возвращает (N, out).
    """
    raise NotImplementedError("TODO: реализуй x @ W + b")


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
    raise NotImplementedError("TODO: relu(linear(...)) затем linear(...)")


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
    raise NotImplementedError("TODO: средняя cross-entropy через log-sum-exp")


def init_xavier(fan_in: int, fan_out: int, rng: np.random.Generator = None) -> np.ndarray:
    """Инициализация весов Xavier (Glorot): масштаб под симметричные активации.

    Возвращает матрицу формы (fan_in, fan_out) из нормального шума со стандартным
    отклонением sqrt(2 / (fan_in + fan_out)). Усреднение fan_in и fan_out балансирует
    дисперсию и прямого, и обратного потока (см. README, Идея 6).

    Контракт:
      - fan_in, fan_out: положительные целые (число входов / выходов слоя).
      - rng: опциональный np.random.Generator для воспроизводимости. Если None —
        создаётся np.random.default_rng() (несидированный).
      - Возвращает np.ndarray формы (fan_in, fan_out); эмпирическое std ≈ целевому.
    """
    raise NotImplementedError("TODO: std = sqrt(2/(fan_in+fan_out)), сгенерируй матрицу через rng")


def init_he(fan_in: int, fan_out: int, rng: np.random.Generator = None) -> np.ndarray:
    """Инициализация весов He (Kaiming): масштаб под ReLU.

    Возвращает матрицу формы (fan_in, fan_out) из нормального шума со стандартным
    отклонением sqrt(2 / fan_in). Множитель 2 компенсирует то, что ReLU обнуляет
    примерно половину входов, теряя половину дисперсии (см. README, Идея 6).

    Контракт:
      - fan_in, fan_out: положительные целые.
      - rng: опциональный np.random.Generator. Если None — np.random.default_rng().
      - Возвращает np.ndarray формы (fan_in, fan_out); эмпирическое std ≈ целевому.
    """
    raise NotImplementedError("TODO: std = sqrt(2/fan_in), сгенерируй матрицу через rng")
