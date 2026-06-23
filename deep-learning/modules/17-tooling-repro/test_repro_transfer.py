# Тесты для Задания 7 модуля 17 (кросс-тема) — перенос обучения:
# freeze_backbone / replace_head / transfer_finetune.
#
# Rules:
#   - Все вызовы новых функций — ВНУТРИ тест-функций (чистая сборка против стаба).
#   - Оракул: torch / прямой пересчёт — никогда не функции под тестом.
#   - «Предобученная» модель = крошечный backbone, построенный и сохранённый
#     ЛОКАЛЬНО в тесте через save_model. НИКАКИХ загрузок из интернета, всё на CPU.
#   - Сиды фиксированы (set_seed) → два прогона бит-в-бит совпадают.

import os

import torch
import torch.nn as nn
import torch.optim as optim

from repro import (
    set_seed,
    save_model,
    count_parameters,
    freeze_backbone,
    replace_head,
    transfer_finetune,
)
from repro import _build_backbone_skeleton


# ---------------------------------------------------------------------------
# Хелперы. Архитектура донора берётся из ТОГО ЖЕ _build_backbone_skeleton, что
# использует transfer_finetune — иначе load_model не сможет влить state_dict.
#   тело  = [Linear(6,8), ReLU]   (первые 2 модуля)
#   голова = Linear(8,2)          (последний модуль)
# ---------------------------------------------------------------------------

def _save_donor_backbone(tmp_path, seed=0):
    """Построить, (псевдо)обучить и сохранить крошечный backbone; вернуть путь."""
    set_seed(seed)
    donor = _build_backbone_skeleton()
    # «предобучим» на шуме, чтобы веса были нетривиальными
    x = torch.randn(12, 6)
    y = torch.randint(0, 2, (12,))
    opt = optim.SGD(donor.parameters(), lr=0.1)
    crit = nn.CrossEntropyLoss()
    for _ in range(5):
        opt.zero_grad()
        crit(donor(x), y).backward()
        opt.step()
    path = os.path.join(str(tmp_path), "backbone.pt")
    save_model(donor, path)
    return path, donor


def _toy_classification(seed, n_classes):
    set_seed(seed)
    x = torch.randn(24, 6)
    y = torch.randint(0, n_classes, (24,))
    return x, y


# ---------------------------------------------------------------------------
# freeze_backbone: тело заморожено, голова обучаема, count_parameters сходится
# ---------------------------------------------------------------------------

def test_freeze_backbone_keeps_only_tail_trainable():
    set_seed(0)
    model = _build_backbone_skeleton()      # [Linear(6,8), ReLU, Linear(8,2)]
    freeze_backbone(model, n_keep_trainable_tail=1)

    # тело (Linear(6,8)) — заморожено
    for p in model[0].parameters():
        assert p.requires_grad is False
    # голова (Linear(8,2)) — обучаема
    for p in model[-1].parameters():
        assert p.requires_grad is True


def test_freeze_backbone_param_count_oracle():
    """count_parameters после заморозки == ровно numel() параметров головы."""
    set_seed(1)
    model = _build_backbone_skeleton()
    freeze_backbone(model, n_keep_trainable_tail=1)

    head = model[-1]                        # Linear(8,2): 8*2 + 2 = 18
    head_params = sum(p.numel() for p in head.parameters())
    assert count_parameters(model) == head_params == 18

    total = sum(p.numel() for p in model.parameters())
    frozen = total - count_parameters(model)
    assert count_parameters(model) + frozen == total


def test_freeze_backbone_returns_model():
    set_seed(2)
    model = _build_backbone_skeleton()
    returned = freeze_backbone(model, n_keep_trainable_tail=1)
    assert returned is model


def test_frozen_params_get_no_grad_head_does():
    """После backward замороженные параметры имеют .grad is None, голова — нет."""
    set_seed(3)
    model = _build_backbone_skeleton()
    freeze_backbone(model, n_keep_trainable_tail=1)

    x, y = _toy_classification(4, n_classes=2)
    loss = nn.CrossEntropyLoss()(model(x), y)
    loss.backward()

    for p in model[0].parameters():         # тело
        assert p.grad is None
    for p in model[-1].parameters():        # голова
        assert p.grad is not None
        assert not torch.allclose(p.grad, torch.zeros_like(p.grad))


# ---------------------------------------------------------------------------
# replace_head: новая голова с нужным числом выходов, in_features сохранён
# ---------------------------------------------------------------------------

def test_replace_head_changes_out_features():
    set_seed(5)
    model = _build_backbone_skeleton()      # голова Linear(8, 2)
    old_in = model[-1].in_features
    replace_head(model, new_out=3)
    assert isinstance(model[-1], nn.Linear)
    assert model[-1].out_features == 3
    assert model[-1].in_features == old_in  # признаки тела не изменились


def test_replace_head_forward_shape_oracle():
    """После replace_head(K) forward(x) даёт логиты формы (N, K)."""
    set_seed(6)
    model = _build_backbone_skeleton()
    replace_head(model, new_out=4)
    x = torch.randn(10, 6)
    with torch.no_grad():
        out = model(x)
    assert out.shape == (10, 4)


def test_replace_head_is_freshly_initialized():
    """Новая голова — со свежими (другими) весами, не копия старой."""
    set_seed(7)
    model = _build_backbone_skeleton()
    old_head_w = model[-1].weight.detach().clone()
    replace_head(model, new_out=2)          # то же число выходов, но новый слой
    new_head_w = model[-1].weight
    assert not torch.allclose(old_head_w, new_head_w)


def test_replace_head_leaves_backbone_untouched():
    set_seed(8)
    model = _build_backbone_skeleton()
    body_w_before = model[0].weight.detach().clone()
    replace_head(model, new_out=5)
    assert torch.equal(model[0].weight, body_w_before)


# ---------------------------------------------------------------------------
# transfer_finetune: главный сценарий feature-extraction
# ---------------------------------------------------------------------------

def test_transfer_backbone_load_roundtrip(tmp_path):
    """Загруженный backbone поэлементно совпадает с сохранённым донором."""
    path, donor = _save_donor_backbone(tmp_path, seed=0)
    # построим каркас и загрузим — должно совпасть с донором
    from repro import load_model
    skeleton = _build_backbone_skeleton()
    load_model(skeleton, path)
    for (n_d, p_d), (n_s, p_s) in zip(
        donor.state_dict().items(), skeleton.state_dict().items()
    ):
        assert n_d == n_s
        assert torch.allclose(p_d, p_s, atol=0)


def test_transfer_finetune_backbone_unchanged(tmp_path):
    """СЕРДЦЕ переноса обучения: веса тела бит-в-бит не меняются за тренировку."""
    path, donor = _save_donor_backbone(tmp_path, seed=0)
    x, y = _toy_classification(seed=42, n_classes=3)
    out = transfer_finetune(path, x, y, new_out=3, n_steps=20, lr=0.05)
    assert out["backbone_unchanged"] is True


def test_transfer_finetune_param_count_oracle(tmp_path):
    """trainable_params == numel головы; trainable + frozen == все параметры."""
    path, donor = _save_donor_backbone(tmp_path, seed=0)
    x, y = _toy_classification(seed=43, n_classes=3)
    out = transfer_finetune(path, x, y, new_out=3, n_steps=5, lr=0.05)

    # голова: Linear(8, 3) = 8*3 + 3 = 27
    assert out["trainable_params"] == 27
    # тело: Linear(6,8) = 6*8+8 = 56, ReLU = 0
    assert out["frozen_params"] == 56
    assert out["trainable_params"] + out["frozen_params"] == 27 + 56


def test_transfer_finetune_loss_decreases(tmp_path):
    path, donor = _save_donor_backbone(tmp_path, seed=0)
    x, y = _toy_classification(seed=44, n_classes=3)
    out = transfer_finetune(path, x, y, new_out=3, n_steps=40, lr=0.1)
    assert out["loss_decreased"] is True


def test_transfer_finetune_deterministic(tmp_path):
    """Два прогона из одного сида/донора дают бит-в-бит одинаковую диагностику."""
    path, donor = _save_donor_backbone(tmp_path, seed=0)
    x, y = _toy_classification(seed=45, n_classes=2)
    out1 = transfer_finetune(path, x, y, new_out=2, n_steps=15, lr=0.05)
    out2 = transfer_finetune(path, x, y, new_out=2, n_steps=15, lr=0.05)
    assert out1 == out2


def test_transfer_finetune_returns_required_keys(tmp_path):
    path, donor = _save_donor_backbone(tmp_path, seed=0)
    x, y = _toy_classification(seed=46, n_classes=2)
    out = transfer_finetune(path, x, y, new_out=2, n_steps=5, lr=0.05)
    for key in ("trainable_params", "frozen_params", "backbone_unchanged", "loss_decreased"):
        assert key in out
    assert isinstance(out["trainable_params"], int)
    assert isinstance(out["frozen_params"], int)
    assert isinstance(out["backbone_unchanged"], bool)
    assert isinstance(out["loss_decreased"], bool)
