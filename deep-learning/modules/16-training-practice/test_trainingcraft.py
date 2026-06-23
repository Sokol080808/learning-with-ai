# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 16 — Обучение на практике. Тесты детерминированы (сиды зафиксированы),
# крошечные и идут на CPU.

import math

import numpy as np  # noqa: F401  (часть курса на numpy; здесь — для единообразия импортов)
import torch
import torch.nn.functional as F

from trainingcraft import layernorm, init_scale, lr_at_step, clip_gradients


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


# ---------------------------------------------------------------------------
# clip_gradients: глобальная L2-норма; эталон — torch.nn.utils.clip_grad_norm_
# ---------------------------------------------------------------------------

def _make_params_with_grads(seed, shapes):
    """Список листовых тензоров с заданными .grad (детерминированно по seed)."""
    torch.manual_seed(seed)
    params = []
    for shp in shapes:
        p = torch.randn(*shp, requires_grad=True)
        p.grad = torch.randn(*shp)
        params.append(p)
    return params


def test_clip_gradients_returns_pre_clip_norm():
    # Возвращаемое значение — глобальная норма ДО обрезки.
    params = _make_params_with_grads(0, [(3, 4), (5,), (2, 2)])
    manual = torch.sqrt(sum((p.grad ** 2).sum() for p in params)).item()
    ret = clip_gradients(params, max_norm=1.0)
    assert isinstance(ret, float)
    assert math.isclose(ret, manual, rel_tol=1e-6)


def test_clip_gradients_matches_torch_clip_grad_norm():
    # Эталон: те же градиенты, прогнанные через torch.nn.utils.clip_grad_norm_.
    shapes = [(4, 3), (6,), (2, 5)]
    ours = _make_params_with_grads(1, shapes)
    ref = _make_params_with_grads(1, shapes)   # тот же seed -> те же grad

    ret_ours = clip_gradients(ours, max_norm=1.0)
    ret_ref = torch.nn.utils.clip_grad_norm_(ref, max_norm=1.0)

    # совпадает возвращённая норма ДО обрезки
    assert math.isclose(ret_ours, float(ret_ref), rel_tol=1e-5)
    # и совпадают все градиенты ПОСЛЕ обрезки
    for a, b in zip(ours, ref):
        assert torch.allclose(a.grad, b.grad, atol=1e-5)


def test_clip_gradients_passthrough_when_norm_small():
    # Если глобальная норма уже <= max_norm, градиенты не меняются (бит-в-бит).
    params = _make_params_with_grads(2, [(3,), (4,)])
    before = [p.grad.clone() for p in params]
    # ставим заведомо большой порог
    clip_gradients(params, max_norm=1000.0)
    for p, g0 in zip(params, before):
        assert torch.equal(p.grad, g0)


def test_clip_gradients_caps_global_norm():
    # После обрезки глобальная норма не превышает max_norm (с малым допуском).
    params = _make_params_with_grads(3, [(5, 5), (7,), (3, 2)])
    max_norm = 0.5
    clip_gradients(params, max_norm=max_norm)
    new_norm = torch.sqrt(sum((p.grad ** 2).sum() for p in params)).item()
    assert new_norm <= max_norm + 1e-5


def test_clip_gradients_preserves_direction():
    # Обрезка по норме сохраняет направление каждого градиента.
    params = _make_params_with_grads(4, [(6,), (4, 3)])
    before = [p.grad.clone() for p in params]
    clip_gradients(params, max_norm=0.1)   # порог мал -> обрезка точно сработает
    for p, g0 in zip(params, before):
        dir_after = p.grad / p.grad.norm()
        dir_before = g0 / g0.norm()
        assert torch.allclose(dir_after, dir_before, atol=1e-5)


def _train_with_grad_spikes(use_clip, steps=60, lr=0.3):
    """Обучение простой квадратичной задачи w -> 0, где раз в 7 шагов в градиент
    влетает «выброс» (×1e4) — модель взрыва градиента. Возвращает (init, final).

    Без клиппинга один такой выброс при lr·grad улетает в бесконечность.
    С клиппингом по норме длина шага ограничена, и обучение остаётся стабильным.
    """
    torch.manual_seed(0)
    w = torch.tensor([5.0], requires_grad=True)
    target = torch.tensor([0.0])

    def compute_loss():
        return 0.5 * ((w - target) ** 2).sum()

    initial = compute_loss().item()
    for t in range(steps):
        loss = compute_loss()
        w.grad = None
        loss.backward()
        if t % 7 == 0:           # редкий «взрыв» градиента
            w.grad.mul_(1e4)
        if use_clip:
            clip_gradients([w], max_norm=1.0)
        with torch.no_grad():
            w -= lr * w.grad
    return initial, compute_loss().item()


def test_clip_gradients_prevents_divergence_on_grad_spikes():
    # Без клиппинга редкие выбросы градиента разносят обучение в inf;
    # с клиппингом по норме оно остаётся конечным и снижает loss.
    init_clip, final_clip = _train_with_grad_spikes(use_clip=True)
    init_noclip, final_noclip = _train_with_grad_spikes(use_clip=False)

    # без клиппинга — взрыв (не конечно)
    assert not math.isfinite(final_noclip), (
        f"без клиппинга ожидался взрыв (inf/nan), получили {final_noclip}"
    )
    # с клиппингом — стабильно и реально сошлось
    assert math.isfinite(final_clip), "с клиппингом loss не должен взрываться"
    assert final_clip < init_clip, (
        f"с клиппингом loss должен снизиться: {init_clip} -> {final_clip}"
    )


def test_clip_gradients_keeps_training_stable_with_large_lr():
    # Интеграция в стиле модуля: с клиппингом большой lr остаётся управляемым —
    # обучение крошечной модели (Linear + LayerNorm) конечно и снижает loss.
    torch.manual_seed(0)
    N, D_in, D_out = 8, 4, 3
    x = torch.randn(N, D_in)
    target = torch.randn(N, D_out)

    scale = init_scale(D_in)
    W = (torch.randn(D_in, D_out) * scale).requires_grad_(True)
    b = torch.zeros(D_out, requires_grad=True)
    gamma = torch.ones(D_out, requires_grad=True)
    beta = torch.zeros(D_out, requires_grad=True)
    params = [W, b, gamma, beta]

    def compute_loss():
        z = x @ W + b
        h = layernorm(z, gamma, beta)
        return ((h - target) ** 2).mean()

    initial = compute_loss().item()
    big_lr = 0.5   # крупный шаг, который клиппинг держит под контролем
    for _ in range(200):
        loss = compute_loss()
        for p in params:
            p.grad = None
        loss.backward()
        clip_gradients(params, max_norm=1.0)
        with torch.no_grad():
            for p in params:
                p -= big_lr * p.grad

    final = compute_loss().item()
    assert math.isfinite(final), "loss взорвался (inf/nan) несмотря на клиппинг"
    assert final < initial, f"loss не снизился при стабилизированном обучении: {initial} -> {final}"
