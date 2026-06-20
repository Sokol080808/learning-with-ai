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
    if a.shape[0] != b.shape[0]:
        raise ValueError(
            f"Vectors must have equal length, got {a.shape[0]} and {b.shape[0]}"
        )
    return float(np.sum(a * b))


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
    n, k = A.shape
    k2, m = B.shape
    if k != k2:
        raise ValueError(
            f"Shape mismatch: A has {k} columns but B has {k2} rows"
        )
    C = np.zeros((n, m))
    for i in range(n):
        for j in range(m):
            C[i, j] = dot(A[i, :], B[:, j])
    return C


def l2_norm(v: np.ndarray) -> float:
    """Евклидова (L2) норма — длина вектора.

    Контракт:
      v: np.ndarray формы (n,)
      Возвращает: float >= 0, равный sqrt(v[0]^2 + v[1]^2 + ... + v[n-1]^2).

    Реализуй БЕЗ np.linalg.norm.
    """
    return float(np.sqrt(dot(v, v)))


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
    if a.shape[0] != b.shape[0]:
        raise ValueError(
            f"Vectors must have equal length, got {a.shape[0]} and {b.shape[0]}"
        )
    na = l2_norm(a)
    nb = l2_norm(b)
    if na == 0.0 or nb == 0.0:
        raise ValueError("Cannot compute cosine similarity with a zero vector (norm == 0)")
    return float(dot(a, b) / (na * nb))
