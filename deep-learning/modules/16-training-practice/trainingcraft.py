# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй так, чтобы тесты модуля 16 стали зелёными.
#
# Модуль 16 — Обучение на практике.
# Четыре кирпичика ремесла обучения: инициализация весов, нормализация (LayerNorm),
# приём отладки «переобучи один батч» и расписание скорости обучения (warmup + спад).
# Весь модуль — на CPU, на PyTorch, во float32.

import math  # noqa: F401  (пригодится: sqrt, cos, pi для масштаба и расписания lr)

import torch


def layernorm(
    x: torch.Tensor,
    gamma: torch.Tensor,
    beta: torch.Tensor,
    eps: float = 1e-5,
) -> torch.Tensor:
    """LayerNorm: нормализация по ПОСЛЕДНЕЙ оси + обучаемые масштаб и сдвиг.

    Контракт:
      - x: тензор формы (..., D) — нормируем КАЖДЫЙ вектор по последней оси длины D;
      - gamma, beta: тензоры формы (D,) — обучаемые масштаб (γ) и сдвиг (β);
      - eps: добавка под корнем для устойчивости (по умолчанию 1e-5);
      - результат той же формы, что x.

    Формула (по последней оси):
        mu  = mean(x)                 # форма (..., 1)
        var = var(x, unbiased=False)  # СМЕЩЁННАЯ дисперсия (делим на D), форма (..., 1)
        xhat = (x - mu) / sqrt(var + eps)
        out  = xhat * gamma + beta

    ВАЖНО: дисперсию бери СМЕЩЁННУЮ (unbiased=False) — именно так считает
    torch.nn.functional.layer_norm. С unbiased=True результат НЕ совпадёт с эталоном.

    Пример (строка x = [1, 2, 3], gamma=1, beta=0):
        mu = 2; var = (1+0+1)/3 ≈ 0.667; xhat ≈ [-1.225, 0, 1.225]
    """
    mu = x.mean(dim=-1, keepdim=True)
    var = x.var(dim=-1, unbiased=False, keepdim=True)
    xhat = (x - mu) / torch.sqrt(var + eps)
    return xhat * gamma + beta


def init_scale(fan_in: int) -> float:
    """Масштаб инициализации весов: 1 / sqrt(fan_in).

    Контракт:
      - fan_in: число входов нейрона (размер «веера» входящих связей), целое > 0;
      - вернуть обычный float = 1 / sqrt(fan_in).

    ПОЧЕМУ так: при fan_in входах с разбросом ~1 и весах с дисперсией σ² дисперсия
    суммы z равна fan_in·σ². Чтобы Var(z) ≈ 1 (сигнал не раздувается и не затухает),
    берём σ = 1/sqrt(fan_in).

    Пример:
        init_scale(100) -> 0.1
        init_scale(4)   -> 0.5
    """
    return 1.0 / math.sqrt(fan_in)


def lr_at_step(
    base_lr: float,
    step: int,
    warmup: int,
    total: int,
    schedule: str = "cosine",
) -> float:
    """Скорость обучения на шаге step: линейный warmup, затем спад.

    Контракт:
      - base_lr: пиковая скорость обучения (достигается на step == warmup);
      - step: номер текущего шага (0, 1, 2, ...);
      - warmup: длительность разогрева в шагах (может быть 0 — тогда разогрева нет);
      - total: общее число шагов (на step >= total вернуть 0.0);
      - schedule: "linear" или "cosine" — форма спада после разогрева;
      - вернуть float — lr на этом шаге.

    Поведение по шагам:
      1) warmup: если step < warmup -> lr = base_lr * step / warmup  (растёт 0 → base_lr);
      2) конец:  если step >= total -> lr = 0.0;
      3) спад:   progress = (step - warmup) / (total - warmup)  (число 0 → 1), затем
                 "linear": lr = base_lr * (1 - progress)
                 "cosine": lr = base_lr * 0.5 * (1 + cos(pi * progress))

    Ключевые точки: на step == warmup получается lr == base_lr (пик, progress=0);
    на step == total получается lr == 0.0 (progress=1) для обеих форм спада.

    Пример (base_lr=0.1, warmup=10, total=100):
        lr_at_step(0.1, 0,   10, 100) -> 0.0     # самый старт разогрева
        lr_at_step(0.1, 5,   10, 100) -> 0.05    # середина разогрева
        lr_at_step(0.1, 10,  10, 100) -> 0.1     # пик
        lr_at_step(0.1, 100, 10, 100) -> 0.0     # конец
    """
    if step >= total:
        return 0.0
    if warmup > 0 and step < warmup:
        return base_lr * step / warmup
    # decay phase
    if total == warmup:
        # edge case: no decay region
        return base_lr
    progress = (step - warmup) / (total - warmup)
    if schedule == "linear":
        return base_lr * (1.0 - progress)
    else:  # cosine
        return base_lr * 0.5 * (1.0 + math.cos(math.pi * progress))
