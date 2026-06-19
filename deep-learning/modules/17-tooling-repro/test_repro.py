# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 17 — Воспроизводимость и инженерия. Тесты детерминированы (сиды
# зафиксированы), крошечные и идут на CPU. Сохранение/загрузка — через tmp_path.

import os
import random

import numpy as np
import torch
import torch.nn as nn

from repro import set_seed, count_parameters, save_model, load_model


# ---------------------------------------------------------------------------
# Крошечная модель для тестов save/load и count_parameters.
# Параметры:
#   fc1: Linear(4, 3) -> weight (3, 4) = 12 + bias (3,) = 3  -> 15
#   fc2: Linear(3, 2) -> weight (2, 3) =  6 + bias (2,) = 2  ->  8
# Итого обучаемых параметров: 23.
# ---------------------------------------------------------------------------

class TinyNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.fc1 = nn.Linear(4, 3)
        self.fc2 = nn.Linear(3, 2)

    def forward(self, x):
        return self.fc2(torch.relu(self.fc1(x)))


# ---------------------------------------------------------------------------
# set_seed: после фиксации сида два прохода дают одинаковые случайные тензоры
# ---------------------------------------------------------------------------

def test_set_seed_returns_none():
    assert set_seed(0) is None


def test_set_seed_torch_repeatable():
    # ОСОБЕННОСТЬ модуля: после set_seed два прохода дают идентичные тензоры
    set_seed(0)
    a = torch.randn(4)
    set_seed(0)
    b = torch.randn(4)
    assert torch.allclose(a, b)


def test_set_seed_numpy_repeatable():
    set_seed(123)
    a = np.random.rand(5)
    set_seed(123)
    b = np.random.rand(5)
    assert np.allclose(a, b)


def test_set_seed_python_random_repeatable():
    set_seed(7)
    a = [random.random() for _ in range(5)]
    set_seed(7)
    b = [random.random() for _ in range(5)]
    assert a == b


def test_set_seed_all_three_at_once():
    # один вызов set_seed обязан зафиксировать ВСЕ три генератора сразу
    set_seed(42)
    r1 = random.random()
    n1 = np.random.rand(3)
    t1 = torch.randn(3)

    set_seed(42)
    r2 = random.random()
    n2 = np.random.rand(3)
    t2 = torch.randn(3)

    assert r1 == r2
    assert np.allclose(n1, n2)
    assert torch.allclose(t1, t2)


def test_different_seeds_give_different_numbers():
    set_seed(0)
    a = torch.randn(4)
    set_seed(1)
    b = torch.randn(4)
    # разные сиды -> разные последовательности (с подавляющей вероятностью)
    assert not torch.allclose(a, b)


# ---------------------------------------------------------------------------
# count_parameters: число обучаемых параметров
# ---------------------------------------------------------------------------

def test_count_parameters_linear():
    # Linear(4, 3): weight (3, 4) = 12 + bias (3,) = 3 -> 15
    assert count_parameters(nn.Linear(4, 3)) == 15


def test_count_parameters_tinynet():
    # fc1: 15, fc2: 8 -> 23
    assert count_parameters(TinyNet()) == 23


def test_count_parameters_returns_int():
    assert isinstance(count_parameters(nn.Linear(2, 2)), int)


def test_count_parameters_ignores_frozen():
    # заморозим один параметр -> он не должен учитываться
    model = nn.Linear(4, 3)            # всего 15 обучаемых
    model.bias.requires_grad_(False)   # заморозили bias (3 числа)
    assert count_parameters(model) == 12


def test_count_parameters_matches_reference():
    torch.manual_seed(0)
    model = TinyNet()
    expected = sum(p.numel() for p in model.parameters() if p.requires_grad)
    assert count_parameters(model) == expected


# ---------------------------------------------------------------------------
# save_model + load_model: round-trip восстанавливает веса (allclose), tmp_path
# ---------------------------------------------------------------------------

def test_save_model_creates_file(tmp_path):
    set_seed(0)
    model = TinyNet()
    path = os.path.join(str(tmp_path), "model.pt")
    save_model(model, path)
    assert os.path.exists(path)


def test_save_then_load_restores_weights(tmp_path):
    # ОСОБЕННОСТЬ модуля: save_model + load_model восстанавливает веса (allclose)
    set_seed(0)
    src = TinyNet()

    path = os.path.join(str(tmp_path), "tiny.pt")
    save_model(src, path)

    # новый каркас той же архитектуры со СВОИМИ (другими) случайными весами
    set_seed(999)
    dst = TinyNet()

    # до загрузки веса различаются
    assert not torch.allclose(src.fc1.weight, dst.fc1.weight)

    returned = load_model(dst, path)

    # после загрузки все параметры совпадают поэлементно
    for (n_src, p_src), (n_dst, p_dst) in zip(
        src.state_dict().items(), returned.state_dict().items()
    ):
        assert n_src == n_dst
        assert torch.allclose(p_src, p_dst, atol=1e-7)


def test_load_model_returns_model(tmp_path):
    set_seed(0)
    src = TinyNet()
    path = os.path.join(str(tmp_path), "m.pt")
    save_model(src, path)

    dst = TinyNet()
    returned = load_model(dst, path)
    # должна вернуться сама модель (тот же объект, что передали)
    assert isinstance(returned, nn.Module)
    assert returned is dst


def test_loaded_model_gives_same_output(tmp_path):
    # практический критерий: загруженная модель считает то же, что исходная
    set_seed(0)
    src = TinyNet()
    src.eval()

    path = os.path.join(str(tmp_path), "out.pt")
    save_model(src, path)

    set_seed(123)
    dst = TinyNet()
    load_model(dst, path)
    dst.eval()

    set_seed(5)
    x = torch.randn(6, 4)            # (N=6, D_in=4)
    with torch.no_grad():
        out_src = src(x)
        out_dst = dst(x)
    assert torch.allclose(out_src, out_dst, atol=1e-6)
