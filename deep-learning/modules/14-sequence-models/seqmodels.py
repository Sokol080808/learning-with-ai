# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 14 стали зелёными.
#
# Модуль 14 — Последовательности: эмбеддинги и RNN.
# Здесь ты собираешь первую рекуррентную сеть на PyTorch: она читает строку
# символ за символом, несёт скрытое состояние (память) и предсказывает следующий
# символ. Весь модуль — на CPU. Индексы символов держим как целые (torch.long).

import numpy as np  # noqa: F401  (пригодится для генерации/проверок; импорт обязателен по стилю курса)
import torch
import torch.nn as nn


class CharRNN(nn.Module):
    """Символьная рекуррентная сеть: Embedding -> RNN -> Linear.

    Архитектура (три слоя):
      - self.embed = nn.Embedding(vocab_size, embed_dim)
      - self.rnn   = nn.RNN(embed_dim, hidden_size, batch_first=True)
      - self.fc    = nn.Linear(hidden_size, vocab_size)

    forward(x):
      Контракт по формам:
        - вход  x: (B, T) — ЦЕЛЫЕ индексы символов (torch.long), B строк по T шагов;
        - embed: (B, T) -> (B, T, embed_dim);
        - rnn:   (B, T, embed_dim) -> out (B, T, hidden_size), h_n (1, B, hidden_size);
                 нам нужен out (состояние на КАЖДОМ шаге), h_n можно забрать в "_";
        - fc:    (B, T, hidden_size) -> (B, T, vocab_size);
        - возвращаем ЛОГИТЫ формы (B, T, vocab_size) — по числу на каждый символ
          словаря, для каждого шага каждой строки.

    ПОЧЕМУ так: эмбеддинг превращает индексы в обучаемые векторы, RNN прокатывает
    память по шагам (один и тот же блок на каждом шаге), Linear переводит память в
    логиты по словарю — «что сеть думает про следующий символ» в каждой позиции.
    """

    def __init__(self, vocab_size: int, embed_dim: int, hidden_size: int) -> None:
        """Создай три слоя (embed, rnn, fc) и сохрани размеры.

        Не забудь super().__init__() ПЕРВОЙ строкой, иначе слои не зарегистрируются.
        У nn.RNN обязательно batch_first=True — тогда формы будут (B, T, ...).
        """
        super().__init__()
        self.vocab_size = vocab_size
        self.embed_dim = embed_dim
        self.hidden_size = hidden_size
        self.embed = nn.Embedding(vocab_size, embed_dim)
        self.rnn = nn.RNN(embed_dim, hidden_size, batch_first=True)
        self.fc = nn.Linear(hidden_size, vocab_size)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """Прямой проход: индексы (B, T) -> логиты (B, T, vocab_size).

        Шаги: embed(x) -> rnn(...) (возьми только out, h_n в "_") -> fc(out).
        """
        e = self.embed(x)          # (B, T) -> (B, T, embed_dim)
        out, _ = self.rnn(e)       # (B, T, embed_dim) -> (B, T, hidden_size)
        logits = self.fc(out)      # (B, T, hidden_size) -> (B, T, vocab_size)
        return logits


def train_step(
    model: nn.Module,
    optimizer: torch.optim.Optimizer,
    loss_fn: nn.Module,
    X: torch.Tensor,
    Y: torch.Tensor,
) -> float:
    """Один шаг обучения. Возвращает значение loss НА ЭТОМ батче ДО шага (как float).

    Контракт:
      - X: (B, T) — целые индексы-входы;
      - Y: (B, T) — целевые индексы, СДВИНУТЫЕ на 1 (в позиции t лежит символ,
                    который должен идти ПОСЛЕ x_t);
      - loss_fn — это nn.CrossEntropyLoss(), он ждёт логиты формы (N, C) и цели (N,),
        поэтому перед вызовом схлопни оси: логиты (B, T, vocab) -> (B*T, vocab),
        цели Y (B, T) -> (B*T,).

    Канонический цикл шага (ровно в этом порядке):
      1) optimizer.zero_grad()      — обнулить накопленные градиенты;
      2) logits = model(X)          — forward, форма (B, T, vocab);
      3) loss = loss_fn(...)        — посчитать ошибку на схлопнутых формах;
      4) loss.backward()            — автоград посчитает градиенты;
      5) optimizer.step()           — оптимизатор сделает шаг.
    Верни loss.item() (число), посчитанный в шаге 3.
    """
    optimizer.zero_grad()
    logits = model(X)                                          # (B, T, vocab)
    vocab_size = logits.size(-1)
    loss = loss_fn(logits.reshape(-1, vocab_size), Y.reshape(-1))  # (B*T, vocab), (B*T,)
    loss.backward()
    optimizer.step()
    return loss.item()
