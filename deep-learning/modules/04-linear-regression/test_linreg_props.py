# Randomised property tests for module 04 — Linear Regression.
#
# Design goals
# ============
# 1. Every test uses a deterministic seed (varied per test) so failures are
#    reproducible but each test exercises different random inputs.
# 2. Where possible we compare the numpy implementation against a PyTorch
#    oracle (torch.nn.functional / autograd) with tight tolerances.
# 3. We also include finite-difference gradient checks independent of torch.
# 4. Invariants (shape, dtype, non-negativity, boundary values, numerical
#    stability) are checked systematically.
# 5. No module-level calls to linreg functions — all calls are inside test
#    bodies so the file can be IMPORTED (collected) against a stub that raises
#    NotImplementedError without erroring at collection time.

from __future__ import annotations

import numpy as np
import pytest
import torch
import torch.nn.functional as F

from linreg import gd_step, gradients, mse_loss, predict

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _random_Xwyb(rng: np.random.Generator, N: int, D: int):
    """Return (X, w, b, y) with standard-normal entries."""
    X = rng.standard_normal((N, D))
    w = rng.standard_normal(D)
    b = float(rng.standard_normal())
    y = rng.standard_normal(N)
    return X, w, b, y


# ---------------------------------------------------------------------------
# predict — shape, dtype, value invariants
# ---------------------------------------------------------------------------

class TestPredictProperties:
    """Randomised invariants for predict(X, w, b)."""

    @pytest.mark.parametrize("seed,N,D", [
        (10, 1, 1),
        (11, 1, 8),
        (12, 50, 1),
        (13, 50, 8),
        (14, 200, 16),
        (15, 1000, 32),
    ])
    def test_output_shape(self, seed, N, D):
        """predict always returns shape (N,) for any N,D."""
        rng = np.random.default_rng(seed)
        X, w, b, _ = _random_Xwyb(rng, N, D)
        out = predict(X, w, b)
        assert out.shape == (N,), f"expected ({N},), got {out.shape}"

    @pytest.mark.parametrize("seed,N,D", [
        (20, 5, 3),
        (21, 30, 10),
        (22, 100, 5),
    ])
    def test_output_dtype_float(self, seed, N, D):
        """predict should return a float-dtype array."""
        rng = np.random.default_rng(seed)
        X, w, b, _ = _random_Xwyb(rng, N, D)
        out = predict(X, w, b)
        assert np.issubdtype(out.dtype, np.floating), (
            f"expected floating dtype, got {out.dtype}"
        )

    @pytest.mark.parametrize("seed,N,D", [
        (30, 8, 4),
        (31, 64, 12),
        (32, 256, 20),
        (33, 512, 3),
    ])
    def test_oracle_vs_torch_linear(self, seed, N, D):
        """predict(X,w,b) == torch linear layer output (no bias then add b)."""
        rng = np.random.default_rng(seed)
        torch.manual_seed(seed)
        X_np, w_np, b_np, _ = _random_Xwyb(rng, N, D)

        # torch reference: X @ w + b
        X_t = torch.from_numpy(X_np).double()
        w_t = torch.from_numpy(w_np).double()
        b_t = torch.tensor(b_np, dtype=torch.float64)
        expected = (X_t @ w_t + b_t).numpy()

        got = predict(X_np, w_np, b_np)
        assert np.allclose(got, expected, atol=1e-10), (
            f"max abs diff = {np.abs(got - expected).max()}"
        )

    def test_bias_shifts_all_predictions_uniformly(self):
        """Changing b by delta shifts every prediction by the same delta."""
        rng = np.random.default_rng(40)
        X, w, b, _ = _random_Xwyb(rng, 20, 6)
        delta = 3.7
        diff = predict(X, w, b + delta) - predict(X, w, b)
        assert np.allclose(diff, delta), (
            f"expected uniform shift of {delta}, got {diff}"
        )

    def test_zero_weights_give_constant_bias(self):
        """When w == 0, all predictions equal b regardless of X."""
        rng = np.random.default_rng(41)
        N, D = 30, 5
        X = rng.standard_normal((N, D))
        w = np.zeros(D)
        b = 2.718
        out = predict(X, w, b)
        assert np.allclose(out, b), f"expected constant {b}"

    def test_zero_bias_predict_is_linear(self):
        """predict(aX, w, 0) == a * predict(X, w, 0) — linear when b=0."""
        rng = np.random.default_rng(42)
        X, w, _, _ = _random_Xwyb(rng, 15, 4)
        a = 5.0
        b = 0.0
        assert np.allclose(predict(a * X, w, b), a * predict(X, w, b), atol=1e-12)

    def test_single_row_single_feature(self):
        """N=1, D=1 corner case: result must be a length-1 array."""
        X = np.array([[3.0]])
        w = np.array([2.0])
        b = 1.0
        out = predict(X, w, b)
        assert out.shape == (1,)
        assert np.isclose(out[0], 7.0)

    def test_large_magnitude_no_nan_inf(self):
        """Predict should remain finite for large (but not overflow) inputs."""
        rng = np.random.default_rng(43)
        X = rng.standard_normal((50, 10)) * 1e6
        w = rng.standard_normal(10) * 1e6
        b = 1e6
        out = predict(X, w, b)
        assert np.all(np.isfinite(out)), "predict returned NaN or Inf"

    def test_negative_b_correctness(self):
        """Negative bias should work correctly."""
        X = np.array([[1.0, 0.0], [0.0, 1.0]])
        w = np.array([1.0, 1.0])
        b = -5.0
        expected = np.array([-4.0, -4.0])
        assert np.allclose(predict(X, w, b), expected)


# ---------------------------------------------------------------------------
# mse_loss — invariants and oracle
# ---------------------------------------------------------------------------

class TestMseLossProperties:

    @pytest.mark.parametrize("seed,N", [
        (50, 1), (51, 5), (52, 100), (53, 1000),
    ])
    def test_returns_python_float(self, seed, N):
        """mse_loss must return a plain Python float, not a numpy scalar."""
        rng = np.random.default_rng(seed)
        y_pred = rng.standard_normal(N)
        y = rng.standard_normal(N)
        result = mse_loss(y_pred, y)
        assert isinstance(result, float), f"expected float, got {type(result)}"

    @pytest.mark.parametrize("seed,N", [
        (60, 1), (61, 10), (62, 500),
    ])
    def test_non_negative(self, seed, N):
        """MSE >= 0 always."""
        rng = np.random.default_rng(seed)
        y_pred = rng.standard_normal(N)
        y = rng.standard_normal(N)
        assert mse_loss(y_pred, y) >= 0.0

    @pytest.mark.parametrize("N", [1, 2, 50, 200])
    def test_perfect_prediction_gives_zero(self, N):
        """mse_loss(y, y) == 0 for any y."""
        rng = np.random.default_rng(70 + N)
        y = rng.standard_normal(N)
        assert mse_loss(y, y) == 0.0

    @pytest.mark.parametrize("seed,N", [
        (80, 8), (81, 64), (82, 256),
    ])
    def test_oracle_vs_torch_mse(self, seed, N):
        """Compare mse_loss against torch.nn.functional.mse_loss."""
        rng = np.random.default_rng(seed)
        torch.manual_seed(seed)
        y_pred_np = rng.standard_normal(N).astype(np.float64)
        y_np = rng.standard_normal(N).astype(np.float64)

        # torch uses same formula: mean((y_pred - y)^2)
        torch_loss = F.mse_loss(
            torch.from_numpy(y_pred_np).float(),
            torch.from_numpy(y_np).float(),
            reduction="mean",
        ).item()

        got = mse_loss(y_pred_np, y_np)
        assert abs(got - torch_loss) < 1e-5, (
            f"mse_loss={got}, torch={torch_loss}, diff={abs(got-torch_loss)}"
        )

    def test_symmetric(self):
        """MSE(a, b) == MSE(b, a)."""
        rng = np.random.default_rng(90)
        a = rng.standard_normal(40)
        b = rng.standard_normal(40)
        assert np.isclose(mse_loss(a, b), mse_loss(b, a))

    def test_loss_increases_with_error_magnitude(self):
        """Larger prediction errors yield larger MSE."""
        y = np.zeros(20)
        small = np.ones(20) * 0.1
        large = np.ones(20) * 10.0
        assert mse_loss(small, y) < mse_loss(large, y)

    def test_single_element(self):
        """N=1 edge case: loss = (y_pred - y)^2."""
        y_pred = np.array([3.0])
        y = np.array([1.0])
        assert np.isclose(mse_loss(y_pred, y), 4.0)

    def test_known_value_with_negatives(self):
        """Hand-computed: y_pred=[-1,1], y=[1,-1] -> mean(4,4)=4."""
        y_pred = np.array([-1.0, 1.0])
        y = np.array([1.0, -1.0])
        assert np.isclose(mse_loss(y_pred, y), 4.0)

    @pytest.mark.parametrize("seed,N", [(100, 50), (101, 200)])
    def test_scale_sensitivity(self, seed, N):
        """Scaling predictions by k multiplies MSE by k^2 (y=0 reference)."""
        rng = np.random.default_rng(seed)
        y_pred = rng.standard_normal(N)
        y = np.zeros(N)
        k = 3.0
        loss1 = mse_loss(y_pred, y)
        loss2 = mse_loss(k * y_pred, y)
        assert np.isclose(loss2, k**2 * loss1, rtol=1e-9)


# ---------------------------------------------------------------------------
# gradients — oracle vs torch autograd + finite differences
# ---------------------------------------------------------------------------

class TestGradientsProperties:

    @pytest.mark.parametrize("seed,N,D", [
        (110, 5, 3), (111, 20, 6), (112, 100, 10), (113, 1, 4),
    ])
    def test_output_shapes(self, seed, N, D):
        """gradients returns (dw of shape (D,), db of type float)."""
        rng = np.random.default_rng(seed)
        X, w, b, y = _random_Xwyb(rng, N, D)
        dw, db = gradients(X, y, w, b)
        assert dw.shape == (D,), f"dw shape {dw.shape} != ({D},)"
        assert isinstance(db, float), f"db type {type(db)} != float"

    @pytest.mark.parametrize("seed,N,D", [
        (120, 6, 3),
        (121, 32, 8),
        (122, 128, 16),
        (123, 1, 2),
        (124, 500, 5),
    ])
    def test_oracle_vs_torch_autograd(self, seed, N, D):
        """Analytic gradients match torch autograd with atol=1e-6."""
        rng = np.random.default_rng(seed)
        torch.manual_seed(seed)
        X_np, w_np, b_np, y_np = _random_Xwyb(rng, N, D)

        X_t = torch.tensor(X_np, dtype=torch.float64, requires_grad=False)
        w_t = torch.tensor(w_np, dtype=torch.float64, requires_grad=True)
        b_t = torch.tensor(b_np, dtype=torch.float64, requires_grad=True)
        y_t = torch.tensor(y_np, dtype=torch.float64)

        y_pred_t = X_t @ w_t + b_t
        loss_t = F.mse_loss(y_pred_t, y_t, reduction="mean")
        loss_t.backward()

        dw_torch = w_t.grad.numpy()
        db_torch = float(b_t.grad.item())

        dw_np, db_np = gradients(X_np, y_np, w_np, b_np)

        assert np.allclose(dw_np, dw_torch, atol=1e-6), (
            f"dw max diff={np.abs(dw_np - dw_torch).max():.2e}"
        )
        assert abs(db_np - db_torch) < 1e-6, (
            f"db diff={abs(db_np - db_torch):.2e}"
        )

    @pytest.mark.parametrize("seed,N,D", [
        (130, 7, 3),
        (131, 15, 5),
        (132, 50, 8),
    ])
    def test_finite_difference_check(self, seed, N, D):
        """Finite-difference numerical gradient must match analytic gradient."""
        rng = np.random.default_rng(seed)
        X, w, b, y = _random_Xwyb(rng, N, D)

        dw_analytic, db_analytic = gradients(X, y, w, b)

        eps = 1e-5

        # dw via central differences
        dw_num = np.zeros(D)
        for j in range(D):
            wp = w.copy(); wp[j] += eps
            wm = w.copy(); wm[j] -= eps
            dw_num[j] = (
                mse_loss(predict(X, wp, b), y) - mse_loss(predict(X, wm, b), y)
            ) / (2 * eps)

        db_num = (
            mse_loss(predict(X, w, b + eps), y)
            - mse_loss(predict(X, w, b - eps), y)
        ) / (2 * eps)

        assert np.allclose(dw_analytic, dw_num, atol=1e-5), (
            f"dw max diff={np.abs(dw_analytic - dw_num).max():.2e}"
        )
        assert abs(db_analytic - db_num) < 1e-5, (
            f"db diff={abs(db_analytic - db_num):.2e}"
        )

    def test_gradient_zero_at_optimum(self):
        """At the exact optimum (w=w_true, b=b_true, noiseless), gradients~=0."""
        rng = np.random.default_rng(140)
        N, D = 64, 4
        w_true = rng.standard_normal(D)
        b_true = float(rng.standard_normal())
        X = rng.standard_normal((N, D))
        y = X @ w_true + b_true  # noiseless → exact minimum
        dw, db = gradients(X, y, w_true, b_true)
        assert np.allclose(dw, 0.0, atol=1e-10), f"dw not zero: {dw}"
        assert abs(db) < 1e-10, f"db not zero: {db}"

    def test_gradient_direction_reduces_loss(self):
        """Taking a small step against the gradient must decrease loss."""
        rng = np.random.default_rng(141)
        X, w, b, y = _random_Xwyb(rng, 30, 5)
        loss_before = mse_loss(predict(X, w, b), y)
        dw, db = gradients(X, y, w, b)
        lr = 1e-3
        w2 = w - lr * dw
        b2 = b - lr * db
        loss_after = mse_loss(predict(X, w2, b2), y)
        assert loss_after < loss_before, (
            f"gradient step increased loss: {loss_before:.6f} -> {loss_after:.6f}"
        )

    def test_single_sample_gradient(self):
        """N=1 edge case: gradient is well-defined and finite."""
        rng = np.random.default_rng(142)
        X, w, b, y = _random_Xwyb(rng, 1, 4)
        dw, db = gradients(X, y, w, b)
        assert np.all(np.isfinite(dw)), "dw contains NaN or Inf for N=1"
        assert np.isfinite(db), "db is NaN or Inf for N=1"

    def test_gradient_linearity_in_error(self):
        """Scaling errors by k scales gradients by k (X fixed, w=0, b=0)."""
        rng = np.random.default_rng(143)
        N, D = 20, 4
        X = rng.standard_normal((N, D))
        y1 = rng.standard_normal(N)
        k = 3.0
        y2 = y1 / k        # error = -y2 vs error=-y1, so y2 makes k-times smaller err
        w = np.zeros(D)
        b = 0.0
        dw1, db1 = gradients(X, y1, w, b)
        dw2, db2 = gradients(X, k * y1, w, b)  # scaling y by k scales error by k
        # error2 = (0 - k*y1) = k * error1, so grad2 = k * grad1
        assert np.allclose(dw2, k * dw1, atol=1e-10), "gradient not linear in error"
        assert np.isclose(db2, k * db1, atol=1e-10)

    @pytest.mark.parametrize("seed,N,D", [
        (150, 8, 3), (151, 40, 6),
    ])
    def test_large_magnitude_gradients_finite(self, seed, N, D):
        """gradients must be finite even for large-scale inputs."""
        rng = np.random.default_rng(seed)
        X = rng.standard_normal((N, D)) * 100
        w = rng.standard_normal(D) * 100
        b = float(rng.standard_normal() * 100)
        y = rng.standard_normal(N) * 100
        dw, db = gradients(X, y, w, b)
        assert np.all(np.isfinite(dw)), "dw contains NaN/Inf"
        assert np.isfinite(db), "db is NaN/Inf"


# ---------------------------------------------------------------------------
# gd_step — shape, types, direction, invariants
# ---------------------------------------------------------------------------

class TestGdStepProperties:

    @pytest.mark.parametrize("seed,D", [
        (160, 1), (161, 4), (162, 16), (163, 128),
    ])
    def test_output_shapes_and_types(self, seed, D):
        """gd_step preserves shapes and returns (ndarray, float)."""
        rng = np.random.default_rng(seed)
        w = rng.standard_normal(D)
        b = float(rng.standard_normal())
        dw = rng.standard_normal(D)
        db = float(rng.standard_normal())
        lr = 0.01

        w_new, b_new = gd_step(w, b, dw, db, lr)
        assert w_new.shape == (D,), f"w_new shape {w_new.shape} != ({D},)"
        assert isinstance(b_new, float), f"b_new type {type(b_new)} != float"

    @pytest.mark.parametrize("seed,D", [
        (170, 3), (171, 10), (172, 50),
    ])
    def test_step_direction_is_minus_gradient(self, seed, D):
        """w_new == w - lr * dw  exactly (up to fp precision)."""
        rng = np.random.default_rng(seed)
        w = rng.standard_normal(D)
        b = float(rng.standard_normal())
        dw = rng.standard_normal(D)
        db = float(rng.standard_normal())
        lr = float(rng.uniform(0.001, 1.0))

        w_new, b_new = gd_step(w, b, dw, db, lr)
        assert np.allclose(w_new, w - lr * dw, atol=1e-14)
        assert np.isclose(b_new, b - lr * db, atol=1e-14)

    def test_zero_gradient_leaves_params_unchanged(self):
        """If gradients are zero, parameters stay the same."""
        rng = np.random.default_rng(180)
        w = rng.standard_normal(5)
        b = 2.5
        dw = np.zeros(5)
        db = 0.0
        w_new, b_new = gd_step(w, b, dw, db, lr=0.5)
        assert np.allclose(w_new, w)
        assert b_new == b

    def test_zero_lr_leaves_params_unchanged(self):
        """lr=0 must leave all parameters unchanged."""
        rng = np.random.default_rng(181)
        w = rng.standard_normal(6)
        b = -1.3
        dw = rng.standard_normal(6)
        db = float(rng.standard_normal())
        w_new, b_new = gd_step(w, b, dw, db, lr=0.0)
        assert np.allclose(w_new, w)
        assert np.isclose(b_new, b)

    def test_does_not_mutate_input_w(self):
        """gd_step must NOT modify the original w array in place."""
        w = np.array([1.0, 2.0, 3.0])
        w_orig = w.copy()
        dw = np.ones(3)
        gd_step(w, 0.0, dw, 0.0, lr=0.1)
        assert np.allclose(w, w_orig), "gd_step mutated input w"

    @pytest.mark.parametrize("lr", [1e-4, 0.01, 0.1, 1.0])
    def test_step_size_proportional_to_lr(self, lr):
        """||w_new - w|| == lr * ||dw|| when moving along dw."""
        rng = np.random.default_rng(190)
        D = 8
        w = rng.standard_normal(D)
        dw = rng.standard_normal(D)
        b, db = 0.0, 0.0
        w_new, _ = gd_step(w, b, dw, db, lr)
        delta = np.linalg.norm(w_new - w)
        expected = lr * np.linalg.norm(dw)
        assert np.isclose(delta, expected, rtol=1e-10)


# ---------------------------------------------------------------------------
# Full training loop — convergence on synthetic data
# ---------------------------------------------------------------------------

class TestTrainingLoop:

    @pytest.mark.parametrize("seed,N,D,lr,steps", [
        (200, 50, 3,  0.05, 300),
        (201, 100, 5, 0.05, 300),
        (202, 200, 2, 0.1,  200),
        (203, 30,  1, 0.05, 500),
    ])
    def test_convergence_to_known_parameters(self, seed, N, D, lr, steps):
        """Training on noiseless data should recover w_true and b_true."""
        rng = np.random.default_rng(seed)
        w_true = rng.standard_normal(D)
        b_true = float(rng.standard_normal())
        X = rng.standard_normal((N, D))
        y = X @ w_true + b_true   # noiseless

        w = np.zeros(D)
        b = 0.0

        for _ in range(steps):
            dw, db = gradients(X, y, w, b)
            w, b = gd_step(w, b, dw, db, lr)

        assert np.allclose(w, w_true, atol=0.01), (
            f"w did not converge: max diff={np.abs(w - w_true).max():.4f}"
        )
        assert abs(b - b_true) < 0.01, (
            f"b did not converge: diff={abs(b - b_true):.4f}"
        )

    def test_loss_reaches_near_zero_on_noiseless_data(self):
        """On noiseless linear data, final MSE should be essentially zero."""
        rng = np.random.default_rng(210)
        N, D = 80, 6
        w_true = rng.standard_normal(D)
        b_true = float(rng.standard_normal())
        X = rng.standard_normal((N, D))
        y = X @ w_true + b_true

        w = np.zeros(D)
        b = 0.0
        lr = 0.05

        for _ in range(500):
            dw, db = gradients(X, y, w, b)
            w, b = gd_step(w, b, dw, db, lr)

        final_loss = mse_loss(predict(X, w, b), y)
        assert final_loss < 1e-8, f"final loss {final_loss:.2e} not near zero"

    def test_loss_monotone_multiple_seeds(self):
        """Loss should be monotonically non-increasing for multiple random datasets."""
        for seed in [220, 221, 222, 223]:
            rng = np.random.default_rng(seed)
            N, D = 30, 3
            X = rng.standard_normal((N, D))
            y = X @ rng.standard_normal(D) + float(rng.standard_normal())
            w = np.zeros(D)
            b = 0.0
            lr = 0.03

            prev_loss = mse_loss(predict(X, w, b), y)
            for step in range(80):
                dw, db = gradients(X, y, w, b)
                w, b = gd_step(w, b, dw, db, lr)
                cur_loss = mse_loss(predict(X, w, b), y)
                assert cur_loss <= prev_loss + 1e-9, (
                    f"seed={seed}, step={step}: loss increased "
                    f"{prev_loss:.6f} -> {cur_loss:.6f}"
                )
                prev_loss = cur_loss

    def test_loss_reduction_factor_vs_initial(self):
        """After sufficient steps the loss should be < 0.01x initial."""
        rng = np.random.default_rng(230)
        N, D = 100, 4
        w_true = rng.standard_normal(D)
        b_true = float(rng.standard_normal())
        X = rng.standard_normal((N, D))
        y = X @ w_true + b_true + 0.001 * rng.standard_normal(N)

        w = np.zeros(D)
        b = 0.0
        lr = 0.05

        initial_loss = mse_loss(predict(X, w, b), y)
        for _ in range(400):
            dw, db = gradients(X, y, w, b)
            w, b = gd_step(w, b, dw, db, lr)
        final_loss = mse_loss(predict(X, w, b), y)

        assert final_loss < 0.01 * initial_loss, (
            f"insufficient reduction: initial={initial_loss:.4f}, "
            f"final={final_loss:.6f}, ratio={final_loss/initial_loss:.4f}"
        )

    def test_single_feature_convergence(self):
        """D=1 special case: scalar w should converge to correct slope."""
        rng = np.random.default_rng(240)
        N = 50
        x_col = rng.standard_normal(N)
        X = x_col.reshape(N, 1)
        w_true = np.array([4.2])
        b_true = -1.1
        y = X @ w_true + b_true

        w = np.zeros(1)
        b = 0.0

        for _ in range(400):
            dw, db = gradients(X, y, w, b)
            w, b = gd_step(w, b, dw, db, lr=0.05)

        assert np.isclose(w[0], w_true[0], atol=1e-3), (
            f"w[0]={w[0]:.4f}, w_true={w_true[0]}"
        )
        assert np.isclose(b, b_true, atol=1e-3), f"b={b:.4f}, b_true={b_true}"


# ---------------------------------------------------------------------------
# Edge cases — zero inputs, all-same, large magnitudes
# ---------------------------------------------------------------------------

class TestEdgeCases:

    def test_predict_all_zeros(self):
        """predict(0, 0, 0) == 0 array."""
        N, D = 10, 5
        X = np.zeros((N, D))
        w = np.zeros(D)
        b = 0.0
        out = predict(X, w, b)
        assert np.all(out == 0.0)

    def test_mse_both_zero(self):
        """mse_loss(zeros, zeros) == 0."""
        N = 20
        assert mse_loss(np.zeros(N), np.zeros(N)) == 0.0

    def test_gradients_zero_error(self):
        """When y_pred == y, gradients must be zero."""
        rng = np.random.default_rng(250)
        N, D = 15, 4
        X = rng.standard_normal((N, D))
        w = rng.standard_normal(D)
        b = float(rng.standard_normal())
        # y equals the model prediction exactly → zero error
        y = predict(X, w, b)
        dw, db = gradients(X, y, w, b)
        assert np.allclose(dw, 0.0, atol=1e-10)
        assert abs(db) < 1e-10

    def test_mse_large_residuals_finite(self):
        """MSE stays finite for large but representable residuals."""
        y_pred = np.ones(100) * 1e6
        y = np.zeros(100)
        result = mse_loss(y_pred, y)
        assert np.isfinite(result)

    def test_predict_negative_bias(self):
        """Negative bias should produce consistently offset predictions."""
        rng = np.random.default_rng(260)
        X, w, _, _ = _random_Xwyb(rng, 20, 5)
        b = -100.0
        out = predict(X, w, b)
        expected = X @ w + b
        assert np.allclose(out, expected)

    def test_gd_step_negative_gradient_increases_param(self):
        """Negative gradient should increase the parameter after the step."""
        w = np.array([0.0, 0.0])
        dw = np.array([-1.0, -1.0])  # negative gradient → w should increase
        b, db = 0.0, 0.0
        w_new, _ = gd_step(w, b, dw, db, lr=1.0)
        assert np.all(w_new > 0), f"w_new={w_new} should be positive"

    def test_full_pipeline_all_same_targets(self):
        """When all y are the same constant c, optimum has w=0 and b=c."""
        rng = np.random.default_rng(270)
        N, D = 40, 5
        X = rng.standard_normal((N, D))
        c = 7.0
        y = np.full(N, c)

        w = np.zeros(D)
        b = 0.0

        for _ in range(200):
            dw, db = gradients(X, y, w, b)
            w, b = gd_step(w, b, dw, db, lr=0.05)

        assert np.allclose(w, 0.0, atol=0.05), f"w={w} should be near 0"
        assert np.isclose(b, c, atol=0.05), f"b={b} should be near {c}"
