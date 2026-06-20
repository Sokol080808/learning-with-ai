# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 17 стали зелёными.
#
# Модуль 17 — Воспроизводимость и инженерия.
# Здесь ты делаешь эксперимент повторяемым: фиксируешь ВСЕ источники случайности,
# считаешь размер модели и умеешь сохранять/загружать веса через state_dict.
# Весь модуль — на CPU (никаких GPU).

import random  # noqa: F401  (пригодится: один из трёх генераторов случайности)

import numpy as np  # noqa: F401  (пригодится: второй генератор случайности)
import torch
import torch.nn as nn
import torch.optim as optim


def set_seed(seed: int) -> None:
    """Зафиксировать ВСЕ три источника случайности: random, numpy, torch.

    Контракт:
      - засидировать стандартный модуль random;
      - засидировать numpy.random;
      - засидировать torch;
      - ничего не возвращать (None) — работа в побочном эффекте.

    ПОЧЕМУ все три: в DL-коде работают три независимых генератора, и забытый
    любой из них рушит воспроизводимость. После set_seed(seed) два одинаковых
    блока со случайностью обязаны дать идентичные результаты.

    Пример:
        set_seed(0); a = torch.randn(4)
        set_seed(0); b = torch.randn(4)
        # тогда torch.allclose(a, b) == True
    """
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)


def count_parameters(model: torch.nn.Module) -> int:
    """Число ОБУЧАЕМЫХ параметров модели.

    Контракт:
      - просуммировать numel() по всем параметрам model.parameters();
      - учитывать ТОЛЬКО те, у кого requires_grad == True (замороженные не в счёт);
      - вернуть int.

    Напоминание: p.numel() — число элементов в тензоре-параметре (произведение
    размеров его формы). Для nn.Linear(4, 3): weight (3, 4) = 12 и bias (3,) = 3,
    итого 15.

    Пример:
        count_parameters(torch.nn.Linear(4, 3))  -> 15
    """
    return int(sum(p.numel() for p in model.parameters() if p.requires_grad))


def save_model(model: torch.nn.Module, path: str) -> None:
    """Сохранить state_dict модели в файл path.

    Контракт:
      - сохранить именно state_dict (словарь {имя: тензор}), а не объект целиком;
      - ничего не возвращать (None).

    ПОЧЕМУ state_dict: это просто данные (веса по именам), устойчивые к рефакторингу
    кода, в отличие от пикла всего объекта.

    Пример:
        save_model(model, "model.pt")   # на диске появится файл с весами
    """
    torch.save(model.state_dict(), path)


def load_model(model: torch.nn.Module, path: str) -> torch.nn.Module:
    """Загрузить state_dict из path в model и вернуть эту же model.

    Контракт:
      - model — это уже готовый каркас НУЖНОЙ архитектуры (создан до вызова);
      - прочитать state_dict из файла path и влить его в model;
      - вернуть ту же самую model с залитыми весами.

    После корректной загрузки веса model поэлементно совпадают с сохранёнными
    (проверяется через allclose).

    Пример:
        m2 = SomeNet()                  # пустой каркас, веса случайные
        load_model(m2, "model.pt")      # веса m2 стали такими же, как у сохранённой
    """
    state = torch.load(path, weights_only=True)
    model.load_state_dict(state)
    return model


# ---------------------------------------------------------------------------
# Задание 5 (интеграционное) — set_mode + run_reproducible_experiment
# ---------------------------------------------------------------------------


def set_mode(model: torch.nn.Module, train: bool) -> torch.nn.Module:
    """Переключить модель в режим обучения или инференса.

    Контракт:
      - train=True  → model.train()  (dropout активен, BN по батчу);
      - train=False → model.eval()   (dropout выкл, BN по накопленному среднему);
      - вернуть ту же самую model (для удобства цепочки вызовов);
      - после вызова model.training обязан быть равен аргументу train.

    ПОЧЕМУ это важно: классическая ошибка — мерить качество, забыв model.eval(),
    тогда dropout режет активации и метрики прыгают. И зеркальная — обучать, забыв
    вернуть model.train() после валидации.

    Пример:
        set_mode(model, True)   # → model.training is True
        set_mode(model, False)  # → model.training is False
    """
    if train:
        model.train()
    else:
        model.eval()
    return model


def run_reproducible_experiment(
    seed: int,
    n_features: int = 8,
    hidden: int = 16,
    n_steps: int = 5,
    lr: float = 0.01,
) -> dict:
    """Полный воспроизводимый эксперимент: сид → детерминированные метрики.

    Контракт:
      - зафиксировать ВСЕ три источника случайности через set_seed(seed);
      - построить крошечную двухслойную сеть (n_features → hidden → 1);
      - сгенерировать случайный батч (16 примеров × n_features признаков);
      - обучить модель n_steps шагов SGD (lr=lr, MSELoss) в режиме train;
      - после тренировки перевести в eval-режим и посчитать итоговый loss
        (forward без градиентов, тот же батч x/y);
      - вернуть словарь с ключами:
            "final_loss"     : float — финальный MSE-loss после n_steps шагов,
            "n_params"       : int   — число обучаемых параметров сети,
            "training_mode"  : bool  — model.training после завершения
                                      (должно быть False — eval);
      - ДЕТЕРМИНИЗМ: два вызова с одинаковым seed ОБЯЗАНЫ вернуть одинаковые
        числа (включая final_loss до последнего бита);
      - РАЗЛИЧИМОСТЬ: два вызова с разными seed С ВЫСОКОЙ ВЕРОЯТНОСТЬЮ дают
        разный final_loss (разная инициализация → разная траектория).

    Пример:
        r1 = run_reproducible_experiment(0)
        r2 = run_reproducible_experiment(0)
        assert r1["final_loss"] == r2["final_loss"]   # детерминизм
        assert r1["training_mode"] == False            # eval после завершения
    """
    set_seed(seed)

    # Построить модель
    model = nn.Sequential(
        nn.Linear(n_features, hidden),
        nn.ReLU(),
        nn.Linear(hidden, 1),
    )
    set_mode(model, True)  # train режим для обучения

    # Сгенерировать данные (после сида — детерминировано)
    x = torch.randn(16, n_features)
    y = torch.randn(16, 1)

    # Обучить n_steps шагов
    optimizer = optim.SGD(model.parameters(), lr=lr)
    criterion = nn.MSELoss()

    for _ in range(n_steps):
        optimizer.zero_grad()
        pred = model(x)
        loss = criterion(pred, y)
        loss.backward()
        optimizer.step()

    # Перейти в eval и посчитать финальный loss
    set_mode(model, False)
    with torch.no_grad():
        final_pred = model(x)
        final_loss = criterion(final_pred, y).item()

    return {
        "final_loss": final_loss,
        "n_params": count_parameters(model),
        "training_mode": model.training,
    }
