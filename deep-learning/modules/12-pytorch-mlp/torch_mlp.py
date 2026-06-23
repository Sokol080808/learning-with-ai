# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 12 стали зелёными.
#
# Модуль 12 — MLP на PyTorch: nn.Module и обучение.
# Здесь ты впервые собираешь настоящую обучаемую сеть как объект nn.Module,
# отдаёшь её параметры оптимизатору и пишешь цикл обучения из пяти строк:
# zero_grad -> forward -> loss -> backward -> step.
# Весь модуль — на CPU (никаких GPU).

import torch
import torch.nn as nn
from torch.utils.data import TensorDataset, DataLoader


class MLP(nn.Module):
    """Двухслойный перцептрон (multilayer perceptron) для классификации.

    Архитектура:
        x (N, in_dim) -> Linear(in_dim, hidden) -> ReLU -> Linear(hidden, out_dim) -> логиты (N, out_dim)

    Контракт:
      - __init__(in_dim, hidden, out_dim): первой строкой вызови super().__init__();
        затем создай два слоя nn.Linear и сохрани их в self (например, self.fc1, self.fc2).
        Между слоями — нелинейность ReLU (слоем nn.ReLU() или функцией torch.relu в forward).
      - forward(x): принимает x формы (N, in_dim), возвращает ЛОГИТЫ формы (N, out_dim)
        БЕЗ softmax (softmax/логарифм сделает функция потерь CrossEntropyLoss).

    ПОЧЕМУ ReLU между слоями: два Linear подряд без нелинейности схлопываются в один
    линейный слой. ReLU ломает линейность и даёт сети гибкость.
    ПОЧЕМУ без softmax на выходе: nn.CrossEntropyLoss ждёт сырые логиты и применяет
    softmax внутри сама (так точнее и удобнее).
    """

    def __init__(self, in_dim: int, hidden: int, out_dim: int) -> None:
        super().__init__()
        self.fc1 = nn.Linear(in_dim, hidden)
        self.fc2 = nn.Linear(hidden, out_dim)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.fc2(torch.relu(self.fc1(x)))


def train_step(
    model: nn.Module,
    optimizer: torch.optim.Optimizer,
    loss_fn: nn.Module,
    X: torch.Tensor,
    y: torch.Tensor,
) -> float:
    """Один шаг обучения. Возвращает значение потерь (float) на ТЕКУЩИХ параметрах (до шага).

    Схема (строго в этом порядке):
        1. optimizer.zero_grad()   — обнулить старые градиенты (.grad копит +=, иначе сложатся)
        2. logits = model(X)       — FORWARD: прогон сквозь сеть, форма (N, C)
        3. loss = loss_fn(logits, y) — LOSS: одно число
        4. loss.backward()         — BACKWARD: autograd заполняет .grad всех параметров
        5. optimizer.step()        — STEP: оптимизатор двигает параметры против градиента

    Формы:
      - X: (N, in_dim);
      - y: (N,) — целочисленные метки классов 0..C-1 (то, что ждёт CrossEntropyLoss).

    Возврат: loss.item() — обычный питоновский float (значение потерь до обновления).

    ПОЧЕМУ zero_grad первым: без обнуления градиенты накопятся с прошлого шага и шаг
    пойдёт не туда. ПОЧЕМУ возвращаем .item(): отвязываем число от графа, чтобы по нему
    нельзя было случайно продолжить backprop и чтобы не держать лишнюю память.
    """
    optimizer.zero_grad()
    logits = model(X)
    loss = loss_fn(logits, y)
    loss.backward()
    optimizer.step()
    return loss.item()


def train_epoch(
    model: nn.Module,
    optimizer: torch.optim.Optimizer,
    loss_fn: nn.Module,
    X: torch.Tensor,
    y: torch.Tensor,
    batch_size: int = 32,
) -> float:
    """Одна ЭПОХА обучения мини-батчами. Возвращает СРЕДНИЙ loss по всем мини-батчам.

    Эпоха — это один полный проход по всем данным, нарезанным на мини-батчи
    (в отличие от train_step, который делает один шаг на одном батче).

    Схема:
        1. dataset = TensorDataset(X, y)                  — склеить признаки и метки
        2. loader  = DataLoader(dataset, batch_size, shuffle=True)
        3. для каждого (X_batch, y_batch) из loader: loss = train_step(...)
        4. вернуть среднее значение loss по числу мини-батчей

    Формы:
      - X: (N, in_dim);
      - y: (N,) — целочисленные метки классов 0..C-1.
      - batch_size: размер мини-батча; последний батч может быть меньше, если N
        не делится на batch_size нацело.

    Возврат: float — средний loss по мини-батчам этой эпохи (среднее ПО БАТЧАМ,
    а не по объектам).

    ПОЧЕМУ мини-батчи: один объект даёт шумный градиент, весь датасет — точный, но
    дорогой и редкий шаг; мини-батч усредняет шум и даёт ceil(N/batch_size) обновлений
    за эпоху вместо одного. ПОЧЕМУ shuffle=True: ломает порядок данных, чтобы батчи не
    оказались из одного класса и обучение не перекосило.

    Оракул для самопроверки: при batch_size == len(X) и shuffle=False DataLoader выдаёт
    ровно один батч со всеми данными — тогда эта функция совпадает с одним train_step на
    полном X, y.
    """
    dataset = TensorDataset(X, y)
    loader = DataLoader(dataset, batch_size=batch_size, shuffle=True)
    total_loss = 0.0
    num_batches = 0
    for X_batch, y_batch in loader:
        total_loss += train_step(model, optimizer, loss_fn, X_batch, y_batch)
        num_batches += 1
    return total_loss / num_batches


def accuracy(model: nn.Module, X: torch.Tensor, y: torch.Tensor) -> float:
    """Доля верных предсказаний на (X, y). Возвращает float в диапазоне [0, 1].

    Контракт:
      - переведи модель в режим оценки: model.eval();
      - считай предсказания под torch.no_grad() (градиенты при оценке не нужны);
      - logits = model(X) формы (N, C); предсказанный класс = argmax по dim=1 -> (N,);
      - верни долю совпадений предсказаний с y как float.

    Формы:
      - X: (N, in_dim);
      - y: (N,) — истинные метки классов 0..C-1.

    Подсказка: (preds == y).float().mean().item() уже даёт число в [0, 1].

    ПОЧЕМУ eval() и no_grad(): eval отключает «капризные» слои (Dropout/BatchNorm) —
    в нашем простом MLP это no-op, но это правильная привычка; no_grad не строит граф,
    что быстрее и экономнее по памяти при простом замере качества.
    """
    model.eval()
    with torch.no_grad():
        logits = model(X)
        preds = logits.argmax(dim=1)
    return (preds == y).float().mean().item()
