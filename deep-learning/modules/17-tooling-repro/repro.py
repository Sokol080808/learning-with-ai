# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 17 стали зелёными.
#
# Модуль 17 — Воспроизводимость и инженерия.
# Здесь ты делаешь эксперимент повторяемым: фиксируешь ВСЕ источники случайности,
# считаешь размер модели и умеешь сохранять/загружать веса через state_dict.
# Весь модуль — на CPU (никаких GPU).

import random  # noqa: F401  (пригодится: один из трёх генераторов случайности)

import numpy as np  # noqa: F401  (пригодится: второй генератор случайности)
import torch


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
