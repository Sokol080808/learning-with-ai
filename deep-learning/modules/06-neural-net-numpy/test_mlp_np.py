# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 06 — Нейросеть с нуля: forward pass. Только numpy, CPU, всё детерминированно.
import numpy as np
import pytest

from mlp_np import relu, softmax, linear, forward2, cross_entropy


# ---------- эталонные реализации (для сверки через allclose) ----------

def ref_softmax(x):
    m = x.max(axis=-1, keepdims=True)
    e = np.exp(x - m)
    return e / e.sum(axis=-1, keepdims=True)


def ref_cross_entropy(logits, y):
    m = logits.max(axis=1, keepdims=True)
    lse = m.ravel() + np.log(np.exp(logits - m).sum(axis=1))
    correct = logits[np.arange(logits.shape[0]), y]
    return float(np.mean(lse - correct))


# ---------- relu ----------

def test_relu_basic():
    x = np.array([-2.0, -0.5, 0.0, 0.5, 3.0])
    out = relu(x)
    assert np.allclose(out, [0.0, 0.0, 0.0, 0.5, 3.0])


def test_relu_keeps_shape_2d():
    np.random.seed(0)
    x = np.random.randn(4, 7)
    out = relu(x)
    assert out.shape == x.shape
    assert np.allclose(out, np.maximum(0.0, x))


def test_relu_does_not_mutate_input():
    x = np.array([-1.0, 2.0, -3.0])
    x_copy = x.copy()
    _ = relu(x)
    assert np.allclose(x, x_copy), "relu не должен менять входной массив на месте"


# ---------- softmax ----------

def test_softmax_rows_sum_to_one():
    np.random.seed(1)
    x = np.random.randn(5, 4)
    out = softmax(x)
    assert out.shape == x.shape
    assert np.allclose(out.sum(axis=-1), np.ones(5))


def test_softmax_nonnegative():
    np.random.seed(2)
    x = np.random.randn(3, 6)
    out = softmax(x)
    assert np.all(out >= 0.0)


def test_softmax_matches_reference():
    np.random.seed(3)
    x = np.random.randn(8, 5)
    assert np.allclose(softmax(x), ref_softmax(x), atol=1e-8)


def test_softmax_uniform_logits():
    x = np.zeros((2, 4))
    out = softmax(x)
    assert np.allclose(out, np.full((2, 4), 0.25))


def test_softmax_numerically_stable_large_logits():
    # Без сдвига на максимум exp(1000) даст inf и испортит результат.
    x = np.array([[1000.0, 1000.0, 1000.0]])
    out = softmax(x)
    assert np.all(np.isfinite(out))
    assert np.allclose(out, np.full((1, 3), 1.0 / 3.0))


def test_softmax_invariant_to_shift():
    np.random.seed(4)
    x = np.random.randn(4, 5)
    shifted = x + 100.0
    assert np.allclose(softmax(x), softmax(shifted), atol=1e-8)


# ---------- linear ----------

def test_linear_shape():
    np.random.seed(5)
    x = np.random.randn(6, 3)
    W = np.random.randn(3, 4)
    b = np.random.randn(4)
    out = linear(x, W, b)
    assert out.shape == (6, 4)


def test_linear_matches_reference():
    np.random.seed(6)
    x = np.random.randn(10, 5)
    W = np.random.randn(5, 7)
    b = np.random.randn(7)
    assert np.allclose(linear(x, W, b), x @ W + b, atol=1e-8)


def test_linear_known_values():
    x = np.array([[1.0, 2.0]])
    W = np.array([[1.0, 0.0, 2.0],
                  [0.0, 1.0, 3.0]])
    b = np.array([10.0, 20.0, 30.0])
    # x·W = [1, 2, 1*2 + 2*3] = [1, 2, 8]; +b = [11, 22, 38]
    assert np.allclose(linear(x, W, b), [[11.0, 22.0, 38.0]])


# ---------- forward2 ----------

def _make_net(D, H, C, seed=7):
    rng = np.random.RandomState(seed)
    W1 = rng.randn(D, H) * 0.1
    b1 = rng.randn(H) * 0.1
    W2 = rng.randn(H, C) * 0.1
    b2 = rng.randn(C) * 0.1
    return W1, b1, W2, b2


def test_forward2_shape():
    np.random.seed(8)
    N, D, H, C = 5, 4, 6, 3
    x = np.random.randn(N, D)
    W1, b1, W2, b2 = _make_net(D, H, C)
    logits = forward2(x, W1, b1, W2, b2)
    assert logits.shape == (N, C)


def test_forward2_matches_manual_composition():
    np.random.seed(9)
    N, D, H, C = 7, 5, 8, 4
    x = np.random.randn(N, D)
    W1, b1, W2, b2 = _make_net(D, H, C, seed=11)
    expected = np.maximum(0.0, x @ W1 + b1) @ W2 + b2
    assert np.allclose(forward2(x, W1, b1, W2, b2), expected, atol=1e-8)


def test_forward2_uses_relu_in_hidden_layer():
    # Если бы скрытый слой был линейным (без ReLU), сеть схлопнулась бы в линейную
    # и результат совпал бы с x·(W1·W2)+... — проверяем, что это НЕ так.
    np.random.seed(10)
    N, D, H, C = 6, 4, 5, 3
    x = np.random.randn(N, D)
    W1, b1, W2, b2 = _make_net(D, H, C, seed=13)
    linear_only = (x @ W1 + b1) @ W2 + b2
    got = forward2(x, W1, b1, W2, b2)
    assert not np.allclose(got, linear_only), "между слоями должна быть нелинейность ReLU"


# ---------- cross_entropy ----------

def test_cross_entropy_returns_float():
    np.random.seed(14)
    logits = np.random.randn(5, 3)
    y = np.array([0, 1, 2, 1, 0])
    loss = cross_entropy(logits, y)
    assert isinstance(loss, float)


def test_cross_entropy_matches_reference():
    np.random.seed(15)
    logits = np.random.randn(12, 5)
    y = np.random.randint(0, 5, size=12)
    assert np.isclose(cross_entropy(logits, y), ref_cross_entropy(logits, y), atol=1e-8)


def test_cross_entropy_perfect_prediction_is_near_zero():
    # Огромный логит у правильного класса => вероятность ≈ 1 => loss ≈ 0.
    logits = np.array([[100.0, 0.0, 0.0],
                       [0.0, 100.0, 0.0]])
    y = np.array([0, 1])
    loss = cross_entropy(logits, y)
    assert loss < 1e-3


def test_cross_entropy_uniform_logits_equals_log_C():
    # Равные логиты => равномерное распределение => loss == log(C) для любого y.
    C = 4
    logits = np.zeros((3, C))
    y = np.array([0, 2, 3])
    loss = cross_entropy(logits, y)
    assert np.isclose(loss, np.log(C), atol=1e-8)


def test_cross_entropy_numerically_stable():
    # Большие логиты не должны давать inf/nan.
    logits = np.array([[1000.0, -1000.0, 0.0],
                       [-1000.0, 1000.0, 0.0]])
    y = np.array([0, 1])
    loss = cross_entropy(logits, y)
    assert np.isfinite(loss)
    assert loss < 1e-3


# ---------- интеграция: forward2 + cross_entropy на игрушечной задаче ----------

def test_pipeline_lower_loss_with_better_weights():
    # «Обучение снижает loss», но без backprop: показываем, что подогнанные под задачу
    # веса дают loss заметно (< 0.5x) меньше, чем случайные. Forward + cross_entropy.
    np.random.seed(16)
    N, D, H, C = 40, 3, 16, 2

    # Линейно разделимая игрушечная задача: класс = знак суммы первых двух признаков.
    x = np.random.randn(N, D)
    y = (x[:, 0] + x[:, 1] > 0).astype(np.int64)

    # Случайная сеть.
    W1r, b1r, W2r, b2r = _make_net(D, H, C, seed=17)
    loss_random = cross_entropy(forward2(x, W1r, b1r, W2r, b2r), y)

    # «Хорошая» сеть: первый скрытый нейрон ~ (x0+x1), пропускаем через ReLU,
    # и читаем его в логиты так, чтобы класс 1 выигрывал при больших (x0+x1).
    W1 = np.zeros((D, H))
    b1 = np.zeros(H)
    W1[0, 0] = 5.0
    W1[1, 0] = 5.0          # нейрон 0 ≈ relu(5*(x0+x1)) — активен, когда x0+x1>0
    W1[0, 1] = -5.0
    W1[1, 1] = -5.0         # нейрон 1 ≈ relu(-5*(x0+x1)) — активен, когда x0+x1<0
    W2 = np.zeros((H, C))
    W2[0, 1] = 1.0          # нейрон 0 -> голос за класс 1
    W2[1, 0] = 1.0          # нейрон 1 -> голос за класс 0
    b2 = np.zeros(C)
    loss_good = cross_entropy(forward2(x, W1, b1, W2, b2), y)

    assert loss_good < 0.5 * loss_random
