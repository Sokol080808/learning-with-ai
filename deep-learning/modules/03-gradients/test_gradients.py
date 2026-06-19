# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 03 — Производные, цепное правило, градиенты.
# Тесты детерминированы (сиды зафиксированы), крошечные и гоняются на CPU.

import numpy as np
import pytest

from gradients import numerical_gradient, grad_sum_squares, grad_of_affine


# --------------------------------------------------------------------------
# numerical_gradient — центральная разность
# --------------------------------------------------------------------------

def test_numgrad_shape_matches_input():
    np.random.seed(0)
    x = np.random.randn(5)
    g = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
    assert g.shape == x.shape


def test_numgrad_sum_squares():
    # f(x) = sum(x^2)  =>  grad = 2x
    np.random.seed(0)
    x = np.random.randn(4)
    g = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
    assert np.allclose(g, 2.0 * x, atol=1e-5)


def test_numgrad_does_not_mutate_input():
    np.random.seed(1)
    x = np.random.randn(6)
    x_before = x.copy()
    _ = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
    assert np.array_equal(x, x_before)


def test_numgrad_linear_function():
    # f(x) = dot(w, x)  =>  grad = w  (постоянный, не зависит от x)
    np.random.seed(2)
    w = np.random.randn(4)
    x = np.random.randn(4)
    g = numerical_gradient(lambda v: float(np.dot(w, v)), x)
    assert np.allclose(g, w, atol=1e-5)


def test_numgrad_works_on_2d_shape():
    # центральная разность должна работать для любой формы, не только (D,)
    np.random.seed(3)
    X = np.random.randn(3, 2)
    g = numerical_gradient(lambda V: float(np.sum(V ** 2)), X)
    assert g.shape == X.shape
    assert np.allclose(g, 2.0 * X, atol=1e-5)


def test_numgrad_nonpolynomial_function():
    # f(x) = sum(sin(x))  =>  grad = cos(x); проверяем на честной нелинейности
    np.random.seed(4)
    x = np.random.randn(5)
    g = numerical_gradient(lambda v: float(np.sum(np.sin(v))), x)
    assert np.allclose(g, np.cos(x), atol=1e-5)


# --------------------------------------------------------------------------
# grad_sum_squares — аналитика 2*x
# --------------------------------------------------------------------------

def test_grad_sum_squares_value():
    np.random.seed(5)
    x = np.random.randn(7)
    assert np.allclose(grad_sum_squares(x), 2.0 * x)


def test_grad_sum_squares_shape():
    x = np.array([1.0, -2.0, 3.0, 4.0])
    g = grad_sum_squares(x)
    assert g.shape == x.shape


def test_grad_sum_squares_matches_numerical():
    # аналитика должна совпасть с численной проверкой (gradient checking)
    np.random.seed(6)
    x = np.random.randn(5)
    num = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
    assert np.allclose(grad_sum_squares(x), num, atol=1e-5)


# --------------------------------------------------------------------------
# grad_of_affine — аналитика w
# --------------------------------------------------------------------------

def test_grad_of_affine_value():
    np.random.seed(7)
    w = np.random.randn(6)
    x = np.random.randn(6)
    assert np.allclose(grad_of_affine(x, w), w)


def test_grad_of_affine_shape():
    w = np.array([0.5, -1.0, 2.0])
    x = np.array([3.0, 3.0, 3.0])
    assert grad_of_affine(x, w).shape == x.shape


def test_grad_of_affine_matches_numerical():
    np.random.seed(8)
    w = np.random.randn(5)
    x = np.random.randn(5)
    num = numerical_gradient(lambda v: float(np.dot(w, v)), x)
    assert np.allclose(grad_of_affine(x, w), num, atol=1e-5)


# --------------------------------------------------------------------------
# Применение: градиентный спуск по f(x) = sum(x^2) должен снижать loss.
# Здесь работают сразу и аналитика (grad_sum_squares), и сам смысл градиента.
# --------------------------------------------------------------------------

def test_gradient_descent_reduces_loss():
    np.random.seed(9)
    x = np.random.randn(8)

    def loss(v):
        return float(np.sum(v ** 2))

    loss_start = loss(x)
    lr = 0.1
    for _ in range(100):
        g = grad_sum_squares(x)   # шагаем ПРОТИВ градиента
        x = x - lr * g

    loss_end = loss(x)
    # обучение должно заметно снизить loss (не требуем точного числа)
    assert loss_end < 0.5 * loss_start
