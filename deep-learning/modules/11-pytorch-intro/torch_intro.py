# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 11 стали зелёными.
#
# Модуль 11 — Знакомство с PyTorch.
# Здесь ты впервые переносишь привычные операции на torch.Tensor и получаешь
# градиент через autograd, НЕ считая производную руками.
# Весь модуль — на CPU (никаких GPU). Всё считаем во float32.

from typing import Callable

import numpy as np  # noqa: F401  (пригодится: вход to_tensor может быть numpy-массивом)
import torch


def to_tensor(x) -> torch.Tensor:
    """Превратить вход в torch.Tensor типа float32.

    Контракт:
      - x может быть списком чисел, numpy-массивом или уже тензором;
      - вернуть torch.Tensor с тем же содержимым и формой, но dtype == torch.float32.

    ПОЧЕМУ float32: autograd работает только с вещественными типами, а float32 —
    стандарт точности/скорости в Deep Learning.

    Пример:
        to_tensor([1, 2, 3]).dtype  -> torch.float32
        to_tensor([1, 2, 3])        -> tensor([1., 2., 3.])
    """
    return torch.as_tensor(x, dtype=torch.float32)


def grad_of(f: Callable[[torch.Tensor], torch.Tensor], x: torch.Tensor) -> torch.Tensor:
    """Вернуть df/dx через autograd.

    Контракт:
      - f принимает тензор и возвращает СКАЛЯР (тензор формы ());
      - нужно посчитать градиент f по x и вернуть тензор той же формы, что x;
      - исходный x не портим: дифференцируем по его копии с requires_grad=True.

    Подсказка по шагам: сделай свежую копию x с включённым requires_grad,
    посчитай y = f(копия), вызови y.backward() и верни .grad этой копии.

    Пример (f(x) = (x**2).sum(), значит df/dx = 2x):
        x = torch.tensor([1.0, 2.0, 3.0])
        grad_of(lambda t: (t ** 2).sum(), x)  -> tensor([2., 4., 6.])
    """
    xr = x.clone().detach().requires_grad_(True)
    y = f(xr)
    y.backward()
    return xr.grad


def linear_forward(x: torch.Tensor, W: torch.Tensor, b: torch.Tensor) -> torch.Tensor:
    """Линейный слой: x @ W + b на тензорах.

    Формы:
      - x: (N, D_in)   — N объектов по D_in признаков;
      - W: (D_in, D_out);
      - b: (D_out,)    — добавляется к КАЖДОЙ строке (broadcasting);
      - результат: (N, D_out).

    Пример:
        x: (2, 3), W: (3, 4), b: (4,)  -> результат (2, 4)
    """
    return x @ W + b


def mse_torch(pred: torch.Tensor, y: torch.Tensor) -> torch.Tensor:
    """Среднеквадратичная ошибка: mean((pred - y) ** 2).

    Контракт:
      - pred и y одинаковой формы;
      - вернуть СКАЛЯРНЫЙ тензор (форма ()) — среднее квадратов разностей.

    Пример:
        pred = [1.0, 2.0, 3.0], y = [1.0, 2.0, 5.0]
        разности: [0, 0, -2]; квадраты: [0, 0, 4]; среднее: 4/3 ≈ 1.333
    """
    return torch.mean((pred - y) ** 2)


def numpy_bridge(x: np.ndarray) -> np.ndarray:
    """Мост torch ↔ numpy: numpy -> тензор -> +1.0 -> обратно в numpy.

    Контракт:
      - x — numpy-массив (любой формы);
      - превратить его во float32-тензор через torch.from_numpy (ОБЩАЯ память);
      - прибавить 1.0 ВНУТРИ torch.no_grad() (чтобы результат не отслеживал градиент);
      - вернуть результат как numpy-массив через .numpy();
      - на выходе: тот же shape, dtype == float32, массив на CPU, значения == x + 1.0.

    ПОЧЕМУ так (Идея 8):
      - from_numpy сохраняет dtype входа — поэтому вход приводим к float32;
      - .numpy() падает с RuntimeError на тензоре с requires_grad=True, поэтому
        безопасное «извлечение в numpy» оборачивают так, чтобы граф не строился
        (здесь — no_grad).

    Пример:
        numpy_bridge(np.array([1.0, 2.0], dtype=np.float32))  -> array([2., 3.], float32)
    """
    t = torch.from_numpy(x.astype(np.float32))
    with torch.no_grad():
        out = t + 1.0
    return out.numpy()
