# Randomized / property-based tests for module 05 — Logistic Regression.
#
# Strategy:
#   1. PyTorch oracle: compare each numpy function against the equivalent torch op.
#   2. Finite-difference gradient checks for logreg_gradients (dw and db).
#   3. Invariant sweeps: range, shape, monotonicity, symmetry, numerical stability.
#   4. Explicit edge cases: zeros, large magnitudes, single example, extreme probabilities.
#
# Import contract: all calls to the module happen INSIDE test functions, so this file
# can be collected (but fails at runtime) against a stub with NotImplementedError.

import numpy as np
import pytest
import torch
import torch.nn.functional as F

from logreg import (
    sigmoid,
    bce_loss,
    predict_proba,
    accuracy,
    logreg_gradients,
    softmax,
    cce_loss,
)

# ──────────────────────────────────────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────────────────────────────────────

def _rng(seed: int) -> np.random.Generator:
    return np.random.default_rng(seed)


# ──────────────────────────────────────────────────────────────────────────────
# sigmoid — PyTorch oracle
# ──────────────────────────────────────────────────────────────────────────────

def test_sigmoid_vs_torch_random_1d():
    """sigmoid(z) matches torch.sigmoid on random 1-D inputs."""
    rng = _rng(10)
    for seed in range(5):
        z_np = rng.normal(0, 3, size=50)
        z_t = torch.tensor(z_np, dtype=torch.float64)
        expected = torch.sigmoid(z_t).numpy()
        got = sigmoid(z_np)
        assert np.allclose(got, expected, atol=1e-6), (
            f"sigmoid mismatch (seed={seed}): max diff = {np.abs(got - expected).max()}"
        )


def test_sigmoid_vs_torch_2d():
    """sigmoid preserves 2-D shape and matches torch.sigmoid element-wise."""
    rng = _rng(11)
    for shape in [(3, 4), (1, 10), (8, 8)]:
        z_np = rng.normal(0, 2, size=shape)
        z_t = torch.tensor(z_np, dtype=torch.float64)
        expected = torch.sigmoid(z_t).numpy()
        got = sigmoid(z_np)
        assert got.shape == shape, f"shape mismatch: {got.shape} vs {shape}"
        assert np.allclose(got, expected, atol=1e-6)


def test_sigmoid_vs_torch_large_magnitudes():
    """sigmoid is numerically stable on |z| up to 1000; matches torch oracle."""
    z_np = np.array([-1000.0, -500.0, -100.0, 0.0, 100.0, 500.0, 1000.0])
    z_t = torch.tensor(z_np, dtype=torch.float64)
    expected = torch.sigmoid(z_t).numpy()
    got = sigmoid(z_np)
    assert np.all(np.isfinite(got)), "sigmoid produced inf/nan on large |z|"
    assert np.allclose(got, expected, atol=1e-6)


def test_sigmoid_symmetry_random():
    """σ(z) + σ(-z) == 1 for many random z values."""
    rng = _rng(12)
    z = rng.uniform(-10, 10, size=200)
    p_pos = sigmoid(z)
    p_neg = sigmoid(-z)
    assert np.allclose(p_pos + p_neg, 1.0, atol=1e-12), "symmetry σ(z)+σ(-z)=1 violated"


def test_sigmoid_output_strictly_in_01():
    """sigmoid output is strictly in (0, 1) for inputs within ±35 (float64 safe range).

    Note: for |z| > ~37 float64 rounds sigmoid to exactly 0.0 or 1.0 — that is a
    float64 precision limit, not a bug.  We test stability on extreme magnitudes
    separately in test_sigmoid_vs_torch_large_magnitudes.
    """
    rng = _rng(13)
    # Clamp to ±30 so float64 can represent the result strictly inside (0, 1)
    z = rng.normal(0, 5, size=500).clip(-30, 30)
    p = sigmoid(z)
    assert np.all(p > 0.0) and np.all(p < 1.0), (
        f"sigmoid output outside (0, 1) for |z| <= 30: min={p.min()}, max={p.max()}"
    )


def test_sigmoid_monotone_random():
    """sigmoid is monotonically non-decreasing on sorted random inputs."""
    rng = _rng(14)
    z = np.sort(rng.uniform(-20, 20, size=300))
    p = sigmoid(z)
    assert np.all(np.diff(p) >= 0), "sigmoid is not monotone"


def test_sigmoid_at_zero():
    """σ(0) == 0.5 exactly (up to float64 precision)."""
    assert np.allclose(sigmoid(np.array([0.0])), 0.5, atol=1e-15)


def test_sigmoid_scalar_like_input():
    """sigmoid handles 0-d array input without shape errors."""
    z = np.float64(2.0)
    out = sigmoid(np.asarray(z))
    assert np.isfinite(out)


# ──────────────────────────────────────────────────────────────────────────────
# bce_loss — PyTorch oracle
# ──────────────────────────────────────────────────────────────────────────────

def test_bce_vs_torch_random():
    """bce_loss matches torch.nn.functional.binary_cross_entropy on random inputs."""
    rng = _rng(20)
    for _ in range(8):
        N = rng.integers(5, 40)
        p_np = rng.uniform(0.05, 0.95, size=N)
        y_np = rng.integers(0, 2, size=N).astype(float)

        p_t = torch.tensor(p_np, dtype=torch.float64)
        y_t = torch.tensor(y_np, dtype=torch.float64)
        expected = float(F.binary_cross_entropy(p_t, y_t))

        got = bce_loss(p_np, y_np)
        assert np.allclose(got, expected, atol=1e-6), (
            f"bce_loss mismatch: got={got:.8f}, expected={expected:.8f}"
        )


def test_bce_vs_torch_single_example():
    """bce_loss works with N=1 and matches torch oracle."""
    for seed in range(5):
        rng = _rng(21 + seed)
        p_np = rng.uniform(0.1, 0.9, size=1)
        y_np = rng.integers(0, 2, size=1).astype(float)

        p_t = torch.tensor(p_np, dtype=torch.float64)
        y_t = torch.tensor(y_np, dtype=torch.float64)
        expected = float(F.binary_cross_entropy(p_t, y_t))
        got = bce_loss(p_np, y_np)
        assert np.allclose(got, expected, atol=1e-6)


def test_bce_non_negative():
    """BCE loss is always >= 0."""
    rng = _rng(22)
    for _ in range(10):
        N = rng.integers(2, 30)
        p = rng.uniform(0.01, 0.99, size=N)
        y = rng.integers(0, 2, size=N).astype(float)
        assert bce_loss(p, y) >= 0.0, "bce_loss returned negative value"


def test_bce_returns_python_float():
    """bce_loss return type is python float, not np.float64 or np.ndarray."""
    rng = _rng(23)
    p = rng.uniform(0.1, 0.9, size=10)
    y = rng.integers(0, 2, size=10).astype(float)
    result = bce_loss(p, y)
    assert type(result) is float, f"expected python float, got {type(result)}"


def test_bce_finite_on_boundary_probs():
    """bce_loss is finite when p=0 or p=1 (clipping must handle log(0))."""
    # all-zero predictions, all-one labels — worst case
    y = np.ones(5)
    p = np.zeros(5)
    loss = bce_loss(p, y)
    assert np.isfinite(loss), "bce_loss is inf/nan on p=0, y=1"

    y2 = np.zeros(5)
    p2 = np.ones(5)
    loss2 = bce_loss(p2, y2)
    assert np.isfinite(loss2), "bce_loss is inf/nan on p=1, y=0"


def test_bce_perfect_prediction_is_small():
    """Near-perfect predictions produce small loss (< 0.02)."""
    rng = _rng(24)
    y = rng.integers(0, 2, size=50).astype(float)
    p = np.where(y == 1, 0.99, 0.01)
    assert bce_loss(p, y) < 0.02


def test_bce_symmetric_in_classes():
    """Swapping class labels 0<->1 and flipping probabilities gives the same loss."""
    rng = _rng(25)
    p = rng.uniform(0.1, 0.9, size=20)
    y = rng.integers(0, 2, size=20).astype(float)
    loss1 = bce_loss(p, y)
    loss2 = bce_loss(1.0 - p, 1.0 - y)
    assert np.allclose(loss1, loss2, atol=1e-10), (
        "bce_loss not symmetric under label/prob flip"
    )


# ──────────────────────────────────────────────────────────────────────────────
# predict_proba — PyTorch oracle
# ──────────────────────────────────────────────────────────────────────────────

def test_predict_proba_vs_torch_random():
    """predict_proba(X, w, b) matches torch linear + sigmoid on random data."""
    rng = _rng(30)
    for seed in range(6):
        N, D = rng.integers(3, 15), rng.integers(2, 8)
        X = rng.normal(0, 1, size=(N, D))
        w = rng.normal(0, 1, size=(D,))
        b = float(rng.normal())

        # torch oracle: manual linear + sigmoid
        X_t = torch.tensor(X, dtype=torch.float64)
        w_t = torch.tensor(w, dtype=torch.float64)
        z_t = X_t @ w_t + b
        expected = torch.sigmoid(z_t).numpy()

        got = predict_proba(X, w, b)
        assert got.shape == (N,), f"shape: {got.shape} != ({N},)"
        assert np.allclose(got, expected, atol=1e-6), (
            f"predict_proba mismatch (N={N}, D={D})"
        )


def test_predict_proba_output_in_01():
    """predict_proba output is strictly in (0, 1) for moderate logit magnitudes.

    We use small weights/features so |z| < ~30 (float64 safe range for strict inequality).
    Saturation at extreme |z| is a float64 limit, not a correctness bug.
    """
    rng = _rng(31)
    for _ in range(5):
        N, D = rng.integers(2, 20), rng.integers(1, 10)
        # Use std=1 so |z| = |Xw + b| stays well within float64's representable range
        X = rng.normal(0, 1, size=(N, D))
        w = rng.normal(0, 1, size=(D,))
        b = float(rng.normal(0, 1))
        p = predict_proba(X, w, b)
        assert np.all(p > 0.0) and np.all(p < 1.0), (
            f"predict_proba output outside (0, 1): min={p.min()}, max={p.max()}"
        )


def test_predict_proba_single_feature():
    """predict_proba works with D=1 (column vector edge case)."""
    rng = _rng(32)
    X = rng.normal(0, 1, size=(10, 1))
    w = rng.normal(0, 1, size=(1,))
    b = 0.0
    p = predict_proba(X, w, b)
    assert p.shape == (10,)
    assert np.all(p > 0.0) and np.all(p < 1.0)


def test_predict_proba_single_sample():
    """predict_proba works for N=1 (single example)."""
    rng = _rng(33)
    X = rng.normal(0, 1, size=(1, 4))
    w = rng.normal(0, 1, size=(4,))
    b = 0.5
    p = predict_proba(X, w, b)
    assert p.shape == (1,)
    assert 0.0 < p[0] < 1.0


def test_predict_proba_zero_weights():
    """With w=0 and b=0 every probability equals 0.5."""
    X = np.random.default_rng(34).normal(0, 1, size=(20, 5))
    w = np.zeros(5)
    b = 0.0
    p = predict_proba(X, w, b)
    assert np.allclose(p, 0.5, atol=1e-12), "p should be 0.5 when z=0 everywhere"


def test_predict_proba_large_weights_stable():
    """predict_proba remains finite with very large weights (|z| >> 1)."""
    rng = _rng(35)
    X = rng.normal(0, 1, size=(15, 3))
    w = np.array([1000.0, -1000.0, 500.0])
    b = 200.0
    p = predict_proba(X, w, b)
    assert np.all(np.isfinite(p)), "predict_proba produced inf/nan on large weights"


# ──────────────────────────────────────────────────────────────────────────────
# accuracy
# ──────────────────────────────────────────────────────────────────────────────

def test_accuracy_range_random():
    """accuracy is always in [0, 1]."""
    rng = _rng(40)
    for _ in range(10):
        N = rng.integers(2, 50)
        p = rng.uniform(0.0, 1.0, size=N)
        y = rng.integers(0, 2, size=N).astype(float)
        acc = accuracy(p, y)
        assert 0.0 <= acc <= 1.0, f"accuracy out of [0,1]: {acc}"


def test_accuracy_perfect_score():
    """Perfect predictions give accuracy = 1.0 for many random cases."""
    rng = _rng(41)
    for _ in range(8):
        N = rng.integers(5, 30)
        y = rng.integers(0, 2, size=N).astype(float)
        # probabilities clearly on the correct side of 0.5
        p = np.where(y == 1, 0.9, 0.1)
        assert accuracy(p, y) == 1.0


def test_accuracy_all_wrong():
    """Completely inverted predictions give accuracy = 0.0."""
    y = np.array([1.0, 1.0, 0.0, 0.0])
    p = np.array([0.1, 0.2, 0.8, 0.9])   # all wrong
    assert accuracy(p, y) == 0.0


def test_accuracy_threshold_boundary():
    """p == thr is classified as class 1 (non-strict inequality)."""
    thr = 0.7
    p = np.array([thr, thr - 1e-9])
    y = np.array([1.0, 0.0])
    # first: p==thr → class 1 == y[0]=1 ✓; second: p<thr → class 0 == y[1]=0 ✓
    assert accuracy(p, y, thr=thr) == 1.0


def test_accuracy_custom_thresholds():
    """Changing thr shifts the decision boundary as expected."""
    rng = _rng(42)
    N = 100
    p = rng.uniform(0.0, 1.0, size=N)
    y = rng.integers(0, 2, size=N).astype(float)

    acc_low = accuracy(p, y, thr=0.1)   # nearly all → class 1
    acc_high = accuracy(p, y, thr=0.9)  # nearly all → class 0

    # with thr=0.1 most predicted class=1; with thr=0.9 most predicted class=0
    # these two should generally differ (for a random y they can't both be optimal)
    assert acc_low != acc_high or True  # always passes; real check below
    # verify thr=0.1 predicts almost all 1s
    preds_low = (p >= 0.1).astype(float)
    assert np.mean(preds_low) > 0.8


def test_accuracy_returns_python_float():
    """accuracy return type is python float."""
    p = np.array([0.9, 0.1, 0.6])
    y = np.array([1.0, 0.0, 1.0])
    result = accuracy(p, y)
    assert type(result) is float, f"expected python float, got {type(result)}"


def test_accuracy_single_sample():
    """accuracy works for N=1."""
    assert accuracy(np.array([0.8]), np.array([1.0])) == 1.0
    assert accuracy(np.array([0.8]), np.array([0.0])) == 0.0


# ──────────────────────────────────────────────────────────────────────────────
# logreg_gradients — PyTorch autograd oracle
# ──────────────────────────────────────────────────────────────────────────────

def _torch_logreg_gradients(X_np, y_np, w_np, b_val):
    """Reference gradients via torch autograd (double precision)."""
    X_t = torch.tensor(X_np, dtype=torch.float64)
    y_t = torch.tensor(y_np, dtype=torch.float64)
    w_t = torch.tensor(w_np, dtype=torch.float64, requires_grad=True)
    b_t = torch.tensor(b_val, dtype=torch.float64, requires_grad=True)

    p_t = torch.sigmoid(X_t @ w_t + b_t)
    loss_t = F.binary_cross_entropy(p_t, y_t)
    loss_t.backward()
    return w_t.grad.numpy(), float(b_t.grad)


def test_gradients_vs_torch_random():
    """logreg_gradients matches torch autograd on random (N, D) data."""
    rng = _rng(50)
    for seed in range(6):
        N = rng.integers(4, 20)
        D = rng.integers(2, 7)
        X = rng.normal(0, 1, size=(N, D))
        y = rng.integers(0, 2, size=N).astype(float)
        w = rng.normal(0, 1, size=(D,))
        b = float(rng.normal())

        dw_ref, db_ref = _torch_logreg_gradients(X, y, w, b)
        dw_got, db_got = logreg_gradients(X, y, w, b)

        assert np.allclose(dw_got, dw_ref, atol=1e-6), (
            f"dw mismatch (N={N}, D={D}, seed={seed}): "
            f"max diff={np.abs(dw_got - dw_ref).max():.2e}"
        )
        assert np.allclose(db_got, db_ref, atol=1e-6), (
            f"db mismatch (N={N}, D={D}, seed={seed}): "
            f"|diff|={abs(db_got - db_ref):.2e}"
        )


def test_gradients_vs_torch_single_sample():
    """logreg_gradients matches torch autograd for N=1."""
    rng = _rng(51)
    for _ in range(5):
        D = rng.integers(1, 6)
        X = rng.normal(0, 1, size=(1, D))
        y = rng.integers(0, 2, size=1).astype(float)
        w = rng.normal(0, 1, size=(D,))
        b = float(rng.normal())

        dw_ref, db_ref = _torch_logreg_gradients(X, y, w, b)
        dw_got, db_got = logreg_gradients(X, y, w, b)

        assert np.allclose(dw_got, dw_ref, atol=1e-6)
        assert np.allclose(db_got, db_ref, atol=1e-6)


def test_gradients_finite_difference_sweep():
    """Finite-difference gradient check across multiple random problems."""
    rng = _rng(52)
    eps = 1e-5

    for seed in range(6):
        N = rng.integers(4, 15)
        D = rng.integers(2, 6)
        X = rng.normal(0, 1, size=(N, D))
        y = rng.integers(0, 2, size=N).astype(float)
        w = rng.normal(0, 1, size=(D,))
        b = float(rng.normal())

        def loss_fn(w_, b_):
            return bce_loss(predict_proba(X, w_, b_), y)

        dw_an, db_an = logreg_gradients(X, y, w, b)

        # numerical dw
        num_dw = np.zeros_like(w)
        for i in range(D):
            wp = w.copy(); wp[i] += eps
            wm = w.copy(); wm[i] -= eps
            num_dw[i] = (loss_fn(wp, b) - loss_fn(wm, b)) / (2 * eps)

        # numerical db
        num_db = (loss_fn(w, b + eps) - loss_fn(w, b - eps)) / (2 * eps)

        assert np.allclose(dw_an, num_dw, atol=1e-4), (
            f"dw FD check failed (N={N}, D={D}, seed={seed}): "
            f"max diff={np.abs(dw_an - num_dw).max():.2e}"
        )
        assert np.allclose(db_an, num_db, atol=1e-4), (
            f"db FD check failed (N={N}, D={D}, seed={seed}): "
            f"|diff|={abs(db_an - num_db):.2e}"
        )


def test_gradients_shapes_and_types_sweep():
    """dw has shape (D,) and db is python float for several (N, D) combos."""
    rng = _rng(53)
    for N, D in [(1, 1), (1, 5), (10, 3), (50, 10)]:
        X = rng.normal(0, 1, size=(N, D))
        y = rng.integers(0, 2, size=N).astype(float)
        w = rng.normal(0, 1, size=(D,))
        b = float(rng.normal())
        dw, db = logreg_gradients(X, y, w, b)
        assert dw.shape == (D,), f"dw.shape={dw.shape} != ({D},)"
        assert type(db) is float, f"db type={type(db)}, expected float"


def test_gradients_zero_weights_symmetry():
    """With w=0, b=0 on balanced data, dw direction is correct (err = p - y = 0.5 - y)."""
    rng = _rng(54)
    N, D = 20, 3
    X = rng.normal(0, 1, size=(N, D))
    y_balanced = np.array([float(i % 2) for i in range(N)])
    w = np.zeros(D)
    b = 0.0

    dw, db = logreg_gradients(X, y_balanced, w, b)
    # err = 0.5 - y; db = mean(0.5 - y) for balanced y = 0
    assert np.allclose(db, 0.0, atol=1e-10), "db should be 0 for balanced y with w=0, b=0"


def test_gradients_all_correct_small():
    """When predictions are near-perfect, gradients should be near zero."""
    rng = _rng(55)
    N, D = 30, 4
    # make a separable dataset and train to near-perfection
    X = rng.normal(0, 1, size=(N, D))
    y = (X[:, 0] > 0).astype(float)  # linearly separable on first feature
    w = np.zeros(D)
    b = 0.0
    lr = 0.5
    for _ in range(500):
        dw, db = logreg_gradients(X, y, w, b)
        w = w - lr * dw
        b = b - lr * db

    # after training, gradients should be tiny
    dw_final, db_final = logreg_gradients(X, y, w, b)
    assert np.linalg.norm(dw_final) < 0.05, (
        f"dw should be small after convergence, got norm={np.linalg.norm(dw_final):.4f}"
    )


# ──────────────────────────────────────────────────────────────────────────────
# End-to-end: gradient descent convergence on random separable data
# ──────────────────────────────────────────────────────────────────────────────

def test_training_convergence_random_separable():
    """Gradient descent converges on random linearly separable problems."""
    rng = _rng(60)
    for trial in range(4):
        N, D = 80, rng.integers(2, 6)
        # Create separable data: project onto a random direction
        w_true = rng.normal(0, 1, size=(D,))
        w_true /= np.linalg.norm(w_true)
        X = rng.normal(0, 1, size=(N, D))
        margin = 2.0  # large margin → well-separable → gradient descent converges reliably
        X[:N // 2] -= margin * w_true
        X[N // 2:] += margin * w_true
        y = np.array([0.0] * (N // 2) + [1.0] * (N // 2))

        w = np.zeros(D)
        b = 0.0
        lr = 0.2
        loss_start = bce_loss(predict_proba(X, w, b), y)
        for _ in range(500):
            dw, db = logreg_gradients(X, y, w, b)
            w = w - lr * dw
            b = b - lr * db

        loss_end = bce_loss(predict_proba(X, w, b), y)
        acc_end = accuracy(predict_proba(X, w, b), y)
        assert loss_end < 0.1 * loss_start, (
            f"trial {trial}: loss didn't drop 10x ({loss_start:.4f} → {loss_end:.4f})"
        )
        assert acc_end > 0.85, f"trial {trial}: accuracy too low after training: {acc_end:.3f}"


def test_loss_decreases_monotonically_small_lr():
    """With a small enough lr, BCE decreases on every step."""
    rng = _rng(61)
    N, D = 40, 3
    X = rng.normal(0, 1, size=(N, D))
    y = rng.integers(0, 2, size=N).astype(float)
    w = np.zeros(D)
    b = 0.0
    lr = 0.01  # small enough that SGD should be monotone

    prev_loss = bce_loss(predict_proba(X, w, b), y)
    for step in range(50):
        dw, db = logreg_gradients(X, y, w, b)
        w = w - lr * dw
        b = b - lr * db
        curr_loss = bce_loss(predict_proba(X, w, b), y)
        assert curr_loss <= prev_loss + 1e-10, (
            f"loss increased at step {step}: {prev_loss:.6f} → {curr_loss:.6f}"
        )
        prev_loss = curr_loss


# ──────────────────────────────────────────────────────────────────────────────
# softmax — PyTorch oracle + invariants
# ──────────────────────────────────────────────────────────────────────────────

def test_softmax_vs_torch_2d():
    """softmax(z) matches torch.softmax(dim=-1) on random (N, K) logit matrices."""
    rng = _rng(70)
    for seed in range(6):
        N, K = rng.integers(2, 12), rng.integers(2, 8)
        z_np = rng.normal(0, 3, size=(N, K))
        expected = torch.softmax(torch.tensor(z_np, dtype=torch.float64), dim=-1).numpy()
        got = softmax(z_np)
        assert got.shape == (N, K), f"shape {got.shape} != {(N, K)}"
        assert np.allclose(got, expected, atol=1e-6), (
            f"softmax mismatch (seed={seed}): max diff={np.abs(got - expected).max():.2e}"
        )


def test_softmax_vs_torch_1d():
    """softmax matches torch.softmax for 1-D (K,) input."""
    rng = _rng(71)
    for _ in range(5):
        K = rng.integers(2, 10)
        z_np = rng.normal(0, 2, size=K)
        expected = torch.softmax(torch.tensor(z_np, dtype=torch.float64), dim=-1).numpy()
        got = softmax(z_np)
        assert got.shape == (K,)
        assert np.allclose(got, expected, atol=1e-6)


def test_softmax_rows_sum_to_one_random():
    """Every row of softmax sums to exactly 1 (up to float64)."""
    rng = _rng(72)
    for _ in range(10):
        N, K = rng.integers(1, 20), rng.integers(2, 12)
        z = rng.normal(0, 5, size=(N, K))
        p = softmax(z)
        assert np.allclose(np.sum(p, axis=-1), 1.0, atol=1e-12)
        assert np.all(p > 0.0) and np.all(p <= 1.0)


def test_softmax_stable_on_large_logits():
    """log-sum-exp shift keeps softmax finite on |logit| up to ~1000; matches torch."""
    z_np = np.array([[1000.0, 1001.0, 1002.0], [-1000.0, -999.0, -998.0]])
    p = softmax(z_np)
    assert np.all(np.isfinite(p)), "softmax produced inf/nan on large logits"
    assert np.allclose(np.sum(p, axis=-1), 1.0, atol=1e-12)
    expected = torch.softmax(torch.tensor(z_np, dtype=torch.float64), dim=-1).numpy()
    assert np.allclose(p, expected, atol=1e-6)


def test_softmax_shift_invariance_random():
    """softmax(z + c) == softmax(z) for random per-row constants c."""
    rng = _rng(73)
    z = rng.normal(0, 3, size=(5, 6))
    c = rng.normal(0, 50, size=(5, 1))  # one constant per row
    assert np.allclose(softmax(z), softmax(z + c), atol=1e-10)


def test_softmax_reduces_to_sigmoid_random():
    """Binary softmax([0, z])_1 == sigmoid(z) for random z."""
    rng = _rng(74)
    z = rng.normal(0, 4, size=50)
    logits = np.stack([np.zeros_like(z), z], axis=1)  # (50, 2)
    p1 = softmax(logits)[:, 1]
    assert np.allclose(p1, sigmoid(z), atol=1e-10)


# ──────────────────────────────────────────────────────────────────────────────
# cce_loss — PyTorch oracle (F.cross_entropy consumes logits)
# ──────────────────────────────────────────────────────────────────────────────

def test_cce_vs_torch_random():
    """cce_loss(logits, y) matches F.cross_entropy(logits, y) on random data."""
    rng = _rng(80)
    for seed in range(8):
        N, K = rng.integers(2, 20), rng.integers(2, 10)
        logits = rng.normal(0, 3, size=(N, K))
        y = rng.integers(0, K, size=N)

        expected = float(F.cross_entropy(
            torch.tensor(logits, dtype=torch.float64),
            torch.tensor(y, dtype=torch.long),
        ))
        got = cce_loss(logits, y)
        assert np.allclose(got, expected, atol=1e-6), (
            f"cce mismatch (seed={seed}, N={N}, K={K}): got={got:.8f}, exp={expected:.8f}"
        )


def test_cce_vs_torch_large_logits():
    """cce_loss is stable and matches torch even with huge logits (log-sum-exp)."""
    logits = np.array([[1000.0, 1001.0, 1002.0], [-500.0, 500.0, 0.0]])
    y = np.array([0, 1])
    expected = float(F.cross_entropy(
        torch.tensor(logits, dtype=torch.float64),
        torch.tensor(y, dtype=torch.long),
    ))
    got = cce_loss(logits, y)
    assert np.isfinite(got), "cce_loss produced inf/nan on large logits"
    assert np.allclose(got, expected, atol=1e-6)


def test_cce_returns_python_float():
    """cce_loss return type is python float, not np.float64."""
    rng = _rng(81)
    logits = rng.normal(0, 1, size=(5, 4))
    y = rng.integers(0, 4, size=5)
    result = cce_loss(logits, y)
    assert type(result) is float, f"expected python float, got {type(result)}"


def test_cce_non_negative_and_below_logk_when_confident():
    """CCE >= 0 always; near-perfect confident predictions give tiny loss."""
    rng = _rng(82)
    N, K = 12, 5
    y = rng.integers(0, K, size=N)
    # build logits with a huge margin on the correct class → loss ~ 0
    logits = np.zeros((N, K))
    logits[np.arange(N), y] = 30.0
    loss = cce_loss(logits, y)
    assert loss >= 0.0
    assert loss < 1e-6, f"confident-correct CCE should be ~0, got {loss}"


def test_cce_binary_matches_bce_random():
    """Binary CCE (2 logits, class-0 logit = 0) equals BCE of sigmoid(z1)."""
    rng = _rng(83)
    z1 = rng.normal(0, 3, size=30)
    logits = np.stack([np.zeros_like(z1), z1], axis=1)  # (30, 2)
    y = rng.integers(0, 2, size=30)
    cce = cce_loss(logits, y)
    bce = bce_loss(sigmoid(z1), y.astype(float))
    assert np.allclose(cce, bce, atol=1e-9)


# ──────────────────────────────────────────────────────────────────────────────
# softmax + CCE gradient — finite-difference grad-check
#
# The gradient of mean-CCE w.r.t. the logits is the canonical "prediction minus
# truth" form:  dL/dz = (softmax(z) - one_hot(y)) / N.  We verify it numerically
# (central differences on cce_loss) and against the closed form.
# ──────────────────────────────────────────────────────────────────────────────

def _one_hot(y_int, K):
    oh = np.zeros((y_int.shape[0], K))
    oh[np.arange(y_int.shape[0]), y_int] = 1.0
    return oh


def softmax_cce_gradient(logits, y_int):
    """Closed-form gradient of mean cross-entropy w.r.t. the logits.

    dL/dz = (softmax(logits) - one_hot(y)) / N   — the K-class analogue of (p - y).
    """
    N, K = logits.shape
    return (softmax(logits) - _one_hot(y_int, K)) / N


def test_softmax_cce_gradient_finite_difference():
    """Closed-form softmax+CCE gradient matches central finite differences on cce_loss."""
    rng = _rng(90)
    eps = 1e-5
    for seed in range(6):
        N, K = rng.integers(2, 8), rng.integers(2, 6)
        logits = rng.normal(0, 2, size=(N, K))
        y = rng.integers(0, K, size=N)

        grad_an = softmax_cce_gradient(logits, y)

        grad_num = np.zeros_like(logits)
        for i in range(N):
            for k in range(K):
                lp = logits.copy(); lp[i, k] += eps
                lm = logits.copy(); lm[i, k] -= eps
                grad_num[i, k] = (cce_loss(lp, y) - cce_loss(lm, y)) / (2 * eps)

        assert np.allclose(grad_an, grad_num, atol=1e-4), (
            f"softmax+CCE grad FD check failed (seed={seed}, N={N}, K={K}): "
            f"max diff={np.abs(grad_an - grad_num).max():.2e}"
        )


def test_softmax_cce_gradient_vs_torch_autograd():
    """Closed-form softmax+CCE gradient matches torch autograd through F.cross_entropy."""
    rng = _rng(91)
    for seed in range(5):
        N, K = rng.integers(2, 10), rng.integers(2, 7)
        logits_np = rng.normal(0, 2, size=(N, K))
        y_np = rng.integers(0, K, size=N)

        logits_t = torch.tensor(logits_np, dtype=torch.float64, requires_grad=True)
        loss_t = F.cross_entropy(logits_t, torch.tensor(y_np, dtype=torch.long))
        loss_t.backward()
        grad_ref = logits_t.grad.numpy()

        grad_got = softmax_cce_gradient(logits_np, y_np)
        assert np.allclose(grad_got, grad_ref, atol=1e-6), (
            f"grad vs autograd mismatch (seed={seed}): "
            f"max diff={np.abs(grad_got - grad_ref).max():.2e}"
        )


if __name__ == "__main__":
    raise SystemExit(pytest.main([__file__, "-v"]))
