# Randomised / property-based tests for Module 03 — Gradients.
#
# Design goals
# ─────────────
# • Each test uses a *different* deterministic seed so failures are independent.
# • PyTorch autograd is used as an oracle wherever there is a direct torch
#   equivalent (grad_sum_squares, grad_of_affine).
# • Finite-difference self-consistency checks verify numerical_gradient itself
#   on functions torch.autograd does not help with directly.
# • Invariants (shape preservation, no-mutation, zero vector, large values,
#   negative values, 2-D shapes) are each covered by dedicated tests.
# • All module calls are INSIDE test functions — the file imports/collects
#   cleanly against a stub that raises NotImplementedError.

import numpy as np
import torch
import pytest

from gradients import numerical_gradient, grad_sum_squares, grad_of_affine


# ═══════════════════════════════════════════════════════════════════════════
# Helpers
# ═══════════════════════════════════════════════════════════════════════════

def _torch_grad_sum_squares(x_np: np.ndarray) -> np.ndarray:
    """Reference: ∂/∂x sum(x²) via torch autograd."""
    x_t = torch.tensor(x_np, dtype=torch.float64, requires_grad=True)
    loss = (x_t ** 2).sum()
    loss.backward()
    return x_t.grad.numpy()


def _torch_grad_of_affine(x_np: np.ndarray, w_np: np.ndarray) -> np.ndarray:
    """Reference: ∂/∂x dot(w,x) via torch autograd."""
    x_t = torch.tensor(x_np, dtype=torch.float64, requires_grad=True)
    w_t = torch.tensor(w_np, dtype=torch.float64)
    loss = torch.dot(w_t, x_t)
    loss.backward()
    return x_t.grad.numpy()


# ═══════════════════════════════════════════════════════════════════════════
# numerical_gradient — invariants & edge cases
# ═══════════════════════════════════════════════════════════════════════════

def test_numgrad_shape_preserved_multiple_sizes():
    """Output shape equals input shape for several different sizes."""
    np.random.seed(10)
    for d in (1, 3, 8, 20):
        x = np.random.randn(d)
        g = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
        assert g.shape == (d,), f"shape mismatch for d={d}"


def test_numgrad_does_not_mutate_multiple_seeds():
    """x must be identical before and after the call, tried with 5 seeds."""
    for seed in range(20, 25):
        np.random.seed(seed)
        x = np.random.randn(7)
        x_copy = x.copy()
        numerical_gradient(lambda v: float(np.sum(v ** 3)), x)
        assert np.array_equal(x, x_copy), f"x was mutated (seed={seed})"


def test_numgrad_cubic_multiple_points():
    """f(x) = sum(x³)  →  grad = 3x²; verified at 6 random points."""
    np.random.seed(30)
    for _ in range(6):
        x = np.random.randn(5)
        g = numerical_gradient(lambda v: float(np.sum(v ** 3)), x)
        expected = 3.0 * x ** 2
        assert np.allclose(g, expected, atol=1e-4), (
            f"cubic grad wrong\n  got:      {g}\n  expected: {expected}"
        )


def test_numgrad_2d_shape_cos_function():
    """Central difference works on 2-D arrays; f = sum(cos(x))  →  grad = -sin(x)."""
    np.random.seed(40)
    X = np.random.randn(4, 3)
    g = numerical_gradient(lambda V: float(np.sum(np.cos(V))), X)
    assert g.shape == (4, 3)
    assert np.allclose(g, -np.sin(X), atol=1e-4)


def test_numgrad_zero_vector():
    """Gradient of sum(x²) at x=0 must be exactly 0 (all zeros in, all zeros out)."""
    x = np.zeros(5)
    g = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
    assert np.allclose(g, np.zeros(5), atol=1e-8)


def test_numgrad_single_element():
    """1-element array: gradient must still be a 1-element array with correct value."""
    np.random.seed(50)
    x = np.array([np.random.randn()])
    g = numerical_gradient(lambda v: float(v[0] ** 2), x)
    assert g.shape == (1,)
    assert np.allclose(g, 2.0 * x, atol=1e-5)


def test_numgrad_large_magnitude_stability():
    """Large-magnitude inputs (x ~ 1e3); central difference should still be accurate."""
    np.random.seed(60)
    x = np.random.randn(5) * 1e3
    g = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
    assert np.allclose(g, 2.0 * x, atol=1e-1)  # looser tol for large scale


def test_numgrad_all_negative_inputs():
    """Negative inputs: gradient of sum(x²) is 2x, which should also be negative."""
    np.random.seed(70)
    x = -np.abs(np.random.randn(6))  # guaranteed all-negative
    g = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
    assert np.allclose(g, 2.0 * x, atol=1e-5)
    assert np.all(g <= 0), "gradient of sum(x²) at x<0 should be ≤ 0"


def test_numgrad_custom_eps():
    """Smaller eps=1e-6 should still produce an accurate gradient."""
    np.random.seed(80)
    x = np.random.randn(4)
    g = numerical_gradient(lambda v: float(np.sum(v ** 2)), x, eps=1e-6)
    assert np.allclose(g, 2.0 * x, atol=1e-5)


def test_numgrad_against_finite_diff_loop():
    """
    Self-consistency: compute each component manually with the same formula and
    compare — this detects 'divide by eps instead of 2*eps' class of bugs.
    """
    np.random.seed(90)
    x = np.random.randn(6)
    eps = 1e-5
    f = lambda v: float(np.dot(v, v))  # sum(x²)
    g = numerical_gradient(f, x, eps=eps)

    # Build reference by hand with the exact central-difference formula
    expected = np.zeros_like(x, dtype=float)
    for i in range(len(x)):
        xp, xm = x.copy(), x.copy()
        xp[i] += eps
        xm[i] -= eps
        expected[i] = (f(xp) - f(xm)) / (2 * eps)

    assert np.allclose(g, expected, atol=1e-12), (
        "numerical_gradient disagrees with manually computed central difference"
    )


# ═══════════════════════════════════════════════════════════════════════════
# grad_sum_squares — oracle vs torch autograd + invariants
# ═══════════════════════════════════════════════════════════════════════════

def test_grad_sum_squares_vs_torch_multiple_sizes():
    """Oracle: compare grad_sum_squares(x) against torch autograd for 4 sizes."""
    torch.manual_seed(100)
    np.random.seed(100)
    for d in (1, 5, 10, 50):
        x = np.random.randn(d)
        got = grad_sum_squares(x)
        ref = _torch_grad_sum_squares(x)
        assert np.allclose(got, ref, atol=1e-6), (
            f"grad_sum_squares disagrees with torch for d={d}"
        )


def test_grad_sum_squares_vs_torch_random_loop():
    """Oracle: 10 random vectors, all must match torch autograd."""
    np.random.seed(110)
    for _ in range(10):
        x = np.random.randn(np.random.randint(2, 15))
        got = grad_sum_squares(x)
        ref = _torch_grad_sum_squares(x)
        assert np.allclose(got, ref, atol=1e-6)


def test_grad_sum_squares_is_exactly_2x():
    """Direct algebraic property: result must equal 2.0 * x element-wise."""
    np.random.seed(120)
    for _ in range(8):
        x = np.random.randn(6)
        assert np.allclose(grad_sum_squares(x), 2.0 * x, atol=1e-12)


def test_grad_sum_squares_dtype_is_float():
    """Result must be float even if input is integer."""
    x = np.array([1, -2, 3], dtype=int)
    g = grad_sum_squares(x)
    assert np.issubdtype(g.dtype, np.floating), f"expected float dtype, got {g.dtype}"


def test_grad_sum_squares_does_not_mutate():
    """grad_sum_squares must not modify its input."""
    np.random.seed(130)
    x = np.random.randn(5)
    x_before = x.copy()
    grad_sum_squares(x)
    assert np.array_equal(x, x_before)


def test_grad_sum_squares_zero_input():
    """Gradient at all-zeros must be all-zeros."""
    x = np.zeros(7)
    assert np.allclose(grad_sum_squares(x), np.zeros(7), atol=1e-12)


def test_grad_sum_squares_negative_inputs():
    """At all-negative x, gradient 2x must also be negative."""
    np.random.seed(140)
    x = -np.abs(np.random.randn(5)) - 0.1  # strictly negative
    g = grad_sum_squares(x)
    assert np.all(g < 0), "grad_sum_squares(x<0) should be all negative"


def test_grad_sum_squares_matches_numerical_gradient_loop():
    """Gradient checking: analytic must agree with numerical for 6 random inputs."""
    np.random.seed(150)
    for _ in range(6):
        x = np.random.randn(8)
        analytic = grad_sum_squares(x)
        numeric = numerical_gradient(lambda v: float(np.sum(v ** 2)), x)
        assert np.allclose(analytic, numeric, atol=1e-5), (
            "grad_sum_squares fails gradient check"
        )


# ═══════════════════════════════════════════════════════════════════════════
# grad_of_affine — oracle vs torch autograd + invariants
# ═══════════════════════════════════════════════════════════════════════════

def test_grad_of_affine_vs_torch_multiple_sizes():
    """Oracle: compare grad_of_affine(x, w) against torch autograd for 4 sizes."""
    torch.manual_seed(200)
    np.random.seed(200)
    for d in (1, 4, 10, 30):
        x = np.random.randn(d)
        w = np.random.randn(d)
        got = grad_of_affine(x, w)
        ref = _torch_grad_of_affine(x, w)
        assert np.allclose(got, ref, atol=1e-6), (
            f"grad_of_affine disagrees with torch for d={d}"
        )


def test_grad_of_affine_vs_torch_random_loop():
    """Oracle: 10 random (x, w) pairs, all must match torch autograd."""
    np.random.seed(210)
    for _ in range(10):
        d = np.random.randint(2, 20)
        x = np.random.randn(d)
        w = np.random.randn(d)
        got = grad_of_affine(x, w)
        ref = _torch_grad_of_affine(x, w)
        assert np.allclose(got, ref, atol=1e-6)


def test_grad_of_affine_equals_w_exactly():
    """Algebraic: result must equal w (gradient of dot(w,x) w.r.t. x is w)."""
    np.random.seed(220)
    for _ in range(8):
        d = np.random.randint(2, 12)
        x = np.random.randn(d)
        w = np.random.randn(d)
        assert np.allclose(grad_of_affine(x, w), w, atol=1e-12)


def test_grad_of_affine_is_independent_of_x():
    """Gradient must not change when x changes (f is linear in x, so grad = w always)."""
    np.random.seed(230)
    w = np.random.randn(6)
    for _ in range(5):
        x = np.random.randn(6) * np.random.uniform(0.1, 100)
        assert np.allclose(grad_of_affine(x, w), w, atol=1e-12), (
            "grad_of_affine changed when only x changed — violates linearity"
        )


def test_grad_of_affine_returns_copy_not_same_object():
    """Returned array must not be the same object as w (mutation safety)."""
    np.random.seed(240)
    w = np.random.randn(4)
    x = np.random.randn(4)
    g = grad_of_affine(x, w)
    assert g is not w, "grad_of_affine must return a copy, not the same object as w"
    # Mutate g — w should be unchanged
    g[:] = 0.0
    assert not np.allclose(w, 0.0), "w was mutated through the returned gradient"


def test_grad_of_affine_dtype_is_float():
    """Result must be float even if inputs are integer."""
    x = np.array([1, 2, 3], dtype=int)
    w = np.array([4, 5, 6], dtype=int)
    g = grad_of_affine(x, w)
    assert np.issubdtype(g.dtype, np.floating), f"expected float dtype, got {g.dtype}"


def test_grad_of_affine_zero_weight():
    """w = 0 → gradient must be all zeros regardless of x."""
    np.random.seed(250)
    x = np.random.randn(5)
    w = np.zeros(5)
    g = grad_of_affine(x, w)
    assert np.allclose(g, np.zeros(5), atol=1e-12)


def test_grad_of_affine_matches_numerical_gradient_loop():
    """Gradient checking: analytic must agree with numerical for 6 random (x,w) pairs."""
    np.random.seed(260)
    for _ in range(6):
        d = np.random.randint(2, 12)
        x = np.random.randn(d)
        w = np.random.randn(d)
        analytic = grad_of_affine(x, w)
        numeric = numerical_gradient(lambda v: float(np.dot(w, v)), x)
        assert np.allclose(analytic, numeric, atol=1e-5), (
            f"grad_of_affine fails gradient check for d={d}"
        )


def test_grad_of_affine_large_magnitude():
    """Large-magnitude w: gradient should still equal w without overflow."""
    np.random.seed(270)
    x = np.random.randn(5)
    w = np.random.randn(5) * 1e6
    g = grad_of_affine(x, w)
    assert np.allclose(g, w, atol=1e-6)


# ═══════════════════════════════════════════════════════════════════════════
# Cross-function: gradient descent convergence on sum(x²)
# ═══════════════════════════════════════════════════════════════════════════

def test_gradient_descent_converges_close_to_zero():
    """
    Running gradient descent with grad_sum_squares for 200 steps should drive
    ||x||² to near-zero (< 1e-6), not just reduce it by half.
    """
    np.random.seed(300)
    x = np.random.randn(10)
    lr = 0.1
    for _ in range(200):
        x = x - lr * grad_sum_squares(x)
    assert np.sum(x ** 2) < 1e-6, (
        f"Gradient descent did not converge; residual = {np.sum(x**2):.6e}"
    )


def test_gradient_descent_multiple_starting_points():
    """
    From 5 different random starting points, grad descent must always reduce
    loss by at least 10000x in 150 steps.
    """
    np.random.seed(310)
    for i in range(5):
        x = np.random.randn(8) * 5.0
        loss0 = float(np.sum(x ** 2))
        lr = 0.1
        for _ in range(150):
            x = x - lr * grad_sum_squares(x)
        loss1 = float(np.sum(x ** 2))
        assert loss1 < loss0 / 1e4, (
            f"Gradient descent failed starting point {i}: "
            f"loss0={loss0:.4f}, loss1={loss1:.6e}"
        )
