# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 13 — Свёрточные сети (CNN). Тесты детерминированы (сиды зафиксированы),
# крошечные и идут на CPU. Картинки в форме (N, C, H, W) = (N, 1, 8, 8).

import torch
import torch.nn as nn

from cnn import conv_output_size, SmallCNN, train_step


# ---------------------------------------------------------------------------
# conv_output_size: проверяем формулу на ряде случаев
# ---------------------------------------------------------------------------

def test_conv_output_size_basic():
    # картинка 8, ядро 3, stride 1, без отступа -> 8 - 3 + 1 = 6
    assert conv_output_size(8, 3, 1, 0) == 6


def test_conv_output_size_pool_like():
    # как MaxPool 2x2: in=8, kernel=2, stride=2, pad=0 -> (8-2)//2 + 1 = 4
    assert conv_output_size(8, 2, 2, 0) == 4


def test_conv_output_size_padding_keeps_size():
    # "same"-свёртка: kernel=3, pad=1, stride=1 сохраняет размер
    assert conv_output_size(8, 3, 1, 1) == 8


def test_conv_output_size_stride_floors():
    # stride округляет ВНИЗ (целочисленное деление):
    # in=7, kernel=3, stride=2, pad=0 -> (7-3)//2 + 1 = 3
    assert conv_output_size(7, 3, 2, 0) == 3


def test_conv_output_size_returns_int():
    out = conv_output_size(8, 3, 1, 0)
    assert isinstance(out, int)


def test_conv_output_size_matches_torch_conv():
    # сверяем нашу формулу с тем, что реально выдаёт nn.Conv2d
    torch.manual_seed(0)
    conv = nn.Conv2d(1, 1, kernel_size=3, stride=2, padding=1)
    x = torch.randn(1, 1, 8, 8)
    h_out = conv(x).shape[-1]
    assert conv_output_size(8, 3, 2, 1) == h_out


# ---------------------------------------------------------------------------
# SmallCNN: тип, форма выхода, обучаемые параметры
# ---------------------------------------------------------------------------

def test_smallcnn_is_module():
    model = SmallCNN()
    assert isinstance(model, nn.Module)


def test_smallcnn_output_shape_default_classes():
    torch.manual_seed(0)
    model = SmallCNN()
    x = torch.randn(5, 1, 8, 8)        # (N=5, C=1, H=8, W=8)
    out = model(x)
    assert tuple(out.shape) == (5, 2)  # логиты двух классов


def test_smallcnn_output_shape_custom_classes():
    torch.manual_seed(0)
    model = SmallCNN(num_classes=3)
    x = torch.randn(4, 1, 8, 8)
    out = model(x)
    assert tuple(out.shape) == (4, 3)


def test_smallcnn_single_image_batch():
    torch.manual_seed(0)
    model = SmallCNN()
    x = torch.randn(1, 1, 8, 8)        # батч из одной картинки
    out = model(x)
    assert tuple(out.shape) == (1, 2)


def test_smallcnn_has_trainable_parameters():
    model = SmallCNN()
    params = list(model.parameters())
    assert len(params) > 0
    assert all(p.requires_grad for p in params)


def test_smallcnn_does_not_apply_softmax():
    # на выходе должны быть СЫРЫЕ логиты, а не вероятности:
    # суммы по классам не равны 1, и встречаются отрицательные значения.
    torch.manual_seed(1)
    model = SmallCNN()
    x = torch.randn(8, 1, 8, 8)
    out = model(x)
    row_sums = out.sum(dim=1)
    assert not torch.allclose(row_sums, torch.ones_like(row_sums), atol=1e-3)


# ---------------------------------------------------------------------------
# train_step: возвращает число и реально двигает веса
# ---------------------------------------------------------------------------

def _toy_images(seed: int = 0, n_per_class: int = 8):
    """Игрушечный seeded набор: два класса картинок 8x8, 1 канал.

    Класс 0 — картинки с яркой ВЕРХНЕЙ половиной, класс 1 — с яркой НИЖНЕЙ.
    Узор локальный и сдвиго-различимый, так что CNN его легко ловит.
    """
    g = torch.Generator().manual_seed(seed)
    imgs = []
    labels = []
    for _ in range(n_per_class):
        a = 0.3 * torch.randn(1, 8, 8, generator=g)
        a[:, :4, :] += 2.0          # яркая верхняя половина
        imgs.append(a)
        labels.append(0)
    for _ in range(n_per_class):
        b = 0.3 * torch.randn(1, 8, 8, generator=g)
        b[:, 4:, :] += 2.0          # яркая нижняя половина
        imgs.append(b)
        labels.append(1)
    X = torch.stack(imgs, dim=0)    # (N, 1, 8, 8)
    y = torch.tensor(labels, dtype=torch.long)
    return X, y


def test_train_step_returns_float():
    torch.manual_seed(0)
    model = SmallCNN()
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_images(seed=0)
    loss = train_step(model, optimizer, loss_fn, X, y)
    assert isinstance(loss, float)


def test_train_step_changes_parameters():
    torch.manual_seed(0)
    model = SmallCNN()
    optimizer = torch.optim.SGD(model.parameters(), lr=0.1)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_images(seed=0)

    before = [p.detach().clone() for p in model.parameters()]
    train_step(model, optimizer, loss_fn, X, y)
    after = list(model.parameters())

    # хотя бы один параметр должен измениться после шага оптимизатора
    changed = any(not torch.allclose(b, a) for b, a in zip(before, after))
    assert changed


# ---------------------------------------------------------------------------
# Всё вместе: на крошечном seeded наборе картинок loss заметно падает.
# Проверяем, что кирпичики собираются в обучение: итоговый loss < 0.5 * начальный.
# ---------------------------------------------------------------------------

def test_training_reduces_loss_on_toy_images():
    torch.manual_seed(0)
    model = SmallCNN()
    optimizer = torch.optim.SGD(model.parameters(), lr=0.2)
    loss_fn = nn.CrossEntropyLoss()
    X, y = _toy_images(seed=0)

    initial_loss = train_step(model, optimizer, loss_fn, X, y)

    final_loss = initial_loss
    for _ in range(150):
        final_loss = train_step(model, optimizer, loss_fn, X, y)

    assert final_loss < 0.5 * initial_loss
