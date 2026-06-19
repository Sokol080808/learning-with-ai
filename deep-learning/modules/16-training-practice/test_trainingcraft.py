# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 16 — Обучение на практике. Тесты детерминированы (сиды зафиксированы),
# крошечные и идут на CPU.

import math

import numpy as np  # noqa: F401  (часть курса на numpy; здесь — для единообразия импортов)
import torch
import torch.nn.functional as F

from trainingcraft import layernorm, init_scale, lr_at_step


# ---------------------------------------------------------------------------
# layernorm: совпадает с torch.nn.functional.layer_norm (главная проверка модуля)
# ---------------------------------------------------------------------------

def test_layernorm_matches_torch_reference_2d():
    torch.manual_seed(0)
    D = 5
    x = torch.randn(3, D)
    gamma = torch.randn(D)
    beta = torch.randn(D)
    out = layernorm(x, gamma, beta, eps=1e-5)
    ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
    assert out.shape == x.shape
    assert torch.allclose(out, ref, atol=1e-6)


def test_layernorm_matches_torch_reference_3d():
    # форма (N, T, D): нормируем по последней оси D, остальное не трогаем
    torch.manual_seed(1)
    N, T, D = 2, 4, 6
    x = torch.randn(N, T, D)
    gamma = torch.randn(D)
    beta = torch.randn(D)
    out = layernorm(x, gamma, beta, eps=1e-5)
    ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
    assert out.shape == (N, T, D)
    assert torch.allclose(out, ref, atol=1e-6)


def test_layernorm_identity_params_zero_mean_unit_std():
    # при gamma=1, beta=0 каждая строка должна иметь среднее ~0 и разброс ~1
    torch.manual_seed(2)
    D = 8
    x = torch.randn(4, D) * 3.0 + 5.0  # заведомо со сдвигом и масштабом
    gamma = torch.ones(D)
    beta = torch.zeros(D)
    out = layernorm(x, gamma, beta, eps=1e-5)
    means = out.mean(dim=-1)
    # смещённое стандартное отклонение по строкам
    stds = out.std(dim=-1, unbiased=False)
    assert torch.allclose(means, torch.zeros(4), atol=1e-5)
    assert torch.allclose(stds, torch.ones(4), atol=1e-3)


def test_layernorm_known_row_value():
    # строка [1, 2, 3]: mu=2, var=(1+0+1)/3≈0.6667, xhat≈[-1.2247, 0, 1.2247]
    x = torch.tensor([[1.0, 2.0, 3.0]])
    gamma = torch.ones(3)
    beta = torch.zeros(3)
    out = layernorm(x, gamma, beta, eps=1e-5)
    expected = torch.tensor([[-1.224744, 0.0, 1.224744]])
    assert torch.allclose(out, expected, atol=1e-4)


def test_layernorm_uses_biased_variance():
    # Явная проверка ловушки: эталон torch использует unbiased=False.
    # Если реализация возьмёт unbiased=True, совпадения с layer_norm НЕ будет.
    torch.manual_seed(3)
    D = 7
    x = torch.randn(5, D)
    gamma = torch.ones(D)
    beta = torch.zeros(D)
    out = layernorm(x, gamma, beta, eps=1e-5)
    ref = F.layer_norm(x, (D,), weight=gamma, bias=beta, eps=1e-5)
    assert torch.allclose(out, ref, atol=1e-6)


# ---------------------------------------------------------------------------
# init_scale: 1 / sqrt(fan_in)
# ---------------------------------------------------------------------------

def test_init_scale_known_values():
    assert math.isclose(init_scale(100), 0.1, rel_tol=1e-9)
    assert math.isclose(init_scale(4), 0.5, rel_tol=1e-9)
    assert math.isclose(init_scale(1), 1.0, rel_tol=1e-9)


def test_init_scale_returns_float():
    s = init_scale(16)
    assert isinstance(s, float)
    assert math.isclose(s, 0.25, rel_tol=1e-9)


def test_init_scale_decreases_with_fan_in():
    # больше входов -> меньше масштаб
    assert init_scale(256) < init_scale(64) < init_scale(16)


def test_init_scale_matches_formula():
    for fan_in in (3, 7, 50, 1024):
        assert math.isclose(init_scale(fan_in), 1.0 / math.sqrt(fan_in), rel_tol=1e-12)


# ---------------------------------------------------------------------------
# lr_at_step: warmup + спад, ключевые точки
# ---------------------------------------------------------------------------

def test_lr_warmup_start_is_small():
    # самый старт разогрева: step=0 -> 0
    assert math.isclose(lr_at_step(0.1, 0, 10, 100, schedule="cosine"), 0.0, abs_tol=1e-12)
    assert math.isclose(lr_at_step(0.1, 0, 10, 100, schedule="linear"), 0.0, abs_tol=1e-12)


def test_lr_warmup_is_linear_and_monotonic():
    base_lr, warmup, total = 0.2, 10, 100
    # середина разогрева: ровно половина пика
    assert math.isclose(lr_at_step(base_lr, 5, warmup, total), 0.1, rel_tol=1e-9)
    # warmup строго растёт
    vals = [lr_at_step(base_lr, s, warmup, total) for s in range(warmup + 1)]
    for a, b in zip(vals, vals[1:]):
        assert b > a


def test_lr_peak_at_warmup_equals_base_lr():
    # на step == warmup spад только начинается -> ровно base_lr (для обеих форм)
    assert math.isclose(lr_at_step(0.1, 10, 10, 100, schedule="cosine"), 0.1, rel_tol=1e-9)
    assert math.isclose(lr_at_step(0.1, 10, 10, 100, schedule="linear"), 0.1, rel_tol=1e-9)


def test_lr_zero_at_total():
    # на конце обучения lr доходит до нуля (обе формы)
    assert math.isclose(lr_at_step(0.1, 100, 10, 100, schedule="cosine"), 0.0, abs_tol=1e-9)
    assert math.isclose(lr_at_step(0.1, 100, 10, 100, schedule="linear"), 0.0, abs_tol=1e-9)


def test_lr_zero_beyond_total():
    # за пределами total тоже 0
    assert math.isclose(lr_at_step(0.1, 150, 10, 100), 0.0, abs_tol=1e-12)


def test_lr_decay_is_monotonic_decreasing():
    base_lr, warmup, total = 0.1, 10, 100
    for sched in ("linear", "cosine"):
        vals = [lr_at_step(base_lr, s, warmup, total, schedule=sched)
                for s in range(warmup, total + 1)]
        for a, b in zip(vals, vals[1:]):
            assert b <= a + 1e-12
        # и реально спустился (не застрял на пике)
        assert vals[-1] < vals[0]


def test_lr_decay_stays_within_bounds():
    base_lr, warmup, total = 0.3, 20, 200
    for sched in ("linear", "cosine"):
        for step in range(0, total + 1, 7):
            lr = lr_at_step(base_lr, step, warmup, total, schedule=sched)
            assert -1e-12 <= lr <= base_lr + 1e-9


def test_lr_cosine_midpoint_is_half_of_base():
    # косинус в середине спада (progress=0.5): 0.5*(1+cos(pi/2)) = 0.5 -> base_lr/2
    base_lr, warmup, total = 0.1, 0, 100
    mid = lr_at_step(base_lr, 50, warmup, total, schedule="cosine")
    assert math.isclose(mid, 0.05, rel_tol=1e-6)


# ---------------------------------------------------------------------------
# «Переобучи один батч»: цикл обучения на одном крошечном батче снижает loss.
# Если кирпичики и цикл собраны верно, итоговый loss < 0.5 * начального.
# ---------------------------------------------------------------------------

def test_overfit_single_batch_reduces_loss():
    torch.manual_seed(0)
    N, D_in, D_out = 8, 4, 3  # один крошечный батч из 8 примеров

    # один фиксированный батч (вход + случайные «правильные» ответы)
    x = torch.randn(N, D_in)
    target = torch.randn(N, D_out)

    # маленькая модель: линейный слой, веса инициализированы по init_scale(fan_in)
    scale = init_scale(D_in)
    W = (torch.randn(D_in, D_out) * scale).requires_grad_(True)
    b = torch.zeros(D_out, requires_grad=True)

    # обучаемые параметры LayerNorm по выходной размерности
    gamma = torch.ones(D_out, requires_grad=True)
    beta = torch.zeros(D_out, requires_grad=True)

    def forward():
        z = x @ W + b                 # (N, D_out)
        h = layernorm(z, gamma, beta) # нормализуем активации
        return h

    def loss_fn():
        pred = forward()
        return ((pred - target) ** 2).mean()

    params = [W, b, gamma, beta]
    initial_loss = loss_fn().item()

    base_lr, warmup, total = 0.2, 20, 300
    for step in range(total):
        loss = loss_fn()
        for p in params:
            p.grad = None
        loss.backward()
        lr = lr_at_step(base_lr, step, warmup, total, schedule="cosine")
        with torch.no_grad():
            for p in params:
                p -= lr * p.grad

    final_loss = loss_fn().item()
    assert final_loss < 0.5 * initial_loss
