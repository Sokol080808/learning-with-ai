# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 02 стали зелёными.
#
# Правило модуля: всё руками на numpy. НЕЛЬЗЯ использовать np.dot, np.matmul, оператор @ и
# np.linalg.norm — иначе теряется смысл упражнения. Можно: индексацию, срезы, циклы, поэлементные
# +, *, **, np.sum, np.sqrt, np.zeros, float(...).
#
# Формы (shape) проговорены словами в README. Коротко:
#   вектор длины n      -> форма (n,)
#   матрица rows x cols -> форма (rows, cols)

import numpy as np


def dot(a: np.ndarray, b: np.ndarray) -> float:
    """Скалярное произведение двух векторов.

    Контракт:
      a: np.ndarray формы (n,)
      b: np.ndarray формы (n,)
      Возвращает: float = a[0]*b[0] + a[1]*b[1] + ... + a[n-1]*b[n-1].
      Если длины a и b не совпадают — поднять ValueError.

    Реализуй БЕЗ np.dot.
    """
    raise NotImplementedError("TODO: реализуй скалярное произведение (без np.dot)")


def matmul(A: np.ndarray, B: np.ndarray) -> np.ndarray:
    """Матричное умножение A @ B, реализованное вручную.

    Контракт:
      A: np.ndarray формы (n, k)
      B: np.ndarray формы (k, m)
      Возвращает: np.ndarray формы (n, m), тип float, где
        C[i, j] = скалярное произведение i-й строки A и j-го столбца B.
      Если внутренние размеры не согласованы (число столбцов A != число строк B) —
      поднять ValueError.

    Реализуй БЕЗ np.matmul и без оператора @.
    """
    raise NotImplementedError("TODO: реализуй матричное умножение (без np.matmul и @)")


def l2_norm(v: np.ndarray) -> float:
    """Евклидова (L2) норма — длина вектора.

    Контракт:
      v: np.ndarray формы (n,)
      Возвращает: float >= 0, равный sqrt(v[0]^2 + v[1]^2 + ... + v[n-1]^2).

    Реализуй БЕЗ np.linalg.norm.
    """
    raise NotImplementedError("TODO: реализуй L2-норму (без np.linalg.norm)")


def cosine_similarity(a: np.ndarray, b: np.ndarray) -> float:
    """Косинусная близость двух векторов.

    Контракт:
      a: np.ndarray формы (n,)
      b: np.ndarray формы (n,)
      Возвращает: float в [-1, 1], равный dot(a, b) / (l2_norm(a) * l2_norm(b)).
      Если длины a и b не совпадают — поднять ValueError.
      Если хотя бы один вектор нулевой (его L2-норма == 0) — поднять ValueError
      (делить на ноль нельзя).
    """
    raise NotImplementedError("TODO: реализуй косинусную близость")


def linear_forward(X: np.ndarray, W: np.ndarray, b: np.ndarray) -> np.ndarray:
    """Прямой проход линейного (полносвязного) слоя: Y = X @ W + b.

    Это сборка всего модуля в одну функцию — именно то, что прячется внутри
    `torch.nn.Linear`. Матричное умножение даёт «сырые» выходы нейронов, а
    вектор сдвига b добавляется к КАЖДОЙ строке батча (broadcast).

    Контракт:
      X: np.ndarray формы (N, D) — батч из N примеров по D признаков.
      W: np.ndarray формы (D, H) — веса: D входов, H нейронов (выходов).
      b: np.ndarray формы (H,)   — сдвиг (bias), по одному числу на нейрон.
      Возвращает: np.ndarray формы (N, H), тип float, где
        Y[i, j] = dot(X[i, :], W[:, j]) + b[j].

      Если число столбцов X (= D) не равно числу строк W — поднять ValueError.
      Если длина b не равна числу столбцов W (= H) — поднять ValueError.

    Реализуй БЕЗ np.matmul и без оператора @ — переиспользуй свой matmul, а bias
    прибавь обычным поэлементным `+` (broadcast (H,) на (N, H) numpy делает сам).
    """
    raise NotImplementedError("TODO: используй свой matmul(X, W) и прибавь b (broadcast)")
