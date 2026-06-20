# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 07 стали зелёными.
#
# Модуль 07 — Обратное распространение с нуля (numpy, без torch).
#
# Ты собираешь "ручной" backprop для двухслойной сети-классификатора:
#
#     z1 = x @ W1 + b1        # линейный слой 1
#     a1 = relu(z1)           # нелинейность
#     z2 = a1 @ W2 + b2       # линейный слой 2 (логиты)
#     p  = softmax(z2)        # вероятности классов
#     L  = cross_entropy(p, y)
#
# Твоя цель — функция backward2, которая считает dW1, db1, dW2, db2:
# производные потерь L по каждому параметру. Тест проверит их, сравнив с численными
# градиентами (конечные разности). Если аналитика совпала с численной проверкой —
# значит, твой backprop корректен.
#
# Формы тензоров (запомни их — половина успеха в backprop это формы):
#   x  : (N, D)   — N объектов, D входных признаков
#   W1 : (D, H)   b1 : (H,)     — первый слой: D -> H
#   W2 : (H, C)   b2 : (C,)     — второй слой: H -> C (C классов)
#   y  : (N,)     — целочисленные метки классов 0..C-1
#   z1, a1 : (N, H)
#   z2, p  : (N, C)

from typing import Dict
import numpy as np


def relu(z: np.ndarray) -> np.ndarray:
    """ReLU поэлементно: max(0, z). Форма сохраняется.

    Контракт: возвращает массив той же формы, где отрицательные элементы заменены нулём,
    а неотрицательные оставлены как есть.
    """
    return np.maximum(0.0, z)


def softmax(z: np.ndarray) -> np.ndarray:
    """Softmax по строкам (по последней оси, axis=-1).

    Вход z: (N, C) — логиты. Выход: (N, C) — вероятности, в каждой строке сумма == 1.

    Контракт (важно — численная устойчивость): перед exp вычти из каждой строки её
    максимум (z - max по axis=-1, keepdims=True). Это не меняет результат softmax
    математически, но спасает от переполнения exp. Затем нормируй на сумму exp по строке.
    """
    z_shifted = z - np.max(z, axis=-1, keepdims=True)
    exp = np.exp(z_shifted)
    return exp / np.sum(exp, axis=-1, keepdims=True)


def cross_entropy(p: np.ndarray, y: np.ndarray) -> float:
    """Средняя кросс-энтропия по батчу.

    p: (N, C) — вероятности (выход softmax). y: (N,) — целые метки классов.
    Возвращает одно число: -1/N * sum_i log( p[i, y[i]] ).

    Контракт: бери лог только у вероятности ПРАВИЛЬНОГО класса каждого объекта
    (p[i, y[i]]). Для устойчивости логарифма можно прибавить крошечную eps (например 1e-12).
    """
    N = y.shape[0]
    correct = p[np.arange(N), y]
    return float(-np.mean(np.log(correct + 1e-12)))


def forward2(
    x: np.ndarray,
    W1: np.ndarray,
    b1: np.ndarray,
    W2: np.ndarray,
    b2: np.ndarray,
    y: np.ndarray,
) -> float:
    """Прямой проход двухслойной сети до значения потерь.

    Считает: z1 = x@W1+b1 -> a1 = relu(z1) -> z2 = a1@W2+b2 -> p = softmax(z2)
    и возвращает cross_entropy(p, y) — одно число (скаляр-потеря).

    Эта функция нужна для ЧИСЛЕННОЙ проверки градиента: numerical_gradient будет
    много раз дёргать loss при чуть изменённых параметрах. Поэтому forward2 должна
    зависеть от параметров через переданные аргументы (не через глобальные переменные).

    Формы: x (N, D); W1 (D, H); b1 (H,); W2 (H, C); b2 (C,); y (N,). Возврат: float.
    """
    z1 = x @ W1 + b1
    a1 = relu(z1)
    z2 = a1 @ W2 + b2
    p = softmax(z2)
    return cross_entropy(p, y)


def backward2(
    x: np.ndarray,
    y: np.ndarray,
    W1: np.ndarray,
    b1: np.ndarray,
    W2: np.ndarray,
    b2: np.ndarray,
) -> Dict[str, np.ndarray]:
    """Обратное распространение для двухслойной сети (relu + softmax + cross-entropy).

    Возвращает словарь с градиентами потерь L по параметрам:
        {"dW1": (D, H), "db1": (H,), "dW2": (H, C), "db2": (C,)}
    Формы градиентов СОВПАДАЮТ с формами соответствующих параметров.

    Идея (цепное правило, слой за слоем от потерь к входу):

      1) Прямой проход (нужны промежуточные a1, p):
           z1 = x @ W1 + b1; a1 = relu(z1); z2 = a1 @ W2 + b2; p = softmax(z2)

      2) Градиент по логитам z2 — тот самый красивый результат softmax+CE:
           dz2 = (p - onehot(y)) / N            # форма (N, C)
         где onehot(y)[i, y[i]] = 1, остальное 0; делим на N, т.к. потеря — СРЕДНЯЯ по батчу.

      3) Второй (выходной) линейный слой z2 = a1 @ W2 + b2:
           dW2 = a1.T @ dz2                      # (H, N) @ (N, C) -> (H, C)
           db2 = sum(dz2, axis=0)                # (C,)
           da1 = dz2 @ W2.T                      # (N, C) @ (C, H) -> (N, H)

      4) Сквозь ReLU (a1 = relu(z1)): градиент проходит только там, где z1 > 0:
           dz1 = da1 * (z1 > 0)                  # (N, H), маска производной relu

      5) Первый линейный слой z1 = x @ W1 + b1:
           dW1 = x.T @ dz1                       # (D, N) @ (N, H) -> (D, H)
           db1 = sum(dz1, axis=0)                # (H,)

    Замечания:
      - Деление на N делаем ОДИН раз — в dz2 (шаг 2). Тогда dW1/dW2/db1/db2 автоматически
        получаются как градиенты СРЕДНЕЙ потери, и совпадут с численной проверкой forward2.
      - Используй relu/softmax из этого же файла, чтобы forward внутри backward2 совпадал
        с forward2.
    """
    N, C = x.shape[0], W2.shape[1]

    # Прямой проход — сохраняем промежуточные z1, a1, p для обратного.
    z1 = x @ W1 + b1
    a1 = relu(z1)
    z2 = a1 @ W2 + b2
    p = softmax(z2)

    # Градиент по логитам: красивый результат softmax + cross-entropy.
    oh = np.zeros((N, C))
    oh[np.arange(N), y] = 1.0
    dz2 = (p - oh) / N                # (N, C)

    # Второй (выходной) линейный слой.
    dW2 = a1.T @ dz2                  # (H, C)
    db2 = dz2.sum(axis=0)            # (C,)
    da1 = dz2 @ W2.T                 # (N, H)

    # Сквозь ReLU.
    dz1 = da1 * (z1 > 0)            # (N, H)

    # Первый линейный слой.
    dW1 = x.T @ dz1                  # (D, H)
    db1 = dz1.sum(axis=0)           # (H,)

    return {"dW1": dW1, "db1": db1, "dW2": dW2, "db2": db2}


def numerical_gradient(f, param: np.ndarray, eps: float = 1e-5) -> np.ndarray:
    """Численный градиент скалярной функции f по массиву param (центральная разность).

    f: функция БЕЗ аргументов, возвращающая скаляр-потерю при ТЕКУЩЕМ значении param.
       (Замыкание: внутри f используется тот же объект-массив param, что передан сюда.)
    param: массив параметров (любой формы). Возвращает массив той же формы — оценку dL/dparam.

    Контракт (центральная разность, по каждому элементу независимо):
        для каждого индекса i массива param:
            old = param[i]
            param[i] = old + eps;  f_plus  = f()
            param[i] = old - eps;  f_minus = f()
            grad[i] = (f_plus - f_minus) / (2*eps)
            param[i] = old           # ОБЯЗАТЕЛЬНО восстанови значение!
    Подсказка по обходу всех элементов: np.nditer(param, flags=["multi_index"], op_flags=["readwrite"]).
    """
    grad = np.zeros_like(param)
    it = np.nditer(param, flags=["multi_index"], op_flags=["readwrite"])
    while not it.finished:
        i = it.multi_index
        old = param[i]
        param[i] = old + eps
        f_plus = f()
        param[i] = old - eps
        f_minus = f()
        grad[i] = (f_plus - f_minus) / (2 * eps)
        param[i] = old           # восстановить значение!
        it.iternext()
    return grad
