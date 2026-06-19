# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 09 — Оптимизаторы. Всё на numpy, CPU, детерминированно (сиды зафиксированы).
# Игрушечная задача везде одна: f(p) = sum((p - target) ** 2).
#   градиент:  df/dp = 2 * (p - target)
# Минимум достигается в p == target, где f == 0. Удобно: легко проверить и формулы,
# и факт «обучение снижает loss».

import numpy as np

from optim import sgd_step, momentum_step, adam_step


# ---------- игрушечная квадратичная задача ----------

def quad_loss(p: np.ndarray, target: np.ndarray) -> float:
    """f(p) = sum((p - target) ** 2)."""
    return float(np.sum((p - target) ** 2))


def quad_grad(p: np.ndarray, target: np.ndarray) -> np.ndarray:
    """df/dp = 2 * (p - target)."""
    return 2.0 * (p - target)


# ---------- 1. SGD: проверка формулы ----------

def test_sgd_step_formula():
    params = np.array([1.0, 2.0, 3.0])
    grads = np.array([0.5, -1.0, 2.0])
    lr = 0.1
    out = sgd_step(params, grads, lr)
    expected = params - lr * grads
    assert out.shape == params.shape
    np.testing.assert_allclose(out, expected, atol=1e-12)


def test_sgd_step_does_not_mutate_inputs():
    params = np.array([1.0, 2.0, 3.0])
    grads = np.array([0.5, -1.0, 2.0])
    params_copy = params.copy()
    grads_copy = grads.copy()
    sgd_step(params, grads, 0.1)
    np.testing.assert_array_equal(params, params_copy)
    np.testing.assert_array_equal(grads, grads_copy)


# ---------- 2. momentum: проверка формулы ----------

def test_momentum_step_formula():
    params = np.array([1.0, 2.0])
    grads = np.array([0.4, -0.2])
    velocity = np.array([0.1, 0.1])
    lr = 0.1
    beta = 0.9
    new_params, new_velocity = momentum_step(params, grads, velocity, lr, beta)
    exp_velocity = beta * velocity + (1 - beta) * grads
    exp_params = params - lr * exp_velocity
    assert new_params.shape == params.shape
    assert new_velocity.shape == velocity.shape
    np.testing.assert_allclose(new_velocity, exp_velocity, atol=1e-12)
    np.testing.assert_allclose(new_params, exp_params, atol=1e-12)


def test_momentum_zero_beta_equals_sgd():
    # beta == 0 -> velocity == grads -> шаг совпадает с обычным SGD.
    params = np.array([1.0, -2.0, 0.5])
    grads = np.array([0.3, 0.7, -0.1])
    velocity = np.zeros_like(params)
    lr = 0.05
    new_params, _ = momentum_step(params, grads, velocity, lr, beta=0.0)
    np.testing.assert_allclose(new_params, sgd_step(params, grads, lr), atol=1e-12)


# ---------- 3. Adam: проверка формулы на первом шаге ----------

def test_adam_step_formula_first_step():
    params = np.array([1.0, 2.0, 3.0])
    grads = np.array([0.5, -0.5, 1.0])
    m = np.zeros_like(params)
    v = np.zeros_like(params)
    t = 1
    lr, b1, b2, eps = 1e-3, 0.9, 0.999, 1e-8

    new_params, new_m, new_v = adam_step(params, grads, m, v, t, lr, b1, b2, eps)

    exp_m = b1 * m + (1 - b1) * grads
    exp_v = b2 * v + (1 - b2) * (grads ** 2)
    m_hat = exp_m / (1 - b1 ** t)
    v_hat = exp_v / (1 - b2 ** t)
    exp_params = params - lr * m_hat / (np.sqrt(v_hat) + eps)

    assert new_params.shape == params.shape
    assert new_m.shape == params.shape
    assert new_v.shape == params.shape
    # m и v возвращаются БЕЗ поправки на смещение.
    np.testing.assert_allclose(new_m, exp_m, atol=1e-12)
    np.testing.assert_allclose(new_v, exp_v, atol=1e-12)
    np.testing.assert_allclose(new_params, exp_params, atol=1e-12)


# ---------- паттерн «обучение снижает loss»: каждый оптимизатор сходится к target ----------

def test_sgd_converges_on_quadratic():
    np.random.seed(0)
    target = np.array([3.0, -1.0, 0.5, 2.0])
    p = np.random.randn(4) * 5.0
    lr = 0.1

    loss0 = quad_loss(p, target)
    for _ in range(200):
        g = quad_grad(p, target)
        p = sgd_step(p, g, lr)
    loss1 = quad_loss(p, target)

    assert loss1 < 0.5 * loss0
    np.testing.assert_allclose(p, target, atol=1e-2)


def test_momentum_converges_on_quadratic():
    np.random.seed(0)
    target = np.array([1.0, -2.0, 4.0])
    p = np.random.randn(3) * 5.0
    velocity = np.zeros_like(p)
    lr, beta = 0.05, 0.9

    loss0 = quad_loss(p, target)
    for _ in range(200):
        g = quad_grad(p, target)
        p, velocity = momentum_step(p, g, velocity, lr, beta)
    loss1 = quad_loss(p, target)

    assert loss1 < 0.5 * loss0
    np.testing.assert_allclose(p, target, atol=1e-2)


def test_adam_converges_on_quadratic():
    np.random.seed(0)
    target = np.array([2.0, -3.0, 1.0, 0.0, 5.0])
    p = np.random.randn(5) * 2.0
    m = np.zeros_like(p)
    v = np.zeros_like(p)
    lr, b1, b2, eps = 0.1, 0.9, 0.999, 1e-8

    loss0 = quad_loss(p, target)
    for step in range(1, 301):
        g = quad_grad(p, target)
        p, m, v = adam_step(p, g, m, v, step, lr, b1, b2, eps)
    loss1 = quad_loss(p, target)

    assert loss1 < 0.5 * loss0
    np.testing.assert_allclose(p, target, atol=1e-2)


def test_adam_bias_correction_makes_first_step_meaningful():
    # На первом шаге без поправки шаг был бы ~ lr (т.к. m_hat/sqrt(v_hat) ≈ sign(g)).
    # Проверяем, что одиночный шаг Adam реально сдвигает параметр в сторону target.
    target = np.array([10.0])
    p = np.array([0.0])
    m = np.zeros_like(p)
    v = np.zeros_like(p)
    lr, b1, b2, eps = 0.1, 0.9, 0.999, 1e-8

    g = quad_grad(p, target)  # = -20.0, тянет p вверх к 10
    new_p, _, _ = adam_step(p, g, m, v, 1, lr, b1, b2, eps)
    assert new_p[0] > p[0]  # сдвинулись в сторону target
    # На первом шаге величина сдвига близка к lr (адаптивная нормировка ~1).
    np.testing.assert_allclose(new_p[0] - p[0], lr, atol=1e-6)
