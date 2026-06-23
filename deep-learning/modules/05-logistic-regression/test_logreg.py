# Эти тесты трогать не нужно — это эталон поведения.
#
# Тесты модуля 05 (Логистическая регрессия). Детерминированные (фиксированные сиды),
# крошечные, считаются на CPU за доли секунды. Никаких загрузок данных — всё генерируем
# здесь же из numpy.

import numpy as np
import pytest

from logreg import (
    sigmoid,
    bce_loss,
    predict_proba,
    accuracy,
    logreg_gradients,
    softmax,
    cce_loss,
)


# ---------- sigmoid ----------

def test_sigmoid_known_values():
    # σ(0) = 0.5; σ(z) и σ(-z) симметричны относительно 0.5.
    z = np.array([0.0, 2.0, -2.0])
    p = sigmoid(z)
    assert np.allclose(p[0], 0.5, atol=1e-12)
    # σ(2) ≈ 0.880797, σ(-2) ≈ 0.119203
    assert np.allclose(p[1], 0.8807970779778823, atol=1e-9)
    assert np.allclose(p[2], 0.11920292202211755, atol=1e-9)
    # симметрия: σ(z) + σ(-z) = 1
    assert np.allclose(p[1] + p[2], 1.0, atol=1e-12)


def test_sigmoid_range_and_monotonic():
    z = np.linspace(-6.0, 6.0, 25)
    p = sigmoid(z)
    # строго в (0, 1)
    assert np.all(p > 0.0) and np.all(p < 1.0)
    # сигмоида монотонно возрастает
    assert np.all(np.diff(p) > 0.0)


def test_sigmoid_numerically_stable():
    # Главная проверка стабильности: на больших по модулю z не должно быть inf/nan.
    z = np.array([-1000.0, -50.0, 0.0, 50.0, 1000.0])
    p = sigmoid(z)
    assert np.all(np.isfinite(p)), "sigmoid переполнилась (inf/nan) на больших z"
    # очень отрицательное → ~0, очень положительное → ~1 (но строго внутри (0,1))
    assert p[0] >= 0.0 and p[0] < 1e-12
    assert p[-1] <= 1.0 and p[-1] > 1.0 - 1e-12
    assert np.allclose(p[2], 0.5, atol=1e-12)


def test_sigmoid_preserves_shape():
    z = np.zeros((3, 4))
    assert sigmoid(z).shape == (3, 4)


# ---------- bce_loss ----------

def test_bce_perfect_vs_wrong():
    y = np.array([1.0, 0.0, 1.0, 0.0])
    # почти идеальные предсказания → loss близок к 0
    p_good = np.array([0.99, 0.01, 0.98, 0.02])
    # уверенно неправильные → loss большой
    p_bad = np.array([0.01, 0.99, 0.02, 0.98])
    loss_good = bce_loss(p_good, y)
    loss_bad = bce_loss(p_bad, y)
    assert loss_good < 0.05
    assert loss_bad > 3.0
    assert loss_good < loss_bad


def test_bce_matches_manual_formula():
    # Сверяем со «школьной» формулой среднего по объектам.
    rng = np.random.default_rng(0)
    p = rng.uniform(0.05, 0.95, size=20)
    y = (rng.uniform(0.0, 1.0, size=20) > 0.5).astype(float)
    expected = float(np.mean(-(y * np.log(p) + (1.0 - y) * np.log(1.0 - p))))
    assert np.allclose(bce_loss(p, y), expected, atol=1e-9)


def test_bce_returns_float():
    p = np.array([0.6, 0.4])
    y = np.array([1.0, 0.0])
    out = bce_loss(p, y)
    assert isinstance(out, float)


def test_bce_protected_against_log0():
    # p ровно 0 и 1 не должны давать nan/inf — благодаря клипу.
    y = np.array([1.0, 0.0])
    p = np.array([0.0, 1.0])  # «худший» случай для log
    loss = bce_loss(p, y)
    assert np.isfinite(loss), "bce_loss дала inf/nan: нужна защита от log(0)"


# ---------- predict_proba ----------

def test_predict_proba_shape_and_range():
    np.random.seed(0)
    X = np.random.randn(7, 3)
    w = np.random.randn(3)
    b = 0.5
    p = predict_proba(X, w, b)
    assert p.shape == (7,)
    assert np.all(p > 0.0) and np.all(p < 1.0)


def test_predict_proba_matches_sigmoid_of_linear():
    np.random.seed(1)
    X = np.random.randn(5, 4)
    w = np.random.randn(4)
    b = -0.3
    expected = sigmoid(X @ w + b)
    assert np.allclose(predict_proba(X, w, b), expected, atol=1e-12)


# ---------- accuracy ----------

def test_accuracy_basic():
    p = np.array([0.9, 0.2, 0.7, 0.4])
    y = np.array([1.0, 0.0, 0.0, 0.0])  # третий объект предскажется как 1 (0.7>=0.5) -> ошибка
    # верные: 0.9->1==1, 0.2->0==0, 0.4->0==0 ; неверный: 0.7->1 != 0
    assert np.allclose(accuracy(p, y), 0.75, atol=1e-12)


def test_accuracy_all_correct_and_threshold():
    p = np.array([0.5, 0.49])
    y = np.array([1.0, 0.0])
    # порог по умолчанию 0.5, сравнение нестрогое: 0.5 -> класс 1
    assert np.allclose(accuracy(p, y), 1.0, atol=1e-12)
    # подвинем порог выше 0.5: теперь 0.5 -> класс 0, первый объект становится ошибкой
    assert np.allclose(accuracy(p, y, thr=0.6), 0.5, atol=1e-12)


def test_accuracy_returns_float():
    p = np.array([0.9, 0.1])
    y = np.array([1.0, 0.0])
    assert isinstance(accuracy(p, y), float)


# ---------- logreg_gradients ----------

def test_gradients_shapes_and_types():
    np.random.seed(2)
    X = np.random.randn(6, 3)
    y = (np.random.uniform(size=6) > 0.5).astype(float)
    w = np.random.randn(3)
    b = 0.1
    dw, db = logreg_gradients(X, y, w, b)
    assert dw.shape == (3,)
    assert isinstance(db, float)


def test_gradients_match_numerical():
    # Численная проверка градиента: сравниваем аналитический dw,db с конечными разностями
    # от функции loss(w, b) = bce_loss(predict_proba(X, w, b), y).
    np.random.seed(3)
    X = np.random.randn(8, 4)
    y = (np.random.uniform(size=8) > 0.5).astype(float)
    w = np.random.randn(4)
    b = 0.2

    def loss_of(w_, b_):
        return bce_loss(predict_proba(X, w_, b_), y)

    dw, db = logreg_gradients(X, y, w, b)

    eps = 1e-6
    num_dw = np.zeros_like(w)
    for i in range(w.size):
        wp = w.copy(); wp[i] += eps
        wm = w.copy(); wm[i] -= eps
        num_dw[i] = (loss_of(wp, b) - loss_of(wm, b)) / (2 * eps)
    num_db = (loss_of(w, b + eps) - loss_of(w, b - eps)) / (2 * eps)

    assert np.allclose(dw, num_dw, atol=1e-5)
    assert np.allclose(db, num_db, atol=1e-5)


def test_training_reduces_loss():
    # Игрушечная, но линейно разделимая задача: два облака точек.
    # Запускаем простой градиентный спуск и проверяем, что loss заметно упал.
    np.random.seed(0)
    N = 100
    # класс 0 вокруг (-2, -2), класс 1 вокруг (+2, +2)
    X0 = np.random.randn(N // 2, 2) + np.array([-2.0, -2.0])
    X1 = np.random.randn(N // 2, 2) + np.array([2.0, 2.0])
    X = np.vstack([X0, X1])
    y = np.concatenate([np.zeros(N // 2), np.ones(N // 2)])

    w = np.zeros(2)
    b = 0.0
    lr = 0.1

    p0 = predict_proba(X, w, b)
    loss_start = bce_loss(p0, y)

    for _ in range(300):
        dw, db = logreg_gradients(X, y, w, b)
        w = w - lr * dw
        b = b - lr * db

    p_end = predict_proba(X, w, b)
    loss_end = bce_loss(p_end, y)

    assert loss_end < 0.5 * loss_start, (
        f"loss не упал достаточно: было {loss_start:.4f}, стало {loss_end:.4f}"
    )
    # на линейно разделимой задаче accuracy должна стать высокой
    assert accuracy(p_end, y) > 0.9


# ---------- softmax ----------

def test_softmax_known_values_1d():
    # z = (2, 1, 0): e^2≈7.389, e^1≈2.718, e^0=1, сумма≈11.107
    z = np.array([2.0, 1.0, 0.0])
    p = softmax(z)
    expected = np.array([0.66524096, 0.24472847, 0.09003057])
    assert np.allclose(p, expected, atol=1e-7)
    assert np.allclose(np.sum(p), 1.0, atol=1e-12)


def test_softmax_uniform_when_equal_logits():
    # одинаковые логиты → равномерное распределение 1/K
    z = np.array([5.0, 5.0, 5.0, 5.0])
    p = softmax(z)
    assert np.allclose(p, 0.25, atol=1e-12)


def test_softmax_rows_sum_to_one_2d():
    z = np.array([[1.0, 2.0, 3.0], [0.0, 0.0, 0.0]])
    p = softmax(z)
    assert p.shape == (2, 3)
    assert np.allclose(np.sum(p, axis=-1), 1.0, atol=1e-12)


def test_softmax_shift_invariance():
    # softmax(z + c) == softmax(z): прибавление константы ко всем логитам ничего не меняет
    z = np.array([1.0, 2.0, 3.0])
    assert np.allclose(softmax(z), softmax(z + 100.0), atol=1e-12)


def test_softmax_numerically_stable():
    # главная проверка стабильности: огромные логиты не дают inf/nan,
    # и ответ совпадает с softmax от сдвинутых логитов (−2, −1, 0)
    z_big = np.array([[1000.0, 1001.0, 1002.0]])
    p = softmax(z_big)
    assert np.all(np.isfinite(p)), "softmax переполнился (inf/nan) на больших логитах"
    assert np.allclose(np.sum(p, axis=-1), 1.0, atol=1e-12)
    expected = softmax(np.array([[-2.0, -1.0, 0.0]]))
    assert np.allclose(p, expected, atol=1e-12)


def test_softmax_reduces_to_sigmoid_binary():
    # бинарный softmax(z_1 − z_0) == сигмоида разности логитов
    z = np.array([0.3, 1.1])  # (z_0, z_1)
    p = softmax(z)
    assert np.allclose(p[1], sigmoid(np.array([z[1] - z[0]]))[0], atol=1e-12)


# ---------- cce_loss ----------

def test_cce_perfect_vs_wrong():
    # один объект, истинный класс 0. «Уверенно правильный» логит → loss мал,
    # «уверенно неправильный» → loss велик.
    logits_good = np.array([[10.0, 0.0, 0.0]])
    logits_bad = np.array([[0.0, 0.0, 10.0]])
    y = np.array([0])
    loss_good = cce_loss(logits_good, y)
    loss_bad = cce_loss(logits_bad, y)
    assert loss_good < 0.01
    assert loss_bad > 5.0
    assert loss_good < loss_bad


def test_cce_uniform_logits_equals_log_k():
    # при равных логитах p = 1/K, loss = −log(1/K) = log(K) независимо от метки
    K = 4
    logits = np.zeros((3, K))
    y = np.array([0, 2, 3])
    assert np.allclose(cce_loss(logits, y), np.log(K), atol=1e-12)


def test_cce_returns_float():
    logits = np.array([[1.0, 2.0]])
    y = np.array([1])
    out = cce_loss(logits, y)
    assert isinstance(out, float)


def test_cce_stable_on_large_logits():
    # log-sum-exp защищает от переполнения: гигантские логиты не дают inf/nan
    logits = np.array([[1000.0, 1001.0, 1002.0]])
    y = np.array([2])
    loss = cce_loss(logits, y)
    assert np.isfinite(loss), "cce_loss дала inf/nan: нужна стабильность через log-sum-exp"


def test_cce_matches_manual_log_softmax():
    # сверяем с явной (стабильной) формулой −log(softmax)_y
    rng = np.random.default_rng(0)
    logits = rng.normal(0, 2, size=(6, 5))
    y = rng.integers(0, 5, size=6)
    p = softmax(logits)
    expected = float(np.mean(-np.log(p[np.arange(6), y])))
    assert np.allclose(cce_loss(logits, y), expected, atol=1e-9)


def test_cce_binary_matches_bce():
    # бинарная CCE (2 логита) численно совпадает с BCE того же предсказания
    rng = np.random.default_rng(1)
    z1 = rng.normal(0, 2, size=8)              # логит класса 1, класс 0 фиксируем нулём
    logits = np.stack([np.zeros_like(z1), z1], axis=1)  # (8, 2)
    y_int = rng.integers(0, 2, size=8)
    p1 = sigmoid(z1)                            # P(class 1) = σ(z1 − 0)
    assert np.allclose(cce_loss(logits, y_int), bce_loss(p1, y_int.astype(float)), atol=1e-9)


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
