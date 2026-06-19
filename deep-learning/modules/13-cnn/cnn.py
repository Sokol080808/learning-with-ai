# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 13 стали зелёными.
#
# Модуль 13 — Свёрточные сети (CNN).
# Ты считаешь формулу размера выхода свёртки, собираешь крошечную CNN на PyTorch
# (Conv2d + ReLU + MaxPool + Linear) для картинок 1 канал, 8x8, и пишешь один
# шаг обучения. Весь модуль — на CPU. Картинки в форме (N, C, H, W).

from typing import Callable

import torch
import torch.nn as nn


def conv_output_size(in_size: int, kernel: int, stride: int, pad: int) -> int:
    """Размер выхода свёртки по ОДНОЙ оси, по формуле (без вызова torch).

    Контракт:
      - in_size  — размер входа по этой оси (в пикселях);
      - kernel   — размер ядра по этой оси;
      - stride   — шаг скольжения ядра;
      - pad      — отступ (padding) с КАЖДОЙ стороны;
      - вернуть целое число выходных позиций.

    Формула:
        out = (in_size + 2*pad - kernel) // stride + 1

    Деление целочисленное (//): размер карты признаков — целое число.

    Примеры:
        conv_output_size(8, 3, 1, 0) -> 6     # (8 + 0 - 3)//1 + 1
        conv_output_size(8, 2, 2, 0) -> 4     # как MaxPool 2x2: (8-2)//2 + 1
        conv_output_size(8, 3, 1, 1) -> 8     # pad=1 сохраняет размер
    """
    raise NotImplementedError(
        "TODO: верни (in_size + 2*pad - kernel) // stride + 1"
    )


class SmallCNN(nn.Module):
    """Крошечная свёрточная сеть для картинок 1 канал, 8x8.

    Архитектура (собери слои в __init__, вызови по очереди в forward):
        Conv2d  ->  ReLU  ->  MaxPool2d  ->  Linear

    Контракт forward:
      - вход x — тензор формы (N, 1, 8, 8) типа float32;
      - выход — ЛОГИТЫ формы (N, num_classes); НЕ применяй softmax на конце
        (CrossEntropyLoss сделает softmax внутри себя).

    Подбери параметры слоёв так, чтобы формы сошлись. Один рабочий вариант:
        Conv2d(1, 4, kernel_size=3):  (N, 1, 8, 8) -> (N, 4, 6, 6)
        ReLU:                         форма не меняется
        MaxPool2d(2):                 (N, 4, 6, 6) -> (N, 4, 3, 3)
        flatten(1):                   (N, 4, 3, 3) -> (N, 36)
        Linear(36, num_classes):      (N, 36)      -> (N, num_classes)

    Слои-объекты создавай в __init__ (тогда их веса попадут в model.parameters()
    и будут обучаться), а в forward только вызывай их по очереди.
    """

    def __init__(self, num_classes: int = 2) -> None:
        super().__init__()
        raise NotImplementedError(
            "TODO: создай слои Conv2d, ReLU, MaxPool2d, Linear (см. таблицу форм)"
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """Прямой проход: (N, 1, 8, 8) -> логиты (N, num_classes).

        Порядок: conv -> relu -> pool -> flatten(1) -> linear.
        Не забудь «расплющить» перед Linear: x.flatten(1) превращает
        (N, 4, 3, 3) в (N, 36), не трогая ось батча N.
        """
        raise NotImplementedError(
            "TODO: прогони x через conv -> relu -> pool -> flatten(1) -> linear"
        )


def train_step(
    model: nn.Module,
    optimizer: torch.optim.Optimizer,
    loss_fn: Callable[[torch.Tensor, torch.Tensor], torch.Tensor],
    X: torch.Tensor,
    y: torch.Tensor,
) -> float:
    """Один шаг обучения. Возвращает значение loss (число) ДО шага оптимизатора.

    Контракт (канонический цикл одного шага в PyTorch):
      1) optimizer.zero_grad()         — обнулить накопленные градиенты;
      2) pred = model(X)               — forward, получили логиты (N, num_classes);
      3) loss = loss_fn(pred, y)       — посчитали ошибку (y — метки классов, форма (N,));
      4) loss.backward()               — autograd заполнил .grad у всех параметров;
      5) optimizer.step()              — оптимизатор сдвинул веса;
      6) верни loss.item()             — обычное число Python.

    ПОЧЕМУ zero_grad первым: в PyTorch .grad накапливается (+=), а не
    перезаписывается; без обнуления градиенты этого шага сложатся с прошлыми.
    """
    raise NotImplementedError(
        "TODO: zero_grad -> forward -> loss -> backward -> step -> верни loss.item()"
    )
