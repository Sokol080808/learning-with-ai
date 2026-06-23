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
    raise NotImplementedError("TODO: засидируй random, numpy и torch одним вызовом")


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
    raise NotImplementedError("TODO: верни сумму numel() по обучаемым параметрам")


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
    raise NotImplementedError("TODO: torch.save(model.state_dict(), path)")


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
    raise NotImplementedError("TODO: загрузи state_dict из path в model и верни model")


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
    raise NotImplementedError("TODO: переключи model в train/eval по флагу train и верни model")


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
    raise NotImplementedError("TODO: засидируй, построй сеть, обучи n_steps шагов, верни {final_loss, n_params, training_mode}")


# ---------------------------------------------------------------------------
# Задание 6 — чекпоинты для возобновления обучения (save/load_checkpoint)
# ---------------------------------------------------------------------------


def save_checkpoint(
    model: torch.nn.Module,
    optimizer: torch.optim.Optimizer,
    epoch: int,
    path: str,
) -> None:
    """Сохранить ПОЛНЫЙ чекпоинт обучения (а не только веса) в файл path.

    Контракт:
      - сохранить словарь
            {
                "model":     model.state_dict(),
                "optimizer": optimizer.state_dict(),
                "epoch":     epoch,
            }
        одним вызовом torch.save;
      - ничего не возвращать (None).

    ПОЧЕМУ сохраняем state_dict ОПТИМИЗАТОРА, а не только модели: у моментум-SGD
    и Adam есть внутренние буферы (накопленный моментум; у Adam — оценки m и v
    первого и второго моментов). Это «память» оптимизатора о прошлых шагах. Если
    при возобновлении залить только веса, оптимизатор стартует «с холода»:
    моментум обнулён, адаптивные шаги Adam сброшены, и траектория loss поедет не
    туда, куда поехала бы непрерывная тренировка. Поэтому faithful resume =
    веса + состояние оптимизатора + счётчик epoch/step.

    Пример:
        save_checkpoint(model, optimizer, epoch=3, path="ckpt.pt")
    """
    raise NotImplementedError("TODO: сохрани dict с 'model', 'optimizer', 'epoch' через torch.save")


def load_checkpoint(
    model: torch.nn.Module,
    optimizer: torch.optim.Optimizer,
    path: str,
) -> int:
    """Загрузить чекпоинт из path в model и optimizer; вернуть сохранённый epoch.

    Контракт:
      - model и optimizer — уже готовые объекты НУЖНОЙ архитектуры/типа
        (созданы до вызова, как и в load_model);
      - прочитать чекпоинт-словарь из path;
      - влить ckpt["model"] в model через model.load_state_dict(...);
      - влить ckpt["optimizer"] в optimizer через optimizer.load_state_dict(...);
      - вернуть ckpt["epoch"] как int.

    После корректной загрузки и веса модели, и состояние оптимизатора поэлементно
    совпадают с сохранёнными — поэтому продолженная тренировка идёт по той же
    траектории, что и непрерывная (проверяется оракулом «прерванный == непрерывный»).

    Пример:
        model = build_skeleton()              # тот же каркас архитектуры
        optimizer = optim.SGD(model.parameters(), lr=0.1, momentum=0.9)
        start_epoch = load_checkpoint(model, optimizer, "ckpt.pt")
        # продолжаем с epoch = start_epoch + 1
    """
    raise NotImplementedError("TODO: загрузи ckpt, влей 'model' и 'optimizer', верни int(ckpt['epoch'])")


# ---------------------------------------------------------------------------
# Задание 7 (кросс-тема) — перенос обучения: freeze / replace_head / fine-tune
# ---------------------------------------------------------------------------


def freeze_backbone(
    model: nn.Sequential,
    n_keep_trainable_tail: int,
) -> torch.nn.Module:
    """Заморозить «тело» сети, оставив обучаемым только хвост из n модулей.

    Контракт:
      - model — это nn.Sequential (стек подмодулей);
      - заморозить (requires_grad_(False)) ВСЕ параметры, КРОМЕ тех, что
        принадлежат последним n_keep_trainable_tail подмодулям («голове»);
      - параметры головы оставить обучаемыми (requires_grad_(True));
      - вернуть ту же model.

    ПОЧЕМУ это работает: у замороженного параметра requires_grad=False, значит
    после loss.backward() у него .grad is None (он не попал в граф), и шаг
    оптимизатора его не двигает — вес остаётся бит-в-бит прежним. Число
    обучаемых параметров (count_parameters из Идеи 2) при заморозке тела резко
    падает — это и есть «дёшево»: учим только тонкую голову.

    Пример:
        # backbone = первые len-1 модулей, голова = последний Linear
        freeze_backbone(model, n_keep_trainable_tail=1)
        # теперь обучаем только последний слой
    """
    raise NotImplementedError("TODO: заморозь первые (len-n) модулей, оставь обучаемыми последние n, верни model")


def replace_head(model: nn.Sequential, new_out: int) -> torch.nn.Module:
    """Заменить последний nn.Linear свежим nn.Linear с новым числом выходов.

    Контракт:
      - найти ПОСЛЕДНИЙ nn.Linear в model (это «голова» под старую задачу);
      - заменить его на новый nn.Linear(in_features=old.in_features,
        out_features=new_out) со СВЕЖЕЙ случайной инициализацией;
      - остальные модули не трогать;
      - вернуть ту же model.

    ПОЧЕМУ новый слой со случайными весами: старая голова обучена под старые
    классы; под новую задачу (другое число классов new_out) нужна чистая голова,
    которую мы и будем учить. in_features берём у старой головы — размерность
    признаков, которые выдаёт тело, не меняется.

    Пример:
        replace_head(model, new_out=3)   # голова теперь выдаёт логиты на 3 класса
        # model(x).shape[-1] == 3
    """
    raise NotImplementedError("TODO: найди последний nn.Linear, замени на nn.Linear(old.in_features, new_out), верни model")


def transfer_finetune(
    backbone_path: str,
    x: torch.Tensor,
    y: torch.Tensor,
    new_out: int,
    n_steps: int = 20,
    lr: float = 0.05,
) -> dict:
    """Полный сценарий feature-extraction: загрузить тело → новая голова →
    заморозить тело → учить только голову, и вернуть диагностику.

    Контракт:
      - построить СВЕЖИЙ каркас той же архитектуры, что у донора, и загрузить в
        него сохранённый backbone state_dict через load_model(skeleton,
        backbone_path) — никаких загрузок из интернета, «предобученная» модель =
        крошечный backbone, который мы сами обучили и сохранили save_model;
      - replace_head(model, new_out) — поставить свежую голову на new_out классов;
      - freeze_backbone(model, n_keep_trainable_tail=1) — заморозить всё, кроме
        головы (последнего модуля);
      - обучить n_steps шагов SGD + CrossEntropyLoss на крошечных (x, y);
      - вернуть словарь:
            {
                "trainable_params":   int,   # обучаемых параметров (только голова),
                "frozen_params":      int,   # замороженных параметров (тело),
                "backbone_unchanged": bool,  # веса тела бит-в-бит = весам донора,
                "loss_decreased":     bool,  # loss в конце < loss в начале,
            }

    ПОЧЕМУ backbone_unchanged обязан быть True: в этом вся суть переноса обучения
    при feature-extraction — тело заморожено, его веса не двигаются, а адаптация
    под новую задачу происходит ТОЛЬКО в голове. Проверяем это, сравнивая веса
    тела после обучения с весами, которые лежали в файле донора.

    Пример:
        out = transfer_finetune("backbone.pt", x, y, new_out=3, n_steps=30)
        assert out["backbone_unchanged"] is True
        assert out["loss_decreased"] is True
    """
    raise NotImplementedError("TODO: _build_backbone_skeleton → load_model → replace_head → freeze_backbone → обучи голову → верни dict")


def _build_backbone_skeleton() -> nn.Sequential:
    """Каркас крошечного «backbone» для переноса обучения.

    Архитектура донора и каркаса-приёмника обязаны совпадать, иначе load_model не
    сможет влить state_dict. Эта функция — единственный источник правды об этой
    архитектуре: и тест-донор, и transfer_finetune строят сеть через неё.

    Форма: вход 6 признаков → тело (Linear 6→8, ReLU) → голова Linear 8→2.
    «Тело» — это первые два модуля; «голова» — последний Linear.
    """
    return nn.Sequential(
        nn.Linear(6, 8),
        nn.ReLU(),
        nn.Linear(8, 2),
    )
