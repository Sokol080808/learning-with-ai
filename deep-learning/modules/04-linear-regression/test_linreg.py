# Эти тесты трогать не нужно — это эталон поведения.
#
# Детерминированно (np.random.seed), крошечные синтетические данные, только CPU.
# Ключевой тест в конце собирает полный цикл обучения predict→loss→gradients→gd_step
# и проверяет, что loss заметно падает.

import numpy as np

from linreg import predict, mse_loss, gradients, gd_step


# ---------------------------------------------------------------------------
# predict
# ---------------------------------------------------------------------------

def test_predict_shape():
    np.random.seed(0)
    X = np.random.randn(5, 3)
    w = np.random.randn(3)
    b = 0.5
    y_pred = predict(X, w, b)
    assert y_pred.shape == (5,)


def test_predict_matches_manual():
    # Маленький ручной пример, который можно проверить «на бумаге».
    X = np.array([[1.0, 2.0],
                  [3.0, 4.0]])
    w = np.array([1.0, -1.0])
    b = 0.5
    # строка 0: 1*1 + 2*(-1) + 0.5 = -0.5
    # строка 1: 3*1 + 4*(-1) + 0.5 = -0.5
    expected = np.array([-0.5, -0.5])
    assert np.allclose(predict(X, w, b), expected)


def test_predict_equals_Xw_plus_b():
    np.random.seed(1)
    X = np.random.randn(7, 4)
    w = np.random.randn(4)
    b = -1.25
    assert np.allclose(predict(X, w, b), X @ w + b)


# ---------------------------------------------------------------------------
# mse_loss
# ---------------------------------------------------------------------------

def test_mse_zero_when_perfect():
    y = np.array([1.0, 2.0, 3.0])
    assert mse_loss(y, y) == 0.0


def test_mse_is_float():
    y_pred = np.array([1.0, 2.0])
    y = np.array([0.0, 0.0])
    loss = mse_loss(y_pred, y)
    assert isinstance(loss, float)


def test_mse_known_value():
    y_pred = np.array([2.0, 0.0, 2.0, 0.0])
    y = np.array([0.0, 0.0, 0.0, 0.0])
    # ошибки: 2,0,2,0 -> квадраты 4,0,4,0 -> среднее = 8/4 = 2.0
    assert np.isclose(mse_loss(y_pred, y), 2.0)


# ---------------------------------------------------------------------------
# gradients
# ---------------------------------------------------------------------------

def test_gradients_shapes():
    np.random.seed(2)
    X = np.random.randn(6, 3)
    y = np.random.randn(6)
    w = np.random.randn(3)
    b = 0.1
    dw, db = gradients(X, y, w, b)
    assert dw.shape == (3,)
    assert isinstance(db, float)


def test_gradients_match_analytic_formula():
    np.random.seed(3)
    X = np.random.randn(8, 2)
    y = np.random.randn(8)
    w = np.random.randn(2)
    b = -0.3
    e = (X @ w + b) - y
    N = X.shape[0]
    dw_expected = (2.0 / N) * (X.T @ e)
    db_expected = (2.0 / N) * np.sum(e)
    dw, db = gradients(X, y, w, b)
    assert np.allclose(dw, dw_expected)
    assert np.isclose(db, db_expected)


def test_gradients_match_numerical():
    # Сверяем аналитический градиент с численным (конечные разности) — золотой стандарт проверки.
    np.random.seed(4)
    X = np.random.randn(5, 3)
    y = np.random.randn(5)
    w = np.random.randn(3)
    b = 0.2

    def loss_at(w_, b_):
        return mse_loss(predict(X, w_, b_), y)

    dw, db = gradients(X, y, w, b)

    eps = 1e-5
    dw_num = np.zeros_like(w)
    for i in range(w.shape[0]):
        wp = w.copy(); wp[i] += eps
        wm = w.copy(); wm[i] -= eps
        dw_num[i] = (loss_at(wp, b) - loss_at(wm, b)) / (2 * eps)
    db_num = (loss_at(w, b + eps) - loss_at(w, b - eps)) / (2 * eps)

    assert np.allclose(dw, dw_num, atol=1e-5)
    assert np.isclose(db, db_num, atol=1e-5)


# ---------------------------------------------------------------------------
# gd_step
# ---------------------------------------------------------------------------

def test_gd_step_moves_against_gradient():
    w = np.array([1.0, 1.0])
    b = 1.0
    dw = np.array([2.0, -4.0])
    db = 0.5
    lr = 0.1
    w_new, b_new = gd_step(w, b, dw, db, lr)
    assert np.allclose(w_new, np.array([1.0 - 0.1 * 2.0, 1.0 - 0.1 * (-4.0)]))
    assert np.isclose(b_new, 1.0 - 0.1 * 0.5)


def test_gd_step_shapes_and_types():
    w = np.zeros(3)
    b = 0.0
    dw = np.ones(3)
    db = 1.0
    w_new, b_new = gd_step(w, b, dw, db, 0.01)
    assert w_new.shape == (3,)
    assert isinstance(b_new, float)


# ---------------------------------------------------------------------------
# Полный цикл обучения: loss должен заметно падать.
# ---------------------------------------------------------------------------

def test_training_loop_reduces_loss():
    np.random.seed(0)
    N, D = 64, 3

    # Синтетические данные с ИЗВЕСТНЫМ линейным законом + лёгкий шум.
    X = np.random.randn(N, D)
    w_true = np.array([2.0, -3.0, 0.5])
    b_true = 1.0
    y = X @ w_true + b_true + 0.01 * np.random.randn(N)

    # Старт из нуля — модель ещё ничего не знает.
    w = np.zeros(D)
    b = 0.0
    lr = 0.1

    initial_loss = mse_loss(predict(X, w, b), y)

    for _ in range(200):
        dw, db = gradients(X, y, w, b)
        w, b = gd_step(w, b, dw, db, lr)

    final_loss = mse_loss(predict(X, w, b), y)

    # Обучение должно сильно снизить ошибку.
    assert final_loss < 0.5 * initial_loss
    # И на такой простой задаче — почти восстановить истинные параметры.
    assert np.allclose(w, w_true, atol=0.1)
    assert np.isclose(b, b_true, atol=0.1)


def test_training_loss_monotone_decreasing():
    # При разумном lr на квадратичном loss каждый шаг не должен увеличивать ошибку.
    np.random.seed(7)
    N, D = 40, 2
    X = np.random.randn(N, D)
    y = X @ np.array([1.5, -0.5]) + 0.3

    w = np.zeros(D)
    b = 0.0
    lr = 0.05

    prev = mse_loss(predict(X, w, b), y)
    for _ in range(50):
        dw, db = gradients(X, y, w, b)
        w, b = gd_step(w, b, dw, db, lr)
        cur = mse_loss(predict(X, w, b), y)
        assert cur <= prev + 1e-9  # не растёт (с крошечным допуском на численность)
        prev = cur
