# Эти тесты трогать не нужно — это эталон поведения.
#
# Тесты детерминированы (фиксированный сид), данные крошечные, всё на CPU.
# Импорт идёт напрямую из стаба tensors.py этого же каталога.

import numpy as np

from tensors import scale, row_normalize, add_bias, relu_np


# ---------------------------------------------------------------- scale ----

def test_scale_basic():
    x = np.array([1.0, 2.0, 3.0])
    out = scale(x, 2.0)
    assert np.allclose(out, np.array([2.0, 4.0, 6.0]))


def test_scale_keeps_shape():
    np.random.seed(0)
    x = np.random.randn(4, 5)
    out = scale(x, -3.0)
    assert out.shape == x.shape
    assert np.allclose(out, x * -3.0)


def test_scale_zero():
    np.random.seed(1)
    x = np.random.randn(3, 3)
    out = scale(x, 0.0)
    assert np.allclose(out, np.zeros_like(x))


def test_scale_does_not_mutate_input():
    x = np.array([1.0, 2.0, 3.0])
    before = x.copy()
    _ = scale(x, 5.0)
    assert np.allclose(x, before)


# -------------------------------------------------------- row_normalize ----

def test_row_normalize_simple():
    x = np.array([[1.0, 3.0],
                  [2.0, 2.0]])
    out = row_normalize(x)
    expected = np.array([[0.25, 0.75],
                         [0.5, 0.5]])
    assert np.allclose(out, expected)


def test_row_normalize_rows_sum_to_one():
    np.random.seed(0)
    # положительные значения, чтобы суммы строк были далеки от нуля
    x = np.random.rand(6, 4) + 0.1
    out = row_normalize(x)
    assert out.shape == x.shape
    assert np.allclose(out.sum(axis=1), np.ones(6))


def test_row_normalize_matches_reference():
    np.random.seed(2)
    x = np.random.rand(5, 3) + 0.5
    out = row_normalize(x)
    ref = x / x.sum(axis=1, keepdims=True)
    assert np.allclose(out, ref)


# -------------------------------------------------------------- add_bias ---

def test_add_bias_basic():
    x = np.array([[1.0, 2.0],
                  [3.0, 4.0]])
    b = np.array([10.0, 20.0])
    out = add_bias(x, b)
    expected = np.array([[11.0, 22.0],
                         [13.0, 24.0]])
    assert np.allclose(out, expected)


def test_add_bias_shape_and_reference():
    np.random.seed(3)
    x = np.random.randn(7, 4)
    b = np.random.randn(4)
    out = add_bias(x, b)
    assert out.shape == (7, 4)
    assert np.allclose(out, x + b)


def test_add_bias_zero_bias_is_identity():
    np.random.seed(4)
    x = np.random.randn(3, 5)
    b = np.zeros(5)
    out = add_bias(x, b)
    assert np.allclose(out, x)


# -------------------------------------------------------------- relu_np ----

def test_relu_basic():
    x = np.array([-2.0, 0.0, 3.0])
    out = relu_np(x)
    assert np.allclose(out, np.array([0.0, 0.0, 3.0]))


def test_relu_matches_reference():
    np.random.seed(0)
    x = np.random.randn(5, 6)
    out = relu_np(x)
    assert out.shape == x.shape
    assert np.allclose(out, np.maximum(x, 0.0))


def test_relu_nonnegative():
    np.random.seed(5)
    x = np.random.randn(8, 8)
    out = relu_np(x)
    assert np.all(out >= 0.0)


def test_relu_does_not_mutate_input():
    x = np.array([-1.0, 2.0, -3.0])
    before = x.copy()
    _ = relu_np(x)
    assert np.allclose(x, before)
