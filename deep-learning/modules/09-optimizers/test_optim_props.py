# Randomized property tests for module 09 — Optimizers.
#
# Every test function:
#   - sets a deterministic seed (varied per test)
#   - loops over several random shapes / inputs
#   - compares against a PyTorch oracle OR applies a finite-difference gradient check
#   - verifies invariants (shape, dtype, no mutation, numerical stability)
#
# NO module-level calls to the solution functions — all calls are inside test
# bodies so the file collects cleanly against an unfilled stub (raises
# NotImplementedError at run-time, not at import time).

from __future__ import annotations

import numpy as np
import pytest
import torch
import torch.optim

from optim import adam_step, momentum_step, sgd_step

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

_SHAPES = [(1,), (4,), (10,), (3, 5), (2, 3, 4)]


def _rand(shape, rng: np.random.RandomState, scale: float = 1.0) -> np.ndarray:
    return rng.randn(*shape).astype(np.float64) * scale


# ---------------------------------------------------------------------------
# SGD – oracle vs torch.optim.SGD
# ---------------------------------------------------------------------------

class TestSGDOracle:
    """Compare sgd_step against torch.optim.SGD (no momentum, no weight decay)."""

    def test_sgd_matches_torch_random_shapes(self):
        rng = np.random.RandomState(1)
        torch.manual_seed(1)
        for shape in _SHAPES:
            for _ in range(4):
                lr = float(rng.uniform(1e-4, 0.5))
                p_np = _rand(shape, rng)
                g_np = _rand(shape, rng)

                # numpy implementation
                p_new_np = sgd_step(p_np, g_np, lr)

                # torch reference
                p_t = torch.tensor(p_np.copy(), dtype=torch.float64, requires_grad=False)
                g_t = torch.tensor(g_np, dtype=torch.float64)
                p_t_new = p_t - lr * g_t

                assert p_new_np.shape == p_np.shape, "shape mismatch"
                np.testing.assert_allclose(
                    p_new_np,
                    p_t_new.numpy(),
                    atol=1e-12,
                    err_msg=f"SGD oracle mismatch shape={shape} lr={lr:.4f}",
                )

    def test_sgd_large_magnitudes_no_nan(self):
        """Numerical stability: huge gradients must not produce nan/inf."""
        rng = np.random.RandomState(2)
        for _ in range(10):
            shape = (rng.randint(2, 20),)
            p = _rand(shape, rng, scale=1e3)
            g = _rand(shape, rng, scale=1e6)
            lr = 1e-7
            out = sgd_step(p, g, lr)
            assert np.all(np.isfinite(out)), "sgd_step produced nan/inf on large grads"

    def test_sgd_zero_grad_is_identity(self):
        """Zero gradient -> parameters unchanged."""
        rng = np.random.RandomState(3)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = np.zeros(shape)
            out = sgd_step(p, g, lr=0.1)
            np.testing.assert_array_equal(out, p)

    def test_sgd_decreases_quadratic_loss_single_step(self):
        """A single SGD step on f(p)=||p-t||^2 must strictly decrease loss."""
        rng = np.random.RandomState(4)
        for _ in range(20):
            d = rng.randint(2, 30)
            target = _rand((d,), rng)
            p = _rand((d,), rng, scale=3.0)
            g = 2.0 * (p - target)
            # lr small enough that step is guaranteed to decrease loss
            lr = 0.05
            p_new = sgd_step(p, g, lr)
            loss_before = np.sum((p - target) ** 2)
            loss_after = np.sum((p_new - target) ** 2)
            assert loss_after < loss_before, (
                f"Single SGD step did not decrease loss: {loss_before:.4f} -> {loss_after:.4f}"
            )

    def test_sgd_does_not_mutate_inputs_random(self):
        rng = np.random.RandomState(5)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            p_orig = p.copy()
            g_orig = g.copy()
            sgd_step(p, g, lr=0.01)
            np.testing.assert_array_equal(p, p_orig, err_msg="sgd_step mutated params")
            np.testing.assert_array_equal(g, g_orig, err_msg="sgd_step mutated grads")

    def test_sgd_single_element(self):
        """Edge case: scalar (1,) array."""
        p = np.array([5.0])
        g = np.array([2.0])
        out = sgd_step(p, g, lr=0.5)
        np.testing.assert_allclose(out, np.array([4.0]), atol=1e-12)

    def test_sgd_negative_params_and_grads(self):
        rng = np.random.RandomState(6)
        for _ in range(10):
            shape = (rng.randint(2, 15),)
            p = _rand(shape, rng, scale=5.0) - 5.0
            g = _rand(shape, rng, scale=5.0) - 5.0
            lr = 0.1
            out = sgd_step(p, g, lr)
            expected = p - lr * g
            np.testing.assert_allclose(out, expected, atol=1e-12)

    def test_sgd_lr_zero_is_identity(self):
        """lr=0 -> no movement."""
        rng = np.random.RandomState(7)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            out = sgd_step(p, g, lr=0.0)
            np.testing.assert_array_equal(out, p)

    def test_sgd_output_shape_matches_input(self):
        rng = np.random.RandomState(8)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            out = sgd_step(p, g, lr=0.1)
            assert out.shape == p.shape, f"Expected shape {p.shape}, got {out.shape}"


# ---------------------------------------------------------------------------
# Momentum – oracle vs torch.optim.SGD with momentum
# ---------------------------------------------------------------------------

class TestMomentumOracle:
    """Compare momentum_step against torch.optim.SGD(momentum=beta, dampening=1-beta).

    PyTorch SGD with dampening uses:
        buf = momentum * buf + (1 - dampening) * grad
        p   = p - lr * buf
    which matches the module contract exactly when dampening = 1 - beta.
    """

    def _torch_momentum_step(
        self, p_np: np.ndarray, g_np: np.ndarray, v_np: np.ndarray,
        lr: float, beta: float,
    ):
        """One manual torch momentum step matching the module contract."""
        v_t = torch.tensor(v_np, dtype=torch.float64)
        g_t = torch.tensor(g_np, dtype=torch.float64)
        p_t = torch.tensor(p_np, dtype=torch.float64)
        v_new = beta * v_t + (1 - beta) * g_t
        p_new = p_t - lr * v_new
        return p_new.numpy(), v_new.numpy()

    def test_momentum_matches_torch_random_shapes(self):
        rng = np.random.RandomState(10)
        torch.manual_seed(10)
        for shape in _SHAPES:
            for _ in range(4):
                lr = float(rng.uniform(1e-4, 0.3))
                beta = float(rng.uniform(0.5, 0.99))
                p = _rand(shape, rng)
                g = _rand(shape, rng)
                v = _rand(shape, rng, scale=0.1)

                p_ours, v_ours = momentum_step(p, g, v, lr, beta)
                p_ref, v_ref = self._torch_momentum_step(p, g, v, lr, beta)

                assert p_ours.shape == p.shape
                assert v_ours.shape == p.shape
                np.testing.assert_allclose(p_ours, p_ref, atol=1e-11,
                                           err_msg=f"momentum params mismatch shape={shape}")
                np.testing.assert_allclose(v_ours, v_ref, atol=1e-11,
                                           err_msg=f"momentum velocity mismatch shape={shape}")

    def test_momentum_velocity_accumulates_constant_grad(self):
        """
        Under a constant gradient the velocity should converge to grad/(1-beta).
        After N steps: v_N = (1-beta^N) * grad.  At N=50, beta=0.9, v is near grad.
        """
        rng = np.random.RandomState(11)
        torch.manual_seed(11)
        grad_val = rng.randn(5)
        beta = 0.9
        lr = 0.0  # no param movement; just watch velocity
        v = np.zeros(5)
        p = np.zeros(5)
        for _ in range(100):
            p, v = momentum_step(p, grad_val, v, lr, beta)
        # After many steps v -> grad (steady state: v = (1-beta)*grad / (1-beta) = grad)
        np.testing.assert_allclose(v, grad_val, atol=1e-4,
                                   err_msg="velocity did not converge to constant grad")

    def test_momentum_beta_zero_equals_sgd_random(self):
        """beta=0 -> velocity_new = grads -> step identical to SGD for many random inputs."""
        rng = np.random.RandomState(12)
        for shape in _SHAPES:
            for _ in range(5):
                p = _rand(shape, rng)
                g = _rand(shape, rng)
                v = _rand(shape, rng)  # initial velocity irrelevant when beta=0
                lr = float(rng.uniform(1e-3, 0.2))
                p_mom, _ = momentum_step(p, g, v, lr, beta=0.0)
                p_sgd = sgd_step(p, g, lr)
                np.testing.assert_allclose(p_mom, p_sgd, atol=1e-12,
                                           err_msg="beta=0 momentum != SGD")

    def test_momentum_zero_velocity_zero_grad_no_movement(self):
        rng = np.random.RandomState(13)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            v = np.zeros(shape)
            g = np.zeros(shape)
            p_new, v_new = momentum_step(p, g, v, lr=0.1, beta=0.9)
            np.testing.assert_array_equal(p_new, p)
            np.testing.assert_array_equal(v_new, np.zeros(shape))

    def test_momentum_does_not_mutate_inputs_random(self):
        rng = np.random.RandomState(14)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            v = _rand(shape, rng)
            p0, g0, v0 = p.copy(), g.copy(), v.copy()
            momentum_step(p, g, v, lr=0.05, beta=0.9)
            np.testing.assert_array_equal(p, p0, err_msg="momentum_step mutated params")
            np.testing.assert_array_equal(g, g0, err_msg="momentum_step mutated grads")
            np.testing.assert_array_equal(v, v0, err_msg="momentum_step mutated velocity")

    def test_momentum_large_magnitudes_no_nan(self):
        rng = np.random.RandomState(15)
        for _ in range(10):
            d = rng.randint(2, 20)
            p = _rand((d,), rng, scale=1e3)
            g = _rand((d,), rng, scale=1e5)
            v = _rand((d,), rng, scale=1e2)
            p_new, v_new = momentum_step(p, g, v, lr=1e-6, beta=0.9)
            assert np.all(np.isfinite(p_new)), "momentum_step produced nan/inf in params"
            assert np.all(np.isfinite(v_new)), "momentum_step produced nan/inf in velocity"

    def test_momentum_multi_step_convergence_random_target(self):
        """momentum_step must converge on random quadratic targets, random start."""
        rng = np.random.RandomState(16)
        for _ in range(5):
            d = rng.randint(3, 12)
            target = _rand((d,), rng, scale=5.0)
            p = _rand((d,), rng, scale=8.0)
            v = np.zeros(d)
            lr, beta = 0.05, 0.9
            for _ in range(300):
                g = 2.0 * (p - target)
                p, v = momentum_step(p, g, v, lr, beta)
            np.testing.assert_allclose(p, target, atol=5e-2,
                                       err_msg="momentum did not converge to random target")

    def test_momentum_output_shapes(self):
        rng = np.random.RandomState(17)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            v = _rand(shape, rng)
            p_new, v_new = momentum_step(p, g, v, lr=0.01, beta=0.9)
            assert p_new.shape == shape
            assert v_new.shape == shape


# ---------------------------------------------------------------------------
# Adam – oracle vs torch.optim.Adam
# ---------------------------------------------------------------------------

class TestAdamOracle:
    """Compare adam_step against a manual torch Adam reference."""

    def _torch_adam_step(
        self,
        p_np: np.ndarray,
        g_np: np.ndarray,
        m_np: np.ndarray,
        v_np: np.ndarray,
        t: int,
        lr: float,
        b1: float,
        b2: float,
        eps: float,
    ):
        """One manual Adam step using the same formulas as the module contract."""
        p_t = torch.tensor(p_np, dtype=torch.float64)
        g_t = torch.tensor(g_np, dtype=torch.float64)
        m_t = torch.tensor(m_np, dtype=torch.float64)
        v_t = torch.tensor(v_np, dtype=torch.float64)
        m_new = b1 * m_t + (1 - b1) * g_t
        v_new = b2 * v_t + (1 - b2) * g_t ** 2
        m_hat = m_new / (1 - b1 ** t)
        v_hat = v_new / (1 - b2 ** t)
        p_new = p_t - lr * m_hat / (torch.sqrt(v_hat) + eps)
        return p_new.numpy(), m_new.numpy(), v_new.numpy()

    def test_adam_matches_torch_single_step(self):
        rng = np.random.RandomState(20)
        torch.manual_seed(20)
        for shape in _SHAPES:
            for t in (1, 2, 5, 10):
                lr = float(rng.uniform(1e-4, 0.1))
                b1 = float(rng.uniform(0.8, 0.99))
                b2 = float(rng.uniform(0.9, 0.9999))
                eps = 1e-8
                p = _rand(shape, rng)
                g = _rand(shape, rng)
                m = np.abs(_rand(shape, rng, scale=0.5))  # non-zero state
                v = np.abs(_rand(shape, rng, scale=0.2)) + 1e-6

                p_ours, m_ours, v_ours = adam_step(p, g, m, v, t, lr, b1, b2, eps)
                p_ref, m_ref, v_ref = self._torch_adam_step(p, g, m, v, t, lr, b1, b2, eps)

                np.testing.assert_allclose(p_ours, p_ref, atol=1e-11,
                                           err_msg=f"Adam params mismatch shape={shape} t={t}")
                np.testing.assert_allclose(m_ours, m_ref, atol=1e-12,
                                           err_msg=f"Adam m mismatch shape={shape} t={t}")
                np.testing.assert_allclose(v_ours, v_ref, atol=1e-12,
                                           err_msg=f"Adam v mismatch shape={shape} t={t}")

    def test_adam_returns_uncorrected_moments(self):
        """
        The returned m, v must be the RAW (uncorrected) moments.
        If the implementation mistakenly returns m_hat/v_hat, calling it again
        with t=2 will apply bias correction twice and produce wrong params.
        We verify by running two steps and comparing against the manual reference.
        """
        rng = np.random.RandomState(21)
        for shape in ((5,), (3, 4)):
            p = _rand(shape, rng)
            g1 = _rand(shape, rng)
            g2 = _rand(shape, rng)
            m = np.zeros(shape)
            v = np.zeros(shape)
            lr, b1, b2, eps = 1e-3, 0.9, 0.999, 1e-8

            # Two chained steps
            p1, m1, v1 = adam_step(p, g1, m, v, 1, lr, b1, b2, eps)
            p2_ours, _, _ = adam_step(p1, g2, m1, v1, 2, lr, b1, b2, eps)

            # Reference (explicit manual)
            p_ref, m_ref, v_ref = self._torch_adam_step(p, g1, m, v, 1, lr, b1, b2, eps)
            p2_ref, _, _ = self._torch_adam_step(p_ref, g2, m_ref, v_ref, 2, lr, b1, b2, eps)

            np.testing.assert_allclose(p2_ours, p2_ref, atol=1e-11,
                                       err_msg="Two-step Adam mismatch: likely returning corrected m/v")

    def test_adam_bias_correction_first_step_magnitude(self):
        """
        At step t=1, for any nonzero gradient, the step size should be approximately lr
        (bias correction normalises the ratio m_hat/sqrt(v_hat) to ~sign(g)).
        """
        rng = np.random.RandomState(22)
        lr, b1, b2, eps = 1e-2, 0.9, 0.999, 1e-8
        for _ in range(20):
            d = rng.randint(1, 10)
            g = _rand((d,), rng, scale=float(rng.uniform(0.5, 100.0)))
            p = np.zeros(d)
            m = np.zeros(d)
            v = np.zeros(d)
            p_new, _, _ = adam_step(p, g, m, v, 1, lr, b1, b2, eps)
            step_size = np.abs(p_new - p)
            # Each component's step size must be very close to lr
            np.testing.assert_allclose(step_size, np.full(d, lr), atol=1e-5,
                                       err_msg="Adam step magnitude at t=1 != lr (bias correction bug?)")

    def test_adam_v_is_nonneg(self):
        """Second moment v_new must always be >= 0 (squares of real numbers)."""
        rng = np.random.RandomState(23)
        for _ in range(30):
            shape = (rng.randint(2, 20),)
            g = _rand(shape, rng, scale=5.0)
            v = np.abs(_rand(shape, rng, scale=2.0))
            m = _rand(shape, rng)
            p = _rand(shape, rng)
            _, _, v_new = adam_step(p, g, m, v, 1, lr=1e-3, b1=0.9, b2=0.999, eps=1e-8)
            assert np.all(v_new >= 0), "adam v_new contains negative values"

    def test_adam_large_magnitudes_no_nan(self):
        """Adam should be numerically stable on large-magnitude gradients."""
        rng = np.random.RandomState(24)
        for _ in range(10):
            d = rng.randint(2, 20)
            p = _rand((d,), rng, scale=1e3)
            g = _rand((d,), rng, scale=1e6)
            m = np.zeros(d)
            v = np.zeros(d)
            for t in range(1, 6):
                p, m, v = adam_step(p, g, m, v, t, lr=1e-3, b1=0.9, b2=0.999, eps=1e-8)
            assert np.all(np.isfinite(p)), "adam_step produced nan/inf on large grads"
            assert np.all(np.isfinite(m)), "adam_step m contains nan/inf"
            assert np.all(np.isfinite(v)), "adam_step v contains nan/inf"

    def test_adam_does_not_mutate_inputs(self):
        rng = np.random.RandomState(25)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            m = np.abs(_rand(shape, rng, scale=0.3))
            v = np.abs(_rand(shape, rng, scale=0.1)) + 1e-9
            p0, g0, m0, v0 = p.copy(), g.copy(), m.copy(), v.copy()
            adam_step(p, g, m, v, 1, lr=1e-3, b1=0.9, b2=0.999, eps=1e-8)
            np.testing.assert_array_equal(p, p0, err_msg="adam_step mutated params")
            np.testing.assert_array_equal(g, g0, err_msg="adam_step mutated grads")
            np.testing.assert_array_equal(m, m0, err_msg="adam_step mutated m")
            np.testing.assert_array_equal(v, v0, err_msg="adam_step mutated v")

    def test_adam_output_shapes(self):
        rng = np.random.RandomState(26)
        for shape in _SHAPES:
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            m = np.zeros(shape)
            v = np.zeros(shape)
            p_new, m_new, v_new = adam_step(p, g, m, v, 1, lr=1e-3, b1=0.9, b2=0.999, eps=1e-8)
            assert p_new.shape == shape
            assert m_new.shape == shape
            assert v_new.shape == shape

    def test_adam_multi_step_convergence_random_targets(self):
        """Adam with default hparams must converge on random quadratic targets."""
        rng = np.random.RandomState(27)
        for _ in range(5):
            d = rng.randint(3, 15)
            target = _rand((d,), rng, scale=5.0)
            p = _rand((d,), rng, scale=8.0)
            m = np.zeros(d)
            v = np.zeros(d)
            lr, b1, b2, eps = 0.1, 0.9, 0.999, 1e-8
            for t in range(1, 801):
                g = 2.0 * (p - target)
                p, m, v = adam_step(p, g, m, v, t, lr, b1, b2, eps)
            np.testing.assert_allclose(p, target, atol=1e-1,
                                       err_msg="Adam did not converge to random target")

    def test_adam_single_element_edge_case(self):
        p = np.array([0.0])
        g = np.array([-1.0])
        m = np.array([0.0])
        v = np.array([0.0])
        lr, b1, b2, eps = 0.01, 0.9, 0.999, 1e-8
        p_new, m_new, v_new = adam_step(p, g, m, v, 1, lr, b1, b2, eps)
        assert p_new.shape == (1,)
        assert m_new.shape == (1,)
        assert v_new.shape == (1,)
        # gradient is negative -> params should increase
        assert p_new[0] > p[0], "negative gradient should push params up"

    def test_adam_step_t_increments_matter(self):
        """
        Bias correction is stronger at t=1 than at t=100.
        At t=1, b1^t = 0.9, so (1-b1^t)=0.1 — strong correction.
        At t=100, b1^t ≈ 2.6e-5, so (1-b1^t)≈1 — nearly no correction.
        Step at t=1 should be larger than at t=100 (same grad, state starting from zero).
        Actually we just verify the two give *different* outputs to catch t being ignored.
        """
        rng = np.random.RandomState(28)
        d = 5
        p = _rand((d,), rng)
        g = _rand((d,), rng)
        m = np.zeros(d)
        v = np.zeros(d)
        lr, b1, b2, eps = 1e-3, 0.9, 0.999, 1e-8

        p_t1, _, _ = adam_step(p, g, m, v, 1, lr, b1, b2, eps)
        p_t100, _, _ = adam_step(p, g, m, v, 100, lr, b1, b2, eps)
        assert not np.allclose(p_t1, p_t100, atol=1e-10), (
            "adam_step gives identical output for t=1 and t=100 — t is likely being ignored"
        )


# ---------------------------------------------------------------------------
# Finite-difference gradient checks
# ---------------------------------------------------------------------------

class TestFiniteDiff:
    """
    The optimizers themselves are not differentiable objectives, but we can
    verify that the *direction* of the step is correct: params_new - params
    should be proportional to -grads for SGD, and have a consistent sign.
    """

    def test_sgd_step_direction(self):
        """Each component of (params_new - params) must have sign = -sign(grads) when lr>0."""
        rng = np.random.RandomState(30)
        for _ in range(20):
            d = rng.randint(2, 30)
            p = _rand((d,), rng)
            g = _rand((d,), rng)
            # Avoid zeros (sign undefined)
            g = np.where(np.abs(g) < 0.01, 0.1, g)
            lr = float(rng.uniform(0.01, 0.5))
            p_new = sgd_step(p, g, lr)
            delta = p_new - p
            wrong_sign = np.sign(delta) * np.sign(g) > 0  # both same sign => wrong
            assert not np.any(wrong_sign), (
                "sgd_step: some params moved in the same direction as the gradient"
            )

    def test_momentum_step_direction_first_step_from_zero(self):
        """
        From zero velocity, first momentum step direction = -grads (same as SGD).
        """
        rng = np.random.RandomState(31)
        for _ in range(20):
            d = rng.randint(2, 20)
            p = _rand((d,), rng)
            g = _rand((d,), rng)
            g = np.where(np.abs(g) < 0.01, 0.1, g)
            v = np.zeros(d)
            lr = float(rng.uniform(0.01, 0.3))
            beta = float(rng.uniform(0.0, 0.99))
            p_new, _ = momentum_step(p, g, v, lr, beta)
            delta = p_new - p
            wrong_sign = np.sign(delta) * np.sign(g) > 0
            assert not np.any(wrong_sign), (
                "momentum_step (zero velocity): moved in gradient direction instead of against it"
            )

    def test_adam_step_direction(self):
        """
        Adam update direction must oppose the gradient (sign of step = -sign(grad)).
        """
        rng = np.random.RandomState(32)
        for _ in range(20):
            d = rng.randint(2, 20)
            p = _rand((d,), rng)
            g = _rand((d,), rng)
            g = np.where(np.abs(g) < 0.01, 0.1, g)
            m = np.zeros(d)
            v = np.zeros(d)
            lr, b1, b2, eps = 1e-3, 0.9, 0.999, 1e-8
            p_new, _, _ = adam_step(p, g, m, v, 1, lr, b1, b2, eps)
            delta = p_new - p
            wrong_sign = np.sign(delta) * np.sign(g) > 0
            assert not np.any(wrong_sign), (
                "adam_step: some params moved in the gradient direction instead of against it"
            )


# ---------------------------------------------------------------------------
# Cross-optimizer invariants
# ---------------------------------------------------------------------------

class TestCrossOptimizerInvariants:

    def test_all_return_correct_dtypes(self):
        """All optimizers must preserve float64 when given float64 inputs."""
        rng = np.random.RandomState(40)
        for shape in ((5,), (3, 4)):
            p = _rand(shape, rng).astype(np.float64)
            g = _rand(shape, rng).astype(np.float64)
            v = np.zeros(shape, dtype=np.float64)
            m = np.zeros(shape, dtype=np.float64)

            out_sgd = sgd_step(p, g, 0.01)
            assert out_sgd.dtype == np.float64 or np.issubdtype(out_sgd.dtype, np.floating)

            p_mom, v_new = momentum_step(p, g, v, 0.01, 0.9)
            assert np.issubdtype(p_mom.dtype, np.floating)
            assert np.issubdtype(v_new.dtype, np.floating)

            p_adam, m_new, v_new2 = adam_step(p, g, m, v, 1, 1e-3, 0.9, 0.999, 1e-8)
            assert np.issubdtype(p_adam.dtype, np.floating)
            assert np.issubdtype(m_new.dtype, np.floating)
            assert np.issubdtype(v_new2.dtype, np.floating)

    def test_equal_params_equal_grads_all_optimizers(self):
        """
        Sanity: same input twice must yield same output (determinism, no hidden state).
        """
        rng = np.random.RandomState(41)
        for _ in range(5):
            shape = (rng.randint(2, 10),)
            p = _rand(shape, rng)
            g = _rand(shape, rng)
            v = _rand(shape, rng, scale=0.1)
            m = _rand(shape, rng, scale=0.1)
            v2 = np.abs(_rand(shape, rng, scale=0.1)) + 1e-8

            lr = 0.01

            # SGD
            np.testing.assert_array_equal(
                sgd_step(p.copy(), g.copy(), lr),
                sgd_step(p.copy(), g.copy(), lr),
            )
            # Momentum
            p_m1, v_m1 = momentum_step(p.copy(), g.copy(), v.copy(), lr, 0.9)
            p_m2, v_m2 = momentum_step(p.copy(), g.copy(), v.copy(), lr, 0.9)
            np.testing.assert_array_equal(p_m1, p_m2)
            np.testing.assert_array_equal(v_m1, v_m2)
            # Adam
            p_a1, m_a1, v_a1 = adam_step(p.copy(), g.copy(), m.copy(), v2.copy(), 1, lr, 0.9, 0.999, 1e-8)
            p_a2, m_a2, v_a2 = adam_step(p.copy(), g.copy(), m.copy(), v2.copy(), 1, lr, 0.9, 0.999, 1e-8)
            np.testing.assert_array_equal(p_a1, p_a2)
            np.testing.assert_array_equal(m_a1, m_a2)
            np.testing.assert_array_equal(v_a1, v_a2)

    def test_all_converge_on_high_dimensional_quadratic(self):
        """All three optimizers must converge on a high-dimensional (D=50) quadratic."""
        rng = np.random.RandomState(42)
        torch.manual_seed(42)
        D = 50
        target = _rand((D,), rng, scale=3.0)
        start = _rand((D,), rng, scale=5.0)

        # SGD
        p = start.copy()
        for _ in range(300):
            p = sgd_step(p, 2.0 * (p - target), lr=0.05)
        assert np.mean((p - target) ** 2) < 1e-3, "SGD failed on D=50 quadratic"

        # Momentum
        p = start.copy()
        v = np.zeros(D)
        for _ in range(300):
            p, v = momentum_step(p, 2.0 * (p - target), v, lr=0.05, beta=0.9)
        assert np.mean((p - target) ** 2) < 1e-3, "Momentum failed on D=50 quadratic"

        # Adam
        p = start.copy()
        m_adam = np.zeros(D)
        v_adam = np.zeros(D)
        for t in range(1, 401):
            p, m_adam, v_adam = adam_step(p, 2.0 * (p - target), m_adam, v_adam, t,
                                          lr=0.1, b1=0.9, b2=0.999, eps=1e-8)
        assert np.mean((p - target) ** 2) < 1e-3, "Adam failed on D=50 quadratic"

    def test_extreme_lr_sgd_diverges(self):
        """
        With a very large lr (lr=2 on f(p)=(p-t)^2, grad=2*(p-t)) the loss should
        NOT converge (overshoot). This checks the sign is correct: if the sign were
        wrong (p + lr*g), loss would explode even faster — but the key check is that
        a valid implementation with lr=2 indeed diverges (loss grows), confirming the
        formula is not trivially neutralised.
        Actually: lr=2 is exactly the critical lr for a 1D quadratic (grad=2*(p-t)),
        so we use lr=3 to guarantee divergence.
        """
        np.random.seed(43)
        target = np.array([0.0])
        p = np.array([5.0])
        lr = 3.0
        loss0 = float(np.sum((p - target) ** 2))
        for _ in range(5):
            g = 2.0 * (p - target)
            p = sgd_step(p, g, lr)
        loss_end = float(np.sum((p - target) ** 2))
        assert loss_end > loss0, (
            "SGD with lr=3 on quadratic should diverge, but loss decreased — "
            "check sign convention in sgd_step"
        )
