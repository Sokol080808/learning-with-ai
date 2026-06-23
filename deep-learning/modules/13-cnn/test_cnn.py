# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 13 — Свёрточные сети (CNN). Тесты детерминированы (сиды зафиксированы),
# крошечные и идут на CPU. Картинки в форме (N, C, H, W) = (N, 1, 8, 8).

import math

import torch
import torch.nn as nn

from cnn import (
    conv_output_size,
    SmallCNN,
    train_step,
    receptive_field,
    augment_batch,
)


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


# ---------------------------------------------------------------------------
# receptive_field: рецептивное поле стопки same-padding свёрток
#
# Эталон — НЕ просто формула: строим стопку из n Conv2d(weight=1, bias=0),
# подаём импульс (одна единица в центре нулевой картинки) и считаем, сколько
# выходных пикселей стали ненулевыми. Это площадь рецептивного поля RF², так
# что sqrt(count) обязан совпасть с receptive_field(n, k).
# ---------------------------------------------------------------------------

def _rf_impulse_oracle(num_conv_layers: int, kernel_size: int) -> int:
    """Spatial oracle: trace the receptive field with a single-pixel impulse."""
    layers = []
    for _ in range(num_conv_layers):
        conv = nn.Conv2d(
            1, 1, kernel_size=kernel_size, padding=(kernel_size - 1) // 2, bias=True
        )
        with torch.no_grad():
            conv.weight.fill_(1.0)
            conv.bias.zero_()
        layers.append(conv)
    net = nn.Sequential(*layers)

    # картинка заведомо больше любого ожидаемого RF, чтобы поле не упёрлось в край
    size = 1 + (kernel_size - 1) * num_conv_layers + 8
    x = torch.zeros(1, 1, size, size)
    x[0, 0, size // 2, size // 2] = 1.0
    with torch.no_grad():
        out = net(x)
    nonzero = int((out.abs() > 1e-8).sum().item())
    return int(round(math.sqrt(nonzero)))


def test_receptive_field_contract_examples():
    # фиксированные значения из контракта
    assert receptive_field(1, 3) == 3
    assert receptive_field(2, 3) == 5
    assert receptive_field(3, 3) == 7


def test_receptive_field_returns_int():
    assert isinstance(receptive_field(2, 3), int)


def test_receptive_field_kernel5():
    assert receptive_field(1, 5) == 5
    assert receptive_field(2, 5) == 9


def test_receptive_field_kernel1_is_pointwise():
    # ядро 1×1 не расширяет поле: RF всегда 1, сколько слоёв ни ставь
    for n in [1, 2, 5]:
        assert receptive_field(n, 1) == 1


def test_receptive_field_matches_impulse_oracle():
    # совпадение с пространственным эталоном (импульс + подсчёт ненулевых пикселей)
    for n, k in [(1, 3), (2, 3), (3, 3), (1, 5), (2, 5)]:
        assert receptive_field(n, k) == _rf_impulse_oracle(n, k), (
            f"n={n} k={k}: formula={receptive_field(n, k)} "
            f"oracle={_rf_impulse_oracle(n, k)}"
        )


# ---------------------------------------------------------------------------
# augment_batch: аугментация картинок (flip / crop+pad / шум), seeded
# ---------------------------------------------------------------------------

def test_augment_batch_preserves_shape():
    torch.manual_seed(0)
    x = torch.randn(5, 1, 8, 8)
    out = augment_batch(x, seed=7)
    assert tuple(out.shape) == (5, 1, 8, 8)


def test_augment_batch_deterministic_same_seed():
    torch.manual_seed(0)
    x = torch.randn(4, 1, 8, 8)
    a = augment_batch(x, seed=42)
    b = augment_batch(x, seed=42)
    assert torch.equal(a, b)


def test_augment_batch_different_seeds_differ():
    torch.manual_seed(0)
    x = torch.randn(4, 1, 8, 8)
    a = augment_batch(x, seed=1)
    c = augment_batch(x, seed=2)
    assert not torch.allclose(a, c)


def test_augment_batch_flip_oracle_vs_torch():
    # с выключенными шумом и кропом форсированный флип обязан совпасть с torch.flip.
    # seed=0 (по построению) флипает единственную картинку батча.
    torch.manual_seed(0)
    x = torch.randn(1, 1, 8, 8)
    out = augment_batch(x, seed=0, noise_std=0.0, pad=0)
    assert torch.equal(out, torch.flip(x, dims=[-1]))
    # двойной флип возвращает оригинал (инволюция)
    assert torch.equal(torch.flip(out, dims=[-1]), x)


def test_augment_batch_noise_zero_pad_zero_is_identity_or_flip():
    # при noise_std=0, pad=0 выход — это либо сама картинка, либо её зеркало,
    # ничего больше (никакого шума / сдвига).
    torch.manual_seed(0)
    x = torch.randn(6, 1, 8, 8)
    out = augment_batch(x, seed=3, noise_std=0.0, pad=0)
    for i in range(x.shape[0]):
        same = torch.equal(out[i], x[i])
        flipped = torch.equal(out[i], torch.flip(x[i], dims=[-1]))
        assert same or flipped, f"image {i} is neither identity nor horizontal flip"


def test_augment_batch_noise_magnitude_bound():
    # на КОНСТАНТНОЙ картинке флип и кроп — тождество, остаётся только шум.
    # его средняя по модулю величина масштабируется с noise_std и -> 0 при 0.
    const = torch.full((4, 1, 8, 8), 0.5)
    out0 = augment_batch(const, seed=5, noise_std=0.0, pad=0)
    assert torch.equal(out0, const)
    out = augment_batch(const, seed=5, noise_std=0.1, pad=0)
    mad = (out - const).abs().mean().item()
    assert mad < 3 * 0.1, f"noise too large: {mad}"


def test_augment_batch_dtype_and_finite():
    out = augment_batch(torch.zeros(2, 1, 8, 8), seed=1)
    assert out.dtype == torch.float32
    assert torch.isfinite(out).all()


def test_augment_batch_does_not_mutate_input():
    torch.manual_seed(0)
    x = torch.randn(3, 1, 8, 8)
    x_before = x.clone()
    _ = augment_batch(x, seed=9)
    assert torch.equal(x, x_before)


def test_augment_batch_rejects_wrong_rank():
    import pytest

    with pytest.raises(ValueError):
        augment_batch(torch.randn(8, 8), seed=0)
