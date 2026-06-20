# Randomized / property-based tests for module 07 — Backpropagation.
#
# Strategy:
#   1. PyTorch oracle: compare our numpy implementations against torch equivalents.
#   2. Finite-difference gradient checks: verify backward2 gradients on varied random inputs.
#   3. Invariants: shape preservation, softmax row sums, relu non-negativity, etc.
#   4. Edge cases: zeros, negatives, single example, large magnitudes, near-zero inputs.
#
# NOTE: all calls to the module happen INSIDE test functions — this file must be
# importable (and collect cleanly) against the stub branch where every function
# raises NotImplementedError.

import numpy as np
import pytest
import torch
import torch.nn.functional as F

from backprop import (
    relu,
    softmax,
    cross_entropy,
    forward2,
    backward2,
    numerical_gradient,
)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _make_net(seed, N=5, D=4, H=6, C=3):
    """Return (x, y, W1, b1, W2, b2) with a seeded RNG."""
    rng = np.random.RandomState(seed)
    x = rng.randn(N, D)
    y = rng.randint(0, C, size=N)
    W1 = rng.randn(D, H) * 0.5
    b1 = rng.randn(H) * 0.1
    W2 = rng.randn(H, C) * 0.5
    b2 = rng.randn(C) * 0.1
    return x, y, W1, b1, W2, b2


# ---------------------------------------------------------------------------
# RELU — oracle vs torch, invariants, edge cases
# ---------------------------------------------------------------------------

class TestReluOracle:
    @pytest.mark.parametrize("seed,shape", [
        (10, (3, 5)),
        (11, (1, 8)),
        (12, (7, 7)),
        (13, (20, 4)),
        (14, (1, 1)),
    ])
    def test_relu_matches_torch(self, seed, shape):
        np.random.seed(seed)
        z = np.random.randn(*shape)
        z_t = torch.tensor(z, dtype=torch.float64)
        expected = torch.relu(z_t).numpy()
        result = relu(z)
        assert np.allclose(result, expected, atol=1e-7), (
            f"relu mismatch for seed={seed}, shape={shape}"
        )

    def test_relu_all_negative(self):
        """All-negative input -> all zeros."""
        np.random.seed(20)
        z = -np.abs(np.random.randn(4, 5)) - 0.01
        result = relu(z)
        assert np.all(result == 0.0), "relu of all-negative input must be all zeros"

    def test_relu_all_positive(self):
        """All-positive input -> unchanged."""
        np.random.seed(21)
        z = np.abs(np.random.randn(4, 5)) + 0.01
        result = relu(z)
        assert np.allclose(result, z), "relu of all-positive input must equal input"

    def test_relu_zeros(self):
        """relu(0) == 0."""
        z = np.zeros((3, 3))
        result = relu(z)
        assert np.all(result == 0.0)

    def test_relu_nonnegative_invariant(self):
        """Output is always >= 0 for any input."""
        np.random.seed(22)
        z = np.random.randn(10, 10) * 100
        result = relu(z)
        assert np.all(result >= 0.0)

    def test_relu_shape_preserved(self):
        """Shape must be identical to input shape."""
        for seed, shape in [(30, (1,)), (31, (3, 4, 5)), (32, (8, 2))]:
            np.random.seed(seed)
            z = np.random.randn(*shape)
            assert relu(z).shape == z.shape

    def test_relu_large_magnitudes(self):
        """No overflow or unexpected behavior for very large inputs."""
        z = np.array([[1e10, -1e10, 0.0]])
        result = relu(z)
        assert np.all(np.isfinite(result))
        assert np.allclose(result, np.array([[1e10, 0.0, 0.0]]))


# ---------------------------------------------------------------------------
# SOFTMAX — oracle vs torch, invariants, edge cases
# ---------------------------------------------------------------------------

class TestSoftmaxOracle:
    @pytest.mark.parametrize("seed,shape", [
        (40, (4, 3)),
        (41, (1, 5)),
        (42, (10, 10)),
        (43, (2, 2)),
        (44, (8, 4)),
    ])
    def test_softmax_matches_torch(self, seed, shape):
        np.random.seed(seed)
        torch.manual_seed(seed)
        z = np.random.randn(*shape)
        z_t = torch.tensor(z, dtype=torch.float64)
        expected = torch.softmax(z_t, dim=-1).numpy()
        result = softmax(z)
        assert np.allclose(result, expected, atol=1e-7), (
            f"softmax mismatch for seed={seed}, shape={shape}"
        )

    def test_softmax_rows_sum_to_one_random(self):
        """For any random input, each row of softmax output sums to 1."""
        np.random.seed(50)
        for _ in range(8):
            N = np.random.randint(1, 10)
            C = np.random.randint(2, 8)
            z = np.random.randn(N, C) * 5
            p = softmax(z)
            assert np.allclose(p.sum(axis=1), np.ones(N), atol=1e-9), (
                f"softmax rows do not sum to 1 for shape ({N},{C})"
            )

    def test_softmax_probabilities_in_0_1(self):
        """All outputs in [0, 1] for any input."""
        np.random.seed(51)
        z = np.random.randn(6, 5)
        p = softmax(z)
        assert np.all(p >= 0.0) and np.all(p <= 1.0)

    def test_softmax_shift_invariance_random(self):
        """softmax(z) == softmax(z + shift) for any per-row constant shift."""
        np.random.seed(52)
        z = np.random.randn(5, 4)
        shift = np.random.randn(5, 1) * 50
        assert np.allclose(softmax(z), softmax(z + shift), atol=1e-9)

    def test_softmax_numerically_stable_large_positive(self):
        """Huge positive logits must not produce inf/nan."""
        z = np.array([[1e6, 1e6 + 1, 1e6 + 2]])
        p = softmax(z)
        assert np.all(np.isfinite(p)), "softmax produced non-finite values for large input"
        assert np.allclose(p.sum(axis=1), 1.0, atol=1e-9)

    def test_softmax_numerically_stable_large_negative(self):
        """Large negative logits must not produce nan (exp underflow -> 0, not nan)."""
        z = np.array([[-1e6, -1e6 + 1, -1e6 + 2]])
        p = softmax(z)
        assert np.all(np.isfinite(p))
        assert np.allclose(p.sum(axis=1), 1.0, atol=1e-9)

    def test_softmax_single_row(self):
        """Batch of size 1 works correctly."""
        np.random.seed(53)
        z = np.random.randn(1, 5)
        p = softmax(z)
        assert p.shape == (1, 5)
        assert np.allclose(p.sum(), 1.0, atol=1e-9)

    def test_softmax_uniform_input_gives_uniform_output(self):
        """If all logits in a row are equal, softmax output is uniform."""
        z = np.zeros((3, 4))
        p = softmax(z)
        assert np.allclose(p, 0.25 * np.ones((3, 4)), atol=1e-9)

    def test_softmax_shape_preserved(self):
        for seed, N, C in [(60, 3, 4), (61, 1, 2), (62, 7, 10)]:
            np.random.seed(seed)
            z = np.random.randn(N, C)
            assert softmax(z).shape == (N, C)


# ---------------------------------------------------------------------------
# CROSS-ENTROPY — oracle vs torch, invariants, edge cases
# ---------------------------------------------------------------------------

class TestCrossEntropyOracle:
    @pytest.mark.parametrize("seed,N,C", [
        (70, 5, 3),
        (71, 1, 4),
        (72, 10, 5),
        (73, 8, 2),
        (74, 3, 7),
    ])
    def test_cross_entropy_matches_torch(self, seed, N, C):
        """cross_entropy(softmax(z), y) should match torch NLLLoss applied to torch.log_softmax(z)."""
        np.random.seed(seed)
        torch.manual_seed(seed)
        z = np.random.randn(N, C)
        y = np.random.randint(0, C, size=N)

        # Our implementation: first softmax, then cross_entropy
        p_np = softmax(z)
        our_loss = cross_entropy(p_np, y)

        # Torch: use cross_entropy which fuses log_softmax + nll_loss
        z_t = torch.tensor(z, dtype=torch.float64)
        y_t = torch.tensor(y, dtype=torch.long)
        torch_loss = F.cross_entropy(z_t, y_t, reduction="mean").item()

        assert np.isclose(our_loss, torch_loss, atol=1e-5), (
            f"cross_entropy mismatch: ours={our_loss:.7f}, torch={torch_loss:.7f}, "
            f"seed={seed}, N={N}, C={C}"
        )

    def test_cross_entropy_is_nonnegative(self):
        """Cross-entropy must be >= 0 for any valid probability distribution."""
        np.random.seed(80)
        for _ in range(5):
            N, C = np.random.randint(1, 8), np.random.randint(2, 6)
            z = np.random.randn(N, C)
            p = softmax(z)
            y = np.random.randint(0, C, size=N)
            loss = cross_entropy(p, y)
            assert loss >= 0.0, f"cross_entropy returned negative value {loss}"

    def test_cross_entropy_perfect_prediction_near_zero(self):
        """Confident correct prediction -> loss close to 0."""
        N, C = 4, 5
        p = np.full((N, C), 1e-8)
        y = np.array([0, 1, 2, 3])
        p[np.arange(N), y] = 1.0 - (C - 1) * 1e-8
        p = p / p.sum(axis=1, keepdims=True)
        loss = cross_entropy(p, y)
        assert loss < 0.01, f"expected near-zero loss for correct predictions, got {loss}"

    def test_cross_entropy_returns_scalar(self):
        np.random.seed(81)
        N, C = 6, 3
        p = softmax(np.random.randn(N, C))
        y = np.random.randint(0, C, size=N)
        loss = cross_entropy(p, y)
        assert np.ndim(loss) == 0 or np.isscalar(loss)

    def test_cross_entropy_single_example(self):
        """Works with a single example (N=1)."""
        np.random.seed(82)
        p = softmax(np.random.randn(1, 4))
        y = np.array([2])
        loss = cross_entropy(p, y)
        assert np.isfinite(loss)
        expected = -np.log(p[0, 2] + 1e-12)
        assert np.isclose(loss, expected, atol=1e-6)

    def test_cross_entropy_uses_correct_class_only(self):
        """Changing probabilities of wrong classes must not affect the loss."""
        np.random.seed(83)
        N, C = 3, 4
        z = np.random.randn(N, C)
        p_base = softmax(z)
        y = np.array([0, 1, 2])
        loss_base = cross_entropy(p_base, y)

        # Redistribute probability among wrong classes; keep correct class prob fixed
        p_perturbed = p_base.copy()
        for i in range(N):
            wrong_idx = [j for j in range(C) if j != y[i]]
            noise = np.random.randn(len(wrong_idx))
            noise -= noise.mean()
            noise *= 0.01
            p_perturbed[i, wrong_idx] += noise
            # Renormalize while keeping correct class prob fixed
            correct_prob = p_perturbed[i, y[i]]
            remaining = 1.0 - correct_prob
            wrong_probs = p_perturbed[i, wrong_idx]
            wrong_probs = np.maximum(wrong_probs, 1e-9)
            wrong_probs = wrong_probs / wrong_probs.sum() * remaining
            p_perturbed[i, wrong_idx] = wrong_probs

        loss_perturbed = cross_entropy(p_perturbed, y)
        assert np.isclose(loss_base, loss_perturbed, atol=1e-7), (
            "cross_entropy changed when only wrong-class probs changed"
        )


# ---------------------------------------------------------------------------
# BACKWARD2 — PyTorch autograd oracle, varied shapes and seeds
# ---------------------------------------------------------------------------

class TestBackward2Oracle:
    @pytest.mark.parametrize("seed,N,D,H,C", [
        (100, 5,  4,  6,  3),
        (101, 8,  3,  5,  4),
        (102, 1,  6,  8,  2),   # single example
        (103, 12, 7, 10,  5),
        (104, 3,  2,  4,  3),
    ])
    def test_backward2_matches_torch_autograd(self, seed, N, D, H, C):
        """Analytical gradients from backward2 must match PyTorch autograd."""
        np.random.seed(seed)
        torch.manual_seed(seed)

        x_np = np.random.randn(N, D)
        y_np = np.random.randint(0, C, size=N)
        W1_np = np.random.randn(D, H) * 0.5
        b1_np = np.random.randn(H) * 0.1
        W2_np = np.random.randn(H, C) * 0.5
        b2_np = np.random.randn(C) * 0.1

        # Our analytical gradients
        grads = backward2(x_np, y_np, W1_np, b1_np, W2_np, b2_np)

        # PyTorch reference: same forward graph, same cross_entropy (mean reduction)
        x_t  = torch.tensor(x_np,  dtype=torch.float64)
        y_t  = torch.tensor(y_np,  dtype=torch.long)
        W1_t = torch.tensor(W1_np, dtype=torch.float64, requires_grad=True)
        b1_t = torch.tensor(b1_np, dtype=torch.float64, requires_grad=True)
        W2_t = torch.tensor(W2_np, dtype=torch.float64, requires_grad=True)
        b2_t = torch.tensor(b2_np, dtype=torch.float64, requires_grad=True)

        z1_t = x_t @ W1_t + b1_t
        a1_t = torch.relu(z1_t)
        z2_t = a1_t @ W2_t + b2_t
        loss_t = F.cross_entropy(z2_t, y_t, reduction="mean")
        loss_t.backward()

        assert np.allclose(grads["dW1"], W1_t.grad.numpy(), atol=1e-5), \
            f"dW1 mismatch at seed={seed}"
        assert np.allclose(grads["db1"], b1_t.grad.numpy(), atol=1e-5), \
            f"db1 mismatch at seed={seed}"
        assert np.allclose(grads["dW2"], W2_t.grad.numpy(), atol=1e-5), \
            f"dW2 mismatch at seed={seed}"
        assert np.allclose(grads["db2"], b2_t.grad.numpy(), atol=1e-5), \
            f"db2 mismatch at seed={seed}"

    def test_backward2_gradient_shapes_match_params(self):
        """Gradient shapes must exactly match parameter shapes (sanity for every key)."""
        np.random.seed(105)
        for seed in range(5):
            x, y, W1, b1, W2, b2 = _make_net(seed + 200)
            grads = backward2(x, y, W1, b1, W2, b2)
            assert grads["dW1"].shape == W1.shape
            assert grads["db1"].shape == b1.shape
            assert grads["dW2"].shape == W2.shape
            assert grads["db2"].shape == b2.shape

    def test_backward2_dz2_structure(self):
        """dz2 = (p - onehot)/N encodes: sum over rows is 0, and sums to 0 over all entries."""
        # db2 = dz2.sum(axis=0). The full dz2 matrix must sum to 0 (p and onehot have same row sum).
        np.random.seed(106)
        for seed in range(4):
            x, y, W1, b1, W2, b2 = _make_net(seed + 300)
            grads = backward2(x, y, W1, b1, W2, b2)
            # db2 is sum of dz2 rows; the TOTAL of db2 must be ≈ 0 (sum p_i - sum onehot_i)/N
            assert np.isclose(grads["db2"].sum(), 0.0, atol=1e-9), \
                f"db2.sum() should be 0 (p and onehot both sum to 1 per row), got {grads['db2'].sum()}"


# ---------------------------------------------------------------------------
# BACKWARD2 — Finite-difference gradient checks on diverse random inputs
# ---------------------------------------------------------------------------

class TestBackward2FiniteDiff:
    @pytest.mark.parametrize("seed,N,D,H,C", [
        (110, 4,  3,  5,  3),
        (111, 7,  5,  4,  4),
        (112, 2,  8,  6,  2),
        (113, 10, 2,  3,  5),
        (114, 1,  4,  7,  3),   # single-example
    ])
    def test_finite_diff_all_params(self, seed, N, D, H, C):
        """Analytic gradients must match finite-difference estimates (atol=1e-5) for all params."""
        np.random.seed(seed)
        x  = np.random.randn(N, D)
        y  = np.random.randint(0, C, size=N)
        W1 = np.random.randn(D, H) * 0.5
        b1 = np.random.randn(H) * 0.1
        W2 = np.random.randn(H, C) * 0.5
        b2 = np.random.randn(C) * 0.1

        grads = backward2(x, y, W1, b1, W2, b2)

        num_dW1 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), W1)
        num_db1 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), b1)
        num_dW2 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), W2)
        num_db2 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), b2)

        assert np.allclose(grads["dW1"], num_dW1, atol=1e-5), \
            f"dW1 finite-diff mismatch at seed={seed}"
        assert np.allclose(grads["db1"], num_db1, atol=1e-5), \
            f"db1 finite-diff mismatch at seed={seed}"
        assert np.allclose(grads["dW2"], num_dW2, atol=1e-5), \
            f"dW2 finite-diff mismatch at seed={seed}"
        assert np.allclose(grads["db2"], num_db2, atol=1e-5), \
            f"db2 finite-diff mismatch at seed={seed}"

    def test_finite_diff_small_weights(self):
        """Gradient check with near-zero weights (relu mostly inactive)."""
        np.random.seed(115)
        N, D, H, C = 4, 3, 5, 3
        x  = np.random.randn(N, D)
        y  = np.random.randint(0, C, size=N)
        # Very small weights -> z1 will be small, many relu activations might be 0
        W1 = np.random.randn(D, H) * 0.01
        b1 = np.zeros(H)
        W2 = np.random.randn(H, C) * 0.01
        b2 = np.zeros(C)

        grads = backward2(x, y, W1, b1, W2, b2)
        num_dW2 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), W2)
        num_db2 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), b2)
        assert np.allclose(grads["dW2"], num_dW2, atol=1e-5)
        assert np.allclose(grads["db2"], num_db2, atol=1e-5)

    def test_finite_diff_large_weights(self):
        """Gradient check with large-scale weights (atol=1e-4: finite-diff accuracy degrades
        for large logits due to softmax saturation, but analytic gradients remain exact)."""
        np.random.seed(116)
        N, D, H, C = 3, 4, 6, 3
        x  = np.random.randn(N, D)
        y  = np.random.randint(0, C, size=N)
        W1 = np.random.randn(D, H) * 2.0
        b1 = np.random.randn(H) * 2.0
        W2 = np.random.randn(H, C) * 2.0
        b2 = np.random.randn(C) * 2.0

        grads = backward2(x, y, W1, b1, W2, b2)
        num_dW1 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), W1)
        num_db1 = numerical_gradient(lambda: forward2(x, W1, b1, W2, b2, y), b1)
        # atol=1e-4: large logits reduce finite-difference accuracy to ~4-5 significant digits
        assert np.allclose(grads["dW1"], num_dW1, atol=1e-4)
        assert np.allclose(grads["db1"], num_db1, atol=1e-4)


# ---------------------------------------------------------------------------
# NUMERICAL_GRADIENT utility — own property tests (independent of the network)
# ---------------------------------------------------------------------------

class TestNumericalGradient:
    def test_cubic_function(self):
        """f(p) = sum(p^3) -> grad = 3*p^2."""
        np.random.seed(120)
        param = np.random.randn(5)
        snapshot = param.copy()
        g = numerical_gradient(lambda: float(np.sum(param ** 3)), param)
        assert np.allclose(g, 3 * snapshot ** 2, atol=1e-5)

    def test_sine_function(self):
        """f(p) = sum(sin(p)) -> grad = cos(p)."""
        np.random.seed(121)
        param = np.random.randn(4)
        snapshot = param.copy()
        g = numerical_gradient(lambda: float(np.sum(np.sin(param))), param)
        assert np.allclose(g, np.cos(snapshot), atol=1e-5)

    def test_2d_param(self):
        """Works on 2-D parameter array; grad shape matches param shape."""
        np.random.seed(122)
        param = np.random.randn(3, 4)
        g = numerical_gradient(lambda: float(np.sum(param ** 2)), param)
        assert g.shape == param.shape
        assert np.allclose(g, 2 * param, atol=1e-5)

    def test_restores_param_after_perturbation(self):
        """Parameter array must be bit-for-bit identical after gradient computation."""
        np.random.seed(123)
        param = np.random.randn(6)
        snapshot = param.copy()
        numerical_gradient(lambda: float(np.sum(np.exp(param))), param)
        assert np.array_equal(param, snapshot), \
            "numerical_gradient modified param without restoring it"

    def test_scalar_param(self):
        """Works for a 0-D or (1,) array: f(p) = p^4 -> 4*p^3."""
        param = np.array([2.0])
        g = numerical_gradient(lambda: float(param[0] ** 4), param)
        assert np.isclose(g[0], 4 * 2.0 ** 3, atol=1e-5)

    def test_zero_gradient(self):
        """Constant function has zero gradient everywhere."""
        param = np.array([1.0, -2.0, 3.0])
        g = numerical_gradient(lambda: 42.0, param)
        assert np.allclose(g, 0.0, atol=1e-9)


# ---------------------------------------------------------------------------
# FORWARD2 — consistency and numerical properties
# ---------------------------------------------------------------------------

class TestForward2:
    def test_forward2_is_finite_and_positive_on_random_inputs(self):
        """Loss is always finite and positive for any random network."""
        np.random.seed(130)
        for seed in range(8):
            x, y, W1, b1, W2, b2 = _make_net(seed + 400)
            loss = forward2(x, W1, b1, W2, b2, y)
            assert np.isfinite(loss), f"forward2 returned non-finite loss at seed={seed}"
            assert loss > 0.0, f"forward2 returned non-positive loss at seed={seed}"

    def test_forward2_equals_manual_pipeline(self):
        """forward2 must equal manual z1->a1->z2->softmax->cross_entropy chain."""
        np.random.seed(131)
        for seed in range(5):
            x, y, W1, b1, W2, b2 = _make_net(seed + 500)
            z1 = x @ W1 + b1
            a1 = relu(z1)
            z2 = a1 @ W2 + b2
            p = softmax(z2)
            expected = cross_entropy(p, y)
            result = forward2(x, W1, b1, W2, b2, y)
            assert np.isclose(result, expected, atol=1e-10), \
                f"forward2 deviates from manual pipeline at seed={seed}"

    def test_forward2_matches_torch_cross_entropy(self):
        """forward2 output must match F.cross_entropy(logits, y) in PyTorch."""
        np.random.seed(132)
        torch.manual_seed(132)
        for seed in range(5):
            x, y, W1, b1, W2, b2 = _make_net(seed + 600)
            our_loss = forward2(x, W1, b1, W2, b2, y)

            # Build same computation in torch
            z1_t = torch.tensor(x @ W1 + b1, dtype=torch.float64)
            logits_t = torch.relu(z1_t) @ torch.tensor(W2, dtype=torch.float64) + \
                       torch.tensor(b2, dtype=torch.float64)
            y_t = torch.tensor(y, dtype=torch.long)
            torch_loss = F.cross_entropy(logits_t, y_t, reduction="mean").item()

            assert np.isclose(our_loss, torch_loss, atol=1e-5), \
                f"forward2 vs torch mismatch at seed={seed}: ours={our_loss}, torch={torch_loss}"

    def test_forward2_large_batch(self):
        """forward2 handles larger batches without overflow."""
        np.random.seed(133)
        x, y, W1, b1, W2, b2 = _make_net(133, N=64, D=16, H=32, C=8)
        loss = forward2(x, W1, b1, W2, b2, y)
        assert np.isfinite(loss) and loss > 0.0

    def test_forward2_single_example(self):
        """forward2 with N=1 must work and return a positive scalar."""
        np.random.seed(134)
        x, y, W1, b1, W2, b2 = _make_net(134, N=1, D=4, H=6, C=3)
        loss = forward2(x, W1, b1, W2, b2, y)
        assert np.isfinite(loss) and loss > 0.0

    def test_forward2_decreases_under_gradient_descent(self):
        """Multiple GD steps using backward2 must decrease forward2 loss."""
        np.random.seed(135)
        x, y, W1, b1, W2, b2 = _make_net(700, N=8, D=4, H=8, C=3)
        lr = 0.3
        initial_loss = forward2(x, W1, b1, W2, b2, y)
        for _ in range(50):
            g = backward2(x, y, W1, b1, W2, b2)
            W1 = W1 - lr * g["dW1"]
            b1 = b1 - lr * g["db1"]
            W2 = W2 - lr * g["dW2"]
            b2 = b2 - lr * g["db2"]
        final_loss = forward2(x, W1, b1, W2, b2, y)
        assert final_loss < initial_loss, \
            f"loss did not decrease: {initial_loss:.4f} -> {final_loss:.4f}"
