# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 11 — Знакомство с PyTorch. Тесты детерминированы (сиды зафиксированы),
# крошечные и идут на CPU.

import numpy as np
import torch

from torch_intro import to_tensor, grad_of, linear_forward, mse_torch, numpy_bridge


# ---------------------------------------------------------------------------
# to_tensor: всегда float32, форма и значения сохраняются, разные типы входа
# ---------------------------------------------------------------------------

def test_to_tensor_dtype_is_float32():
    t = to_tensor([1, 2, 3])
    assert isinstance(t, torch.Tensor)
    assert t.dtype == torch.float32


def test_to_tensor_values_and_shape_from_list():
    t = to_tensor([[1, 2], [3, 4]])
    assert tuple(t.shape) == (2, 2)
    assert torch.allclose(t, torch.tensor([[1.0, 2.0], [3.0, 4.0]]))


def test_to_tensor_from_numpy():
    arr = np.array([0.5, -1.0, 2.5], dtype=np.float64)
    t = to_tensor(arr)
    assert t.dtype == torch.float32
    assert torch.allclose(t, torch.tensor([0.5, -1.0, 2.5]))


def test_to_tensor_on_cpu():
    t = to_tensor([1.0, 2.0])
    assert t.device.type == "cpu"


# ---------------------------------------------------------------------------
# grad_of: сверка autograd-градиента с аналитическим
# ---------------------------------------------------------------------------

def test_grad_of_sum_of_squares_matches_analytic():
    # f(x) = sum(x**2)  ->  df/dx = 2x
    x = to_tensor([1.0, 2.0, 3.0])
    g = grad_of(lambda t: (t ** 2).sum(), x)
    expected = 2.0 * x
    assert g.shape == x.shape
    assert torch.allclose(g, expected, atol=1e-6)


def test_grad_of_matches_analytic_other_point():
    # тот же f, другая точка: df/dx = 2x
    x = to_tensor([-2.0, 0.0, 4.5, 10.0])
    g = grad_of(lambda t: (t ** 2).sum(), x)
    assert torch.allclose(g, 2.0 * x, atol=1e-6)


def test_grad_of_linear_function():
    # f(x) = sum(3*x)  ->  df/dx = 3 для каждой компоненты
    x = to_tensor([5.0, -1.0, 0.0])
    g = grad_of(lambda t: (3.0 * t).sum(), x)
    assert torch.allclose(g, torch.full_like(x, 3.0), atol=1e-6)


def test_grad_of_does_not_mutate_input():
    # исходный x не должен получить .grad и не должен включить requires_grad
    x = to_tensor([1.0, 2.0, 3.0])
    _ = grad_of(lambda t: (t ** 2).sum(), x)
    assert x.requires_grad is False
    assert x.grad is None


# ---------------------------------------------------------------------------
# linear_forward: формы и значения, сверка с эталоном
# ---------------------------------------------------------------------------

def test_linear_forward_shape():
    torch.manual_seed(0)
    x = torch.randn(2, 3)          # (N=2, D_in=3)
    W = torch.randn(3, 4)          # (D_in=3, D_out=4)
    b = torch.randn(4)             # (D_out=4,)
    out = linear_forward(x, W, b)
    assert tuple(out.shape) == (2, 4)


def test_linear_forward_matches_reference():
    torch.manual_seed(1)
    x = torch.randn(5, 3)
    W = torch.randn(3, 2)
    b = torch.randn(2)
    out = linear_forward(x, W, b)
    expected = x @ W + b
    assert torch.allclose(out, expected, atol=1e-6)


def test_linear_forward_bias_broadcasts_to_each_row():
    # нулевые веса -> каждая строка результата равна b
    x = to_tensor([[1.0, 2.0], [3.0, 4.0], [5.0, 6.0]])
    W = torch.zeros(2, 3)
    b = to_tensor([10.0, 20.0, 30.0])
    out = linear_forward(x, W, b)
    expected = b.repeat(3, 1)
    assert torch.allclose(out, expected, atol=1e-6)


# ---------------------------------------------------------------------------
# mse_torch: скаляр, сверка с ручным значением и с эталоном
# ---------------------------------------------------------------------------

def test_mse_is_scalar():
    pred = to_tensor([1.0, 2.0, 3.0])
    y = to_tensor([1.0, 2.0, 3.0])
    loss = mse_torch(pred, y)
    assert loss.shape == torch.Size([])     # скаляр, форма ()


def test_mse_zero_when_equal():
    pred = to_tensor([1.0, -2.0, 0.5])
    y = to_tensor([1.0, -2.0, 0.5])
    loss = mse_torch(pred, y)
    assert torch.allclose(loss, torch.tensor(0.0), atol=1e-7)


def test_mse_known_value():
    # разности [0, 0, -2] -> квадраты [0, 0, 4] -> среднее 4/3
    pred = to_tensor([1.0, 2.0, 3.0])
    y = to_tensor([1.0, 2.0, 5.0])
    loss = mse_torch(pred, y)
    assert torch.allclose(loss, torch.tensor(4.0 / 3.0), atol=1e-6)


def test_mse_matches_reference():
    torch.manual_seed(2)
    pred = torch.randn(7)
    y = torch.randn(7)
    loss = mse_torch(pred, y)
    expected = torch.mean((pred - y) ** 2)
    assert torch.allclose(loss, expected, atol=1e-6)


# ---------------------------------------------------------------------------
# Всё вместе: ручной градиентный спуск на линейной задаче снижает loss.
# Проверяем, что кирпичики собираются в обучение: итоговый loss < 0.5 * начальный.
# ---------------------------------------------------------------------------

def test_pieces_combine_into_training_that_reduces_loss():
    torch.manual_seed(0)
    N, D_in, D_out = 32, 4, 1

    # игрушечная линейная зависимость y = x @ W_true + b_true
    x = torch.randn(N, D_in)
    W_true = torch.randn(D_in, D_out)
    b_true = torch.randn(D_out)
    y = x @ W_true + b_true

    # стартовые параметры — далеко от истины
    W = torch.zeros(D_in, D_out)
    b = torch.zeros(D_out)

    def loss_for(params):
        Wp = params[: D_in * D_out].reshape(D_in, D_out)
        bp = params[D_in * D_out :]
        pred = linear_forward(x, Wp, bp)
        return mse_torch(pred, y)

    params = torch.cat([W.reshape(-1), b.reshape(-1)])

    initial_loss = loss_for(params).item()

    lr = 0.1
    for _ in range(200):
        g = grad_of(loss_for, params)
        params = params - lr * g

    final_loss = loss_for(params).item()

    assert final_loss < 0.5 * initial_loss


# ---------------------------------------------------------------------------
# numpy_bridge: numpy -> torch (from_numpy) -> +1 под no_grad -> numpy (.numpy())
# ---------------------------------------------------------------------------

def test_numpy_bridge_returns_numpy_array():
    arr = np.array([1.0, 2.0, 3.0], dtype=np.float32)
    out = numpy_bridge(arr)
    assert isinstance(out, np.ndarray)


def test_numpy_bridge_adds_one():
    # детерминированный оракул: вход + 1.0
    arr = np.array([1.0, 2.0, 3.0], dtype=np.float32)
    out = numpy_bridge(arr)
    np.testing.assert_allclose(out, arr + 1.0, atol=1e-6)


def test_numpy_bridge_dtype_is_float32():
    # вход во float64 — на выходе всё равно float32
    arr = np.array([0.5, -1.0, 2.5], dtype=np.float64)
    out = numpy_bridge(arr)
    assert out.dtype == np.float32


def test_numpy_bridge_shape_preserved_2d():
    arr = np.array([[1.0, 2.0], [3.0, 4.0]], dtype=np.float32)
    out = numpy_bridge(arr)
    assert out.shape == (2, 2)
    np.testing.assert_allclose(out, arr + 1.0, atol=1e-6)


def test_numpy_bridge_from_int_array():
    # int-массив должен пройти мост и стать float32
    arr = np.array([0, 5, -3], dtype=np.int64)
    out = numpy_bridge(arr)
    assert out.dtype == np.float32
    np.testing.assert_allclose(out, np.array([1.0, 6.0, -2.0], dtype=np.float32), atol=1e-6)
