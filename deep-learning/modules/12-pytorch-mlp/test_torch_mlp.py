# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 12 — MLP на PyTorch: nn.Module и обучение. Тесты детерминированы
# (сиды зафиксированы), крошечные и идут на CPU. Никакой сети и загрузок данных —
# игрушечные синтетические данные генерируются прямо в тесте.

import numpy as np
import torch
import torch.nn as nn

from torch_mlp import MLP, train_step, train_epoch, accuracy


# ---------------------------------------------------------------------------
# Вспомогательное: крошечная, seeded и хорошо разделимая задача классификации.
# Два «облака» точек вокруг разных центров -> 2 (или больше) класса.
# ---------------------------------------------------------------------------

def _toy_classification(n_per_class: int = 30, n_classes: int = 3, in_dim: int = 4, seed: int = 0):
    """Возвращает (X, y): X формы (N, in_dim) float32, y формы (N,) int64.

    Каждый класс — облако точек вокруг своего случайного центра. Классы хорошо
    разделимы, поэтому MLP обязан обучиться до высокой accuracy за десятки эпох.
    """
    torch.manual_seed(seed)
    centers = 4.0 * torch.randn(n_classes, in_dim)   # далёкие друг от друга центры
    xs, ys = [], []
    for c in range(n_classes):
        cloud = centers[c] + 0.5 * torch.randn(n_per_class, in_dim)
        xs.append(cloud)
        ys.append(torch.full((n_per_class,), c, dtype=torch.long))
    X = torch.cat(xs, dim=0)
    y = torch.cat(ys, dim=0)
    return X, y


# ---------------------------------------------------------------------------
# MLP: это nn.Module, у него есть обучаемые параметры, формы forward сходятся.
# ---------------------------------------------------------------------------

def test_mlp_is_nn_module():
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    assert isinstance(model, nn.Module)


def test_mlp_forward_shape():
    torch.manual_seed(0)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    X = torch.randn(5, 4)               # (N=5, in_dim=4)
    logits = model(X)
    assert tuple(logits.shape) == (5, 3)   # (N, out_dim)


def test_mlp_has_trainable_parameters():
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    params = list(model.parameters())
    assert len(params) > 0
    # все параметры должны требовать градиент (готовы обучаться)
    assert all(p.requires_grad for p in params)


def test_mlp_parameter_count_matches_two_linear_layers():
    # Два слоя: Linear(4->8): 4*8 + 8 = 40; Linear(8->3): 8*3 + 3 = 27; итого 67.
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    total = sum(p.numel() for p in model.parameters())
    assert total == 67


def test_mlp_forward_does_not_apply_softmax():
    # Логиты — сырые числа; softmax дал бы строки, суммирующиеся в 1 и неотрицательные.
    # Проверяем, что это НЕ так (хотя бы один отрицательный логит ИЛИ сумма строки != 1).
    torch.manual_seed(1)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    X = torch.randn(6, 4)
    logits = model(X)
    row_sums = logits.sum(dim=1)
    looks_like_probs = torch.all(logits >= 0) and torch.allclose(
        row_sums, torch.ones_like(row_sums), atol=1e-4
    )
    assert not looks_like_probs


# ---------------------------------------------------------------------------
# train_step: возвращает float, заполняет градиенты, реально двигает параметры.
# ---------------------------------------------------------------------------

def test_train_step_returns_float():
    torch.manual_seed(0)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_classification()
    loss = train_step(model, optimizer, loss_fn, X, y)
    assert isinstance(loss, float)
    assert loss > 0.0


def test_train_step_updates_parameters():
    # После шага хотя бы один параметр должен измениться.
    torch.manual_seed(0)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_classification()

    before = [p.detach().clone() for p in model.parameters()]
    train_step(model, optimizer, loss_fn, X, y)
    after = list(model.parameters())

    changed = any(not torch.allclose(b, a) for b, a in zip(before, after))
    assert changed


def test_train_step_populates_gradients():
    # После шага у параметров должен появиться .grad (backward отработал).
    torch.manual_seed(0)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_classification()

    train_step(model, optimizer, loss_fn, X, y)
    assert any(p.grad is not None for p in model.parameters())


# ---------------------------------------------------------------------------
# accuracy: число в [0, 1]; на идеальном предсказании = 1.0.
# ---------------------------------------------------------------------------

def test_accuracy_in_unit_range():
    torch.manual_seed(0)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    X, y = _toy_classification()
    acc = accuracy(model, X, y)
    assert isinstance(acc, float)
    assert 0.0 <= acc <= 1.0


def test_accuracy_perfect_when_model_is_right():
    # Сконструируем случай, где argmax логитов заведомо совпадает с y.
    # Берём out_dim=3 и подаём X, по которому модель... — проще проверить на готовой
    # модели после обучения (ниже). Здесь проверим граничный случай: если все метки
    # совпадают с предсказанием, accuracy == 1.0. Используем монотонную игрушку.
    torch.manual_seed(0)

    class _Identity3(nn.Module):
        # Возвращает X как логиты; argmax по строке = «истинный» класс.
        def __init__(self):
            super().__init__()

        def forward(self, x):
            return x

    model = _Identity3()
    # Логиты, где максимум в каждой строке стоит в позиции метки.
    X = torch.tensor(
        [[5.0, 0.0, 0.0],   # класс 0
         [0.0, 5.0, 0.0],   # класс 1
         [0.0, 0.0, 5.0]],  # класс 2
    )
    y = torch.tensor([0, 1, 2])
    acc = accuracy(model, X, y)
    assert acc == 1.0


def test_accuracy_half_when_half_right():
    torch.manual_seed(0)

    class _Identity(nn.Module):
        def __init__(self):
            super().__init__()

        def forward(self, x):
            return x

    model = _Identity()
    # 4 строки: первые две предсказаны верно, последние две — неверно.
    X = torch.tensor(
        [[2.0, 0.0],   # argmax=0
         [0.0, 2.0],   # argmax=1
         [2.0, 0.0],   # argmax=0
         [2.0, 0.0]],  # argmax=0
    )
    y = torch.tensor([0, 1, 1, 1])   # верно для строк 0 и 1 -> 2 из 4
    acc = accuracy(model, X, y)
    assert abs(acc - 0.5) < 1e-7


# ---------------------------------------------------------------------------
# Главный тест модуля: обучение реально снижает loss и поднимает accuracy.
# На seeded разделимой задаче за несколько десятков эпох:
#   - итоговый loss < 0.5 * начальный;
#   - итоговая accuracy заметно выше начальной и высока (>= 0.9).
# ---------------------------------------------------------------------------

def test_training_reduces_loss_and_raises_accuracy():
    torch.manual_seed(0)
    X, y = _toy_classification(n_per_class=30, n_classes=3, in_dim=4, seed=0)

    model = MLP(in_dim=4, hidden=16, out_dim=3)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
    loss_fn = nn.CrossEntropyLoss()

    initial_loss = train_step(model, optimizer, loss_fn, X, y)
    initial_acc = accuracy(model, X, y)

    final_loss = initial_loss
    for _ in range(150):
        final_loss = train_step(model, optimizer, loss_fn, X, y)

    final_acc = accuracy(model, X, y)

    # Loss заметно упал.
    assert final_loss < 0.5 * initial_loss
    # Accuracy выросла и стала высокой (задача хорошо разделима).
    assert final_acc > initial_acc
    assert final_acc >= 0.9


def test_training_loss_is_monotone_ish():
    # Не строго монотонно, но loss в конце прогона должен быть ниже, чем в начале.
    torch.manual_seed(0)
    X, y = _toy_classification(seed=1)
    model = MLP(in_dim=4, hidden=16, out_dim=3)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
    loss_fn = nn.CrossEntropyLoss()

    first = train_step(model, optimizer, loss_fn, X, y)
    last = first
    for _ in range(100):
        last = train_step(model, optimizer, loss_fn, X, y)
    assert last < first


# ---------------------------------------------------------------------------
# train_epoch: одна эпоха мини-батчами.
#   - возвращает положительный float;
#   - оракул: при batch_size = len(X) (полный батч) одна эпоха = один train_step
#     на всём X (порядок внутри единственного батча роли не играет);
#   - обучение через train_epoch снижает loss и поднимает accuracy.
# ---------------------------------------------------------------------------

def test_train_epoch_returns_positive_float():
    torch.manual_seed(0)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_classification()
    mean_loss = train_epoch(model, optimizer, loss_fn, X, y, batch_size=16)
    assert isinstance(mean_loss, float)
    assert mean_loss > 0.0


def test_train_epoch_full_batch_matches_single_train_step():
    # ОРАКУЛ: с batch_size = N DataLoader отдаёт ровно ОДИН батч со всеми данными.
    # Тогда train_epoch обязан вернуть то же число, что один train_step на всём X, y,
    # и сдвинуть параметры идентично (shuffle одного полного батча ничего не меняет:
    # full-batch градиент не зависит от порядка объектов).
    X, y = _toy_classification(seed=0)
    N = X.shape[0]

    # Две идентично проинициализированные модели (один и тот же seed перед созданием).
    torch.manual_seed(123)
    model_step = MLP(in_dim=4, hidden=8, out_dim=3)
    torch.manual_seed(123)
    model_epoch = MLP(in_dim=4, hidden=8, out_dim=3)

    loss_fn = nn.CrossEntropyLoss()
    opt_step = torch.optim.SGD(model_step.parameters(), lr=0.1)
    opt_epoch = torch.optim.SGD(model_epoch.parameters(), lr=0.1)

    loss_step = train_step(model_step, opt_step, loss_fn, X, y)
    loss_epoch = train_epoch(model_epoch, opt_epoch, loss_fn, X, y, batch_size=N)

    # Возвращённый средний loss совпадает (батч ровно один).
    assert abs(loss_epoch - loss_step) < 1e-4

    # И параметры после шага совпали — абстракция «эпоха» сводится к одному шагу.
    for p_step, p_epoch in zip(model_step.parameters(), model_epoch.parameters()):
        assert torch.allclose(p_step, p_epoch, atol=1e-5)


def test_train_epoch_changes_parameters():
    torch.manual_seed(0)
    model = MLP(in_dim=4, hidden=8, out_dim=3)
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_classification()

    before = [p.detach().clone() for p in model.parameters()]
    train_epoch(model, optimizer, loss_fn, X, y, batch_size=16)
    after = list(model.parameters())

    changed = any(not torch.allclose(b, a) for b, a in zip(before, after))
    assert changed


def test_training_via_train_epoch_reduces_loss_and_raises_accuracy():
    # Тот же контракт, что и у пошагового обучения, но прогон идёт ЭПОХАМИ
    # (мини-батчами через DataLoader) — проверяем, что абстракция реально учит.
    torch.manual_seed(0)
    X, y = _toy_classification(n_per_class=30, n_classes=3, in_dim=4, seed=0)

    model = MLP(in_dim=4, hidden=16, out_dim=3)
    optimizer = torch.optim.Adam(model.parameters(), lr=0.05)
    loss_fn = nn.CrossEntropyLoss()

    # Качество ДО обучения (на свежей сети) — обучение по эпохам сходится быстро,
    # поэтому замеряем точность здесь, а не после первой эпохи.
    initial_acc = accuracy(model, X, y)

    initial_loss = train_epoch(model, optimizer, loss_fn, X, y, batch_size=16)
    final_loss = initial_loss
    for _ in range(50):
        final_loss = train_epoch(model, optimizer, loss_fn, X, y, batch_size=16)

    final_acc = accuracy(model, X, y)

    assert final_loss < 0.5 * initial_loss
    assert final_acc > initial_acc
    assert final_acc >= 0.9
