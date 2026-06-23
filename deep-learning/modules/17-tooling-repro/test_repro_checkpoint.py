# Тесты для Задания 6 модуля 17 — чекпоинты (save_checkpoint / load_checkpoint).
#
# Rules:
#   - Все вызовы новых функций — ВНУТРИ тест-функций (файл собирается чисто против
#     стаба, который кидает NotImplementedError; падает только в рантайме).
#   - Оракул: torch / прямой пересчёт траектории — никогда не функции под тестом.
#   - Сиды фиксированы для полного детерминизма; всё на CPU, через tmp_path.
#
# Ключевой оракул — «прерванный == непрерывный»: тренировка, прерванная на шаге k
# и продолженная из чекпоинта, обязана дать побитово тот же финальный loss, что и
# непрерывная тренировка той же длины. Это доказывает, что СОСТОЯНИЕ ОПТИМИЗАТОРА
# (буферы моментума) сохранилось верно — чего проверка одних весов не доказывает.

import os

import torch
import torch.nn as nn
import torch.optim as optim

from repro import set_seed, save_checkpoint, load_checkpoint


# ---------------------------------------------------------------------------
# Хелперы: каркас и один прогон обучения. Оптимизатор — SGD С МОМЕНТУМОМ, чтобы у
# него было нетривиальное внутреннее состояние (иначе тест на optimizer state
# вырождается и ничего не доказывает).
# ---------------------------------------------------------------------------

def _make_model():
    return nn.Sequential(
        nn.Linear(4, 8),
        nn.ReLU(),
        nn.Linear(8, 1),
    )


def _make_data(seed):
    set_seed(seed)
    x = torch.randn(16, 4)
    y = torch.randn(16, 1)
    return x, y


def _train_steps(model, optimizer, x, y, n):
    """Прогнать n шагов SGD на (x, y); вернуть loss последнего шага."""
    criterion = nn.MSELoss()
    last = None
    for _ in range(n):
        optimizer.zero_grad()
        loss = criterion(model(x), y)
        loss.backward()
        optimizer.step()
        last = loss.item()
    return last


# ---------------------------------------------------------------------------
# Базовый контракт: round-trip восстанавливает веса модели И состояние оптимизатора
# ---------------------------------------------------------------------------

def test_checkpoint_returns_epoch_int(tmp_path):
    set_seed(0)
    model = _make_model()
    optimizer = optim.SGD(model.parameters(), lr=0.1, momentum=0.9)
    path = os.path.join(str(tmp_path), "ckpt.pt")
    save_checkpoint(model, optimizer, epoch=7, path=path)

    model2 = _make_model()
    opt2 = optim.SGD(model2.parameters(), lr=0.1, momentum=0.9)
    returned = load_checkpoint(model2, opt2, path)
    assert returned == 7
    assert isinstance(returned, int)


def test_checkpoint_saves_dict_with_three_keys(tmp_path):
    set_seed(1)
    model = _make_model()
    optimizer = optim.SGD(model.parameters(), lr=0.1, momentum=0.9)
    path = os.path.join(str(tmp_path), "ckpt.pt")
    # сделаем хотя бы один шаг, чтобы у оптимизатора появилось состояние
    _train_steps(model, optimizer, *_make_data(2), n=1)
    save_checkpoint(model, optimizer, epoch=3, path=path)

    raw = torch.load(path, weights_only=True)
    assert isinstance(raw, dict)
    assert set(raw.keys()) == {"model", "optimizer", "epoch"}
    assert raw["epoch"] == 3
    assert isinstance(raw["model"], dict)          # state_dict модели
    assert isinstance(raw["optimizer"], dict)      # state_dict оптимизатора


def test_checkpoint_restores_model_weights(tmp_path):
    set_seed(10)
    model = _make_model()
    optimizer = optim.SGD(model.parameters(), lr=0.1, momentum=0.9)
    _train_steps(model, optimizer, *_make_data(11), n=5)
    path = os.path.join(str(tmp_path), "ckpt.pt")
    save_checkpoint(model, optimizer, epoch=5, path=path)

    set_seed(999)                                   # другой каркас, другие веса
    model2 = _make_model()
    opt2 = optim.SGD(model2.parameters(), lr=0.1, momentum=0.9)
    # до загрузки веса различаются
    assert not torch.allclose(model[0].weight, model2[0].weight)
    load_checkpoint(model2, opt2, path)
    # после загрузки — поэлементно совпадают
    for (n_a, p_a), (n_b, p_b) in zip(
        model.state_dict().items(), model2.state_dict().items()
    ):
        assert n_a == n_b
        assert torch.allclose(p_a, p_b, atol=0)


def test_checkpoint_restores_optimizer_momentum_buffers(tmp_path):
    """Состояние оптимизатора (буфер моментума) обязано восстановиться точно."""
    set_seed(20)
    model = _make_model()
    optimizer = optim.SGD(model.parameters(), lr=0.1, momentum=0.9)
    _train_steps(model, optimizer, *_make_data(21), n=4)  # накопили моментум
    path = os.path.join(str(tmp_path), "ckpt.pt")
    save_checkpoint(model, optimizer, epoch=4, path=path)

    set_seed(7)
    model2 = _make_model()
    opt2 = optim.SGD(model2.parameters(), lr=0.1, momentum=0.9)
    load_checkpoint(model2, opt2, path)

    # сравним momentum_buffer'ы по всем параметрам напрямую через state оптимизатора
    sd_a = optimizer.state_dict()["state"]
    sd_b = opt2.state_dict()["state"]
    assert sd_a.keys() == sd_b.keys()
    for k in sd_a:
        buf_a = sd_a[k].get("momentum_buffer")
        buf_b = sd_b[k].get("momentum_buffer")
        assert (buf_a is None) == (buf_b is None)
        if buf_a is not None:
            assert torch.allclose(buf_a, buf_b, atol=0)


# ---------------------------------------------------------------------------
# ГЛАВНЫЙ ОРАКУЛ: прерванная и продолженная тренировка == непрерывная
# ---------------------------------------------------------------------------

def test_resume_matches_uninterrupted_run(tmp_path):
    """k+m шагов непрерывно == k шагов → checkpoint → load → ещё m шагов.

    На CPU с детерминированным сидом это БИТ-в-БИТ равенство финального loss.
    Доказывает, что состояние оптимизатора (моментум) восстановлено верно:
    проверки одних весов для этого недостаточно.
    """
    SEED = 123
    k, m = 6, 7

    # --- baseline: непрерывный прогон на k+m шагов ---
    set_seed(SEED)
    base_model = _make_model()
    base_opt = optim.SGD(base_model.parameters(), lr=0.1, momentum=0.9)
    x, y = _make_data(SEED + 1)
    _train_steps(base_model, base_opt, x, y, k + m)
    with torch.no_grad():
        baseline_loss = nn.MSELoss()(base_model(x), y).item()

    # --- прерванный прогон: k шагов, сохранить, продолжить m шагов ---
    set_seed(SEED)
    model = _make_model()
    opt = optim.SGD(model.parameters(), lr=0.1, momentum=0.9)
    x2, y2 = _make_data(SEED + 1)                    # те же данные (тот же под-сид)
    _train_steps(model, opt, x2, y2, k)
    path = os.path.join(str(tmp_path), "resume.pt")
    save_checkpoint(model, opt, epoch=k, path=path)

    # СВЕЖИЙ каркас и оптимизатор «с холода» — состояние придёт только из чекпоинта
    fresh_model = _make_model()
    fresh_opt = optim.SGD(fresh_model.parameters(), lr=0.1, momentum=0.9)
    epoch = load_checkpoint(fresh_model, fresh_opt, path)
    assert epoch == k
    _train_steps(fresh_model, fresh_opt, x2, y2, m)
    with torch.no_grad():
        resumed_loss = nn.MSELoss()(fresh_model(x2), y2).item()

    assert resumed_loss == baseline_loss, (
        "resumed final loss must be bit-identical to the uninterrupted run — "
        "if it differs, optimizer momentum state was not faithfully restored"
    )


def test_resume_without_optimizer_state_diverges(tmp_path):
    """Контр-проверка: если НЕ восстановить состояние оптимизатора (свежий SGD
    без load), траектория с моментумом расходится с baseline — это и есть причина,
    по которой checkpoint обязан хранить optimizer.state_dict().
    """
    SEED = 321
    k, m = 5, 6

    set_seed(SEED)
    base_model = _make_model()
    base_opt = optim.SGD(base_model.parameters(), lr=0.1, momentum=0.9)
    x, y = _make_data(SEED + 1)
    _train_steps(base_model, base_opt, x, y, k + m)
    with torch.no_grad():
        baseline_loss = nn.MSELoss()(base_model(x), y).item()

    # k шагов, сохраняем ТОЛЬКО веса (через checkpoint), но при продолжении
    # намеренно НЕ загружаем optimizer state — создаём свежий холодный SGD.
    set_seed(SEED)
    model = _make_model()
    opt = optim.SGD(model.parameters(), lr=0.1, momentum=0.9)
    x2, y2 = _make_data(SEED + 1)
    _train_steps(model, opt, x2, y2, k)
    path = os.path.join(str(tmp_path), "weights_only.pt")
    save_checkpoint(model, opt, epoch=k, path=path)

    fresh_model = _make_model()
    cold_opt = optim.SGD(fresh_model.parameters(), lr=0.1, momentum=0.9)
    # грузим веса вручную, оптимизатор оставляем холодным (момент = 0)
    raw = torch.load(path, weights_only=True)
    fresh_model.load_state_dict(raw["model"])
    _train_steps(fresh_model, cold_opt, x2, y2, m)
    with torch.no_grad():
        cold_loss = nn.MSELoss()(fresh_model(x2), y2).item()

    assert cold_loss != baseline_loss, (
        "cold optimizer (no momentum restore) must diverge from baseline — "
        "this is exactly why save_checkpoint stores optimizer state"
    )
