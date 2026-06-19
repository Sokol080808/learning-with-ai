# Эти тесты трогать не нужно — это эталон поведения.
#
# Тесты модуля 07 (Обратное распространение с нуля). Всё на numpy, CPU, крошечные данные,
# фиксированные сиды — детерминированно и быстро. Главная проверка: аналитические
# градиенты из backward2 совпадают с численными (конечные разности) на seeded данных.

import numpy as np
import pytest

from backprop import (
    relu,
    softmax,
    cross_entropy,
    forward2,
    backward2,
    numerical_gradient,
)


# ----------------------------------------------------------------------------------------
# Вспомогательное: маленькая seeded сеть и данные.
# ----------------------------------------------------------------------------------------
def make_net(seed=0, N=5, D=4, H=6, C=3):
    """Возвращает (x, y, W1, b1, W2, b2) — крошечная двухслойная сеть и батч с метками."""
    rng = np.random.RandomState(seed)
    x = rng.randn(N, D)
    y = rng.randint(0, C, size=N)
    # Небольшой масштаб весов, чтобы логиты были умеренными, а relu не "выключала" всё.
    W1 = rng.randn(D, H) * 0.5
    b1 = rng.randn(H) * 0.1
    W2 = rng.randn(H, C) * 0.5
    b2 = rng.randn(C) * 0.1
    return x, y, W1, b1, W2, b2


def onehot(y, C):
    out = np.zeros((y.shape[0], C))
    out[np.arange(y.shape[0]), y] = 1.0
    return out


# ----------------------------------------------------------------------------------------
# Базовые кирпичики: relu, softmax, cross_entropy.
# ----------------------------------------------------------------------------------------
def test_relu_basic():
    z = np.array([[-2.0, -0.1, 0.0, 0.3, 5.0]])
    expected = np.array([[0.0, 0.0, 0.0, 0.3, 5.0]])
    assert np.allclose(relu(z), expected)


def test_relu_preserves_shape():
    z = np.random.RandomState(1).randn(4, 7)
    assert relu(z).shape == z.shape


def test_softmax_rows_sum_to_one():
    z = np.random.RandomState(2).randn(5, 3)
    p = softmax(z)
    assert p.shape == z.shape
    assert np.allclose(p.sum(axis=1), np.ones(5))
    assert np.all(p > 0)


def test_softmax_is_shift_invariant():
    # softmax(z) == softmax(z + const) по строке — следствие вычитания max.
    z = np.random.RandomState(3).randn(4, 3)
    shift = np.array([[100.0], [-50.0], [7.0], [0.0]])
    assert np.allclose(softmax(z), softmax(z + shift))


def test_softmax_numerically_stable_large_logits():
    # Большие логиты не должны давать nan/inf (благодаря вычитанию max).
    z = np.array([[1000.0, 1001.0, 1002.0]])
    p = softmax(z)
    assert np.all(np.isfinite(p))
    assert np.allclose(p.sum(axis=1), 1.0)


def test_cross_entropy_known_value():
    # Равномерное распределение по C=3 классам -> потеря = -log(1/3) = log(3) для любой метки.
    p = np.full((4, 3), 1.0 / 3.0)
    y = np.array([0, 1, 2, 0])
    assert np.isclose(cross_entropy(p, y), np.log(3.0), atol=1e-6)


def test_cross_entropy_confident_correct_is_small():
    # Уверенно и правильно -> потеря близка к 0.
    p = np.array([[0.999, 0.0005, 0.0005], [0.0005, 0.999, 0.0005]])
    y = np.array([0, 1])
    assert cross_entropy(p, y) < 0.01


# ----------------------------------------------------------------------------------------
# forward2: согласован с кирпичиками; возвращает скаляр.
# ----------------------------------------------------------------------------------------
def test_forward2_returns_scalar_loss():
    x, y, W1, b1, W2, b2 = make_net(seed=0)
    L = forward2(x, W1, b1, W2, b2, y)
    assert np.isscalar(L) or np.ndim(L) == 0
    assert np.isfinite(L)
    assert L > 0


def test_forward2_matches_manual_composition():
    x, y, W1, b1, W2, b2 = make_net(seed=4)
    z1 = x @ W1 + b1
    a1 = relu(z1)
    z2 = a1 @ W2 + b2
    p = softmax(z2)
    expected = cross_entropy(p, y)
    assert np.isclose(forward2(x, W1, b1, W2, b2, y), expected, atol=1e-10)


# ----------------------------------------------------------------------------------------
# numerical_gradient: проверка самой утилиты на функции с известной производной.
# ----------------------------------------------------------------------------------------
def test_numerical_gradient_on_quadratic():
    # f(p) = sum(p^2) -> grad = 2*p. Проверяем численную утилиту независимо от сети.
    param = np.array([1.0, -2.0, 0.5, 3.0])

    def f():
        return float(np.sum(param ** 2))

    g = numerical_gradient(f, param)
    assert np.allclose(g, 2.0 * param, atol=1e-5)


def test_numerical_gradient_restores_param():
    # Утилита не должна портить переданный массив (значения восстановлены).
    param = np.array([0.3, -1.1, 2.2])
    snapshot = param.copy()

    def f():
        return float(np.sum(param ** 3))

    numerical_gradient(f, param)
    assert np.allclose(param, snapshot)


# ----------------------------------------------------------------------------------------
# ГЛАВНОЕ: аналитический backprop == численный градиент (по каждому параметру).
# ----------------------------------------------------------------------------------------
@pytest.mark.parametrize("seed", [0, 1, 2])
def test_backward2_matches_numerical_gradient(seed):
    x, y, W1, b1, W2, b2 = make_net(seed=seed)
    grads = backward2(x, y, W1, b1, W2, b2)

    # Формы градиентов совпадают с формами параметров.
    assert grads["dW1"].shape == W1.shape
    assert grads["db1"].shape == b1.shape
    assert grads["dW2"].shape == W2.shape
    assert grads["db2"].shape == b2.shape

    # Численный градиент по каждому параметру: f дёргает forward2 при текущих параметрах.
    num_dW1 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), W1)
    num_db1 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), b1)
    num_dW2 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), W2)
    num_db2 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), b2)

    assert np.allclose(grads["dW1"], num_dW1, atol=1e-5)
    assert np.allclose(grads["db1"], num_db1, atol=1e-5)
    assert np.allclose(grads["dW2"], num_dW2, atol=1e-5)
    assert np.allclose(grads["db2"], num_db2, atol=1e-5)


def test_backward2_softmax_ce_gradient_on_logits_is_p_minus_y():
    # Тонкий тест "красивого" градиента: при нулевом первом слое (a1=0 после relu, если z1<=0)
    # проверяем напрямую dz2 = (p - onehot(y))/N через db2 (db2 = sum(dz2, axis=0)).
    # Берём сеть, считаем p и сравниваем db2 с суммой (p - onehot)/N по объектам.
    x, y, W1, b1, W2, b2 = make_net(seed=7)
    z1 = x @ W1 + b1
    a1 = relu(z1)
    z2 = a1 @ W2 + b2
    p = softmax(z2)
    N, C = p.shape
    expected_db2 = ((p - onehot(y, C)) / N).sum(axis=0)
    grads = backward2(x, y, W1, b1, W2, b2)
    assert np.allclose(grads["db2"], expected_db2, atol=1e-8)


# ----------------------------------------------------------------------------------------
# Обучение снижает loss: несколько шагов градиентного спуска через backward2.
# ----------------------------------------------------------------------------------------
def test_training_with_backward2_reduces_loss():
    x, y, W1, b1, W2, b2 = make_net(seed=0, N=16, D=4, H=8, C=3)
    lr = 0.5

    start_loss = forward2(x, W1, b1, W2, b2, y)
    for _ in range(200):
        g = backward2(x, y, W1, b1, W2, b2)
        W1 = W1 - lr * g["dW1"]
        b1 = b1 - lr * g["db1"]
        W2 = W2 - lr * g["dW2"]
        b2 = b2 - lr * g["db2"]
    end_loss = forward2(x, W1, b1, W2, b2, y)

    # Градиентный спуск должен заметно уронить потерю.
    assert end_loss < 0.5 * start_loss
