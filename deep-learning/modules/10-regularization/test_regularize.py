# Эти тесты трогать не нужно — это эталон поведения.
#
# Запуск: ./deep-learning/run.sh 10
# Стек: только numpy. Данные крошечные и синтетические, всё на CPU, сиды зафиксированы.

import numpy as np
import pytest

from regularize import (
    train_val_split,
    l2_penalty,
    dropout_mask,
    should_early_stop,
)


# --------------------------------------------------------------------------- #
#  Задание 1: train_val_split — детерминированное разбиение                    #
# --------------------------------------------------------------------------- #

def _toy_data(n=20, d=3, seed=0):
    rng = np.random.default_rng(seed)
    X = rng.normal(size=(n, d))
    y = np.arange(n)  # метки = номера строк, удобно проверять, что пары не разъехались
    return X, y


def test_split_sizes():
    X, y = _toy_data(n=20, d=3)
    X_tr, y_tr, X_val, y_val = train_val_split(X, y, val_frac=0.25, seed=0)
    # 20 * 0.25 = 5 в валидацию, 15 в обучение
    assert X_val.shape == (5, 3)
    assert X_tr.shape == (15, 3)
    assert y_val.shape == (5,)
    assert y_tr.shape == (15,)


def test_split_is_a_partition():
    # объединение train и val должно дать ровно исходное множество строк (без потерь/дублей)
    X, y = _toy_data(n=20, d=3)
    _, y_tr, _, y_val = train_val_split(X, y, val_frac=0.25, seed=1)
    all_labels = np.concatenate([y_tr, y_val])
    assert sorted(all_labels.tolist()) == list(range(20))


def test_split_keeps_pairs_together():
    # y[i] == номер строки i; после сплита X-строка и её y обязаны соответствовать друг другу
    X, y = _toy_data(n=20, d=3)
    X_tr, y_tr, X_val, y_val = train_val_split(X, y, val_frac=0.3, seed=2)
    for X_part, y_part in [(X_tr, y_tr), (X_val, y_val)]:
        for row, label in zip(X_part, y_part):
            assert np.allclose(row, X[label])


def test_split_is_deterministic():
    X, y = _toy_data(n=20, d=3)
    a = train_val_split(X, y, val_frac=0.25, seed=7)
    b = train_val_split(X, y, val_frac=0.25, seed=7)
    for arr_a, arr_b in zip(a, b):
        assert np.array_equal(arr_a, arr_b)


def test_split_actually_shuffles():
    # при seed!=, который реально перемешивает, валидация не должна быть просто первыми строками
    X, y = _toy_data(n=20, d=3)
    _, _, _, y_val = train_val_split(X, y, val_frac=0.25, seed=3)
    assert y_val.tolist() != [0, 1, 2, 3, 4]


def test_split_2d_targets():
    # y может быть (N, K) — форма второй оси должна сохраниться
    X, _ = _toy_data(n=12, d=2)
    y = np.zeros((12, 4))
    X_tr, y_tr, X_val, y_val = train_val_split(X, y, val_frac=0.5, seed=0)
    assert y_tr.shape == (6, 4)
    assert y_val.shape == (6, 4)


# --------------------------------------------------------------------------- #
#  Задание 2: l2_penalty — штраф за большие веса                              #
# --------------------------------------------------------------------------- #

def test_l2_basic_value():
    w = np.array([3.0, -4.0])  # сумма квадратов = 25
    assert np.isclose(l2_penalty(w, lam=0.1), 2.5)


def test_l2_zero_lambda():
    w = np.array([10.0, -20.0, 100.0])
    assert l2_penalty(w, lam=0.0) == 0.0


def test_l2_returns_python_float():
    w = np.ones((4,))
    out = l2_penalty(w, lam=1.0)
    assert isinstance(out, float)


def test_l2_matches_reference_2d():
    rng = np.random.default_rng(0)
    w = rng.normal(size=(5, 3))
    lam = 0.37
    expected = lam * float(np.sum(w ** 2))
    assert np.isclose(l2_penalty(w, lam), expected)


def test_l2_grows_with_weights():
    small = l2_penalty(np.array([0.1, 0.1]), lam=1.0)
    big = l2_penalty(np.array([5.0, 5.0]), lam=1.0)
    assert big > small


# --------------------------------------------------------------------------- #
#  Задание 3: dropout_mask — маска с масштабированием 1/(1-p)                  #
# --------------------------------------------------------------------------- #

def test_dropout_shape():
    m = dropout_mask((4, 5), p=0.5, seed=0)
    assert m.shape == (4, 5)


def test_dropout_values_are_zero_or_scaled():
    p = 0.3
    m = dropout_mask((10, 10), p=p, seed=0)
    scale = 1.0 / (1.0 - p)
    # каждый элемент — либо 0, либо ровно 1/(1-p)
    is_zero = np.isclose(m, 0.0)
    is_scaled = np.isclose(m, scale)
    assert np.all(is_zero | is_scaled)


def test_dropout_p_zero_keeps_all():
    m = dropout_mask((6, 6), p=0.0, seed=0)
    assert np.allclose(m, 1.0)


def test_dropout_mean_preserved():
    # inverted dropout: среднее по маске должно держаться около 1.0 при большой выборке
    m = dropout_mask((200, 200), p=0.5, seed=0)
    assert abs(m.mean() - 1.0) < 0.05


def test_dropout_is_deterministic():
    a = dropout_mask((8, 8), p=0.4, seed=123)
    b = dropout_mask((8, 8), p=0.4, seed=123)
    assert np.array_equal(a, b)


def test_dropout_drop_fraction_roughly_p():
    p = 0.4
    m = dropout_mask((200, 200), p=p, seed=1)
    frac_dropped = np.isclose(m, 0.0).mean()
    assert abs(frac_dropped - p) < 0.05


# --------------------------------------------------------------------------- #
#  Задание 4: should_early_stop — ранняя остановка                            #
# --------------------------------------------------------------------------- #

def test_early_stop_triggers_after_patience():
    # лучший (0.8) на индексе 1, после него 2 эпохи без улучшения, patience=2 -> True
    assert should_early_stop([1.0, 0.8, 0.9, 0.95], patience=2) is True


def test_early_stop_holds_while_improving():
    # минимум на последней эпохе — терпение не исчерпано -> False
    assert should_early_stop([1.0, 0.8, 0.7], patience=2) is False


def test_early_stop_not_enough_epochs():
    # эпох меньше, чем patience+1 — останавливаться рано
    assert should_early_stop([1.0, 0.9], patience=3) is False


def test_early_stop_strict_improvement_needed():
    # плато: после минимума (0.5 на индексе 1) три эпохи без НОВОГО минимума, patience=3 -> True
    assert should_early_stop([1.0, 0.5, 0.5, 0.5, 0.5], patience=3) is True


def test_early_stop_returns_bool():
    out = should_early_stop([1.0, 0.9, 0.8, 0.7], patience=2)
    assert isinstance(out, bool)


# --------------------------------------------------------------------------- #
#  Мини-сценарий: L2 реально снижает переобучение на игрушечной регрессии      #
#  (итоговый разрыв train/val меньше; loss на val заметно падает)             #
# --------------------------------------------------------------------------- #

def test_l2_reduces_overfitting_gap():
    """Учебная мини-демонстрация: полиномиальная регрессия на шумных данных.

    Без L2 модель сильнее переобучается (большой разрыв train/val), с L2 — меньше.
    Проверяем, что L2 ужимает разрыв более чем вдвое: gap_l2 < 0.5 * gap_no_l2.
    """
    rng = np.random.default_rng(0)

    # истина: y = 2x; данных мало, шум есть, признаки — полином высокой степени (легко переобучиться)
    n = 14
    x = np.linspace(-1.0, 1.0, n)
    y_clean = 2.0 * x
    y = y_clean + rng.normal(scale=0.3, size=n)

    degree = 9
    # матрица признаков (n, degree+1): столбцы x^0..x^degree
    Phi = np.vander(x, degree + 1, increasing=True)

    # делим на train/val нашей же функцией
    X_tr, y_tr, X_val, y_val = train_val_split(Phi, y, val_frac=0.4, seed=0)

    def fit_and_gap(lam):
        # обучаем линейную регрессию градиентным спуском, добавляя L2-штраф к loss
        w = np.zeros(degree + 1)
        lr = 0.05
        for _ in range(2000):
            pred = X_tr @ w
            err = pred - y_tr
            grad = (2.0 / len(y_tr)) * (X_tr.T @ err) + 2.0 * lam * w  # производная L2 = 2*lam*w
            w = w - lr * grad
        train_mse = float(np.mean((X_tr @ w - y_tr) ** 2))
        val_mse = float(np.mean((X_val @ w - y_val) ** 2))
        # сам штраф считаем нашей функцией (входит в эталонную проверку контракта)
        _ = l2_penalty(w, lam)
        return val_mse - train_mse

    gap_no_l2 = fit_and_gap(lam=0.0)
    gap_l2 = fit_and_gap(lam=0.05)

    assert gap_no_l2 > 0.0  # без регуляризации разрыв заметен (переобучение есть)
    assert gap_l2 < 0.5 * gap_no_l2  # L2 ужимает разрыв более чем вдвое
