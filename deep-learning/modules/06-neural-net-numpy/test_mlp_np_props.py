"""
Property-based / oracle tests for module 06 — neural net numpy forward pass.

Strategy
--------
1. PyTorch oracle   — compare every function against the equivalent torch op on
                      random inputs.  Catches wrong formulas that pass hand-crafted
                      fixed tests.
2. Finite-difference gradient check (numerical)
                    — for cross_entropy, perturb each logit by ±eps and verify
                      the numeric gradient is close to the analytic one we'd
                      compute by hand (also verified vs torch autograd).
3. Invariants       — softmax rows sum to 1, probabilities in [0,1], relu
                      non-negative, shapes preserved, return-type contract.
4. Edge-cases       — zeros, negatives, single example, large magnitudes, N=1.

All calls to the module under test happen INSIDE test functions so the file
imports and collects cleanly against an unfilled NotImplementedError stub.
"""

import numpy as np
import pytest
import torch
import torch.nn.functional as F

from mlp_np import (
    relu,
    softmax,
    linear,
    forward2,
    cross_entropy,
    init_xavier,
    init_he,
)


# ============================================================
# helpers
# ============================================================

def _to_torch(arr, requires_grad=False):
    return torch.tensor(arr, dtype=torch.float64, requires_grad=requires_grad)


# ============================================================
# relu — oracle vs torch, invariants, edge-cases
# ============================================================

class TestReluOracle:
    def test_relu_vs_torch_random_shapes(self):
        """relu matches torch.relu on a variety of random shapes."""
        np.random.seed(100)
        torch.manual_seed(100)
        shapes = [(1,), (10,), (4, 7), (3, 5, 6), (2, 3, 4, 5)]
        for shape in shapes:
            x = np.random.randn(*shape)
            np_out = relu(x)
            t_out = torch.relu(_to_torch(x)).numpy()
            assert np.allclose(np_out, t_out, atol=1e-8), \
                f"relu mismatch for shape {shape}"

    def test_relu_vs_torch_loop_over_seeds(self):
        """Multiple random seeds, 2-D inputs."""
        for seed in range(101, 111):
            np.random.seed(seed)
            x = np.random.randn(8, 12) * 3.0
            np_out = relu(x)
            t_out = torch.relu(_to_torch(x)).numpy()
            assert np.allclose(np_out, t_out, atol=1e-8), \
                f"relu mismatch at seed={seed}"

    def test_relu_nonneg_invariant(self):
        """Output is always non-negative regardless of input sign."""
        np.random.seed(102)
        for _ in range(10):
            x = np.random.randn(20, 30)
            assert np.all(relu(x) >= 0.0)

    def test_relu_preserves_shape(self):
        """Output shape equals input shape for arbitrary shapes."""
        np.random.seed(103)
        shapes = [(1,), (1, 1), (100,), (7, 8, 9)]
        for s in shapes:
            x = np.random.randn(*s)
            assert relu(x).shape == s

    def test_relu_zeros_all_negative(self):
        """All-negative input => all-zero output."""
        x = -np.abs(np.random.randn(5, 5)) - 1.0
        assert np.all(relu(x) == 0.0)

    def test_relu_identity_all_positive(self):
        """All-positive input passes through unchanged."""
        np.random.seed(104)
        x = np.abs(np.random.randn(5, 5)) + 0.1
        assert np.allclose(relu(x), x)

    def test_relu_does_not_mutate_input(self):
        """Input array is not modified in-place."""
        np.random.seed(105)
        x = np.random.randn(6, 6)
        orig = x.copy()
        relu(x)
        assert np.allclose(x, orig)

    def test_relu_zero_boundary(self):
        """Exactly-zero values map to zero (not negative)."""
        x = np.zeros((3, 3))
        assert np.all(relu(x) == 0.0)

    def test_relu_single_element(self):
        """Works correctly on a scalar-like array."""
        assert relu(np.array([-5.0])) == pytest.approx(0.0)
        assert relu(np.array([5.0])) == pytest.approx(5.0)

    def test_relu_large_magnitude_no_overflow(self):
        """Large positive values pass through without inf."""
        x = np.array([[1e38, -1e38]])
        out = relu(x)
        assert out[0, 0] == pytest.approx(1e38)
        assert out[0, 1] == pytest.approx(0.0)
        assert np.all(np.isfinite(out))


# ============================================================
# softmax — oracle vs torch, invariants, stability, edge-cases
# ============================================================

class TestSoftmaxOracle:
    def test_softmax_vs_torch_2d_random(self):
        """softmax matches torch.softmax(dim=-1) on random (N, C) inputs."""
        np.random.seed(200)
        torch.manual_seed(200)
        for seed in range(200, 215):
            np.random.seed(seed)
            x = np.random.randn(np.random.randint(2, 10), np.random.randint(2, 8))
            np_out = softmax(x)
            t_out = torch.softmax(_to_torch(x), dim=-1).numpy()
            assert np.allclose(np_out, t_out, atol=1e-6), \
                f"softmax mismatch at seed={seed}, shape={x.shape}"

    def test_softmax_vs_torch_3d_random(self):
        """softmax on 3-D input matches torch over last axis."""
        np.random.seed(201)
        x = np.random.randn(4, 5, 6)
        np_out = softmax(x)
        t_out = torch.softmax(_to_torch(x), dim=-1).numpy()
        assert np.allclose(np_out, t_out, atol=1e-6)

    def test_softmax_rows_sum_to_one_random(self):
        """Every row sums to exactly 1 for 20 random seeds."""
        for seed in range(202, 222):
            np.random.seed(seed)
            x = np.random.randn(np.random.randint(1, 15),
                                np.random.randint(2, 10))
            out = softmax(x)
            assert np.allclose(out.sum(axis=-1), 1.0, atol=1e-8), \
                f"row sums != 1 at seed={seed}"

    def test_softmax_nonneg_random(self):
        """All outputs are non-negative for many random inputs."""
        np.random.seed(203)
        for _ in range(20):
            x = np.random.randn(8, 5)
            assert np.all(softmax(x) >= 0.0)

    def test_softmax_values_in_0_1(self):
        """Output values are in [0, 1] (proper probability)."""
        np.random.seed(204)
        x = np.random.randn(10, 7)
        out = softmax(x)
        assert np.all(out >= 0.0) and np.all(out <= 1.0)

    def test_softmax_numerically_stable_large_positive(self):
        """No inf/nan with very large positive logits."""
        x = np.array([[1000.0, 999.0, 998.0]])
        out = softmax(x)
        assert np.all(np.isfinite(out))
        # largest logit should dominate
        assert out[0, 0] > out[0, 1] > out[0, 2]

    def test_softmax_numerically_stable_large_negative(self):
        """No inf/nan with very large negative logits."""
        x = np.array([[-1000.0, -999.0, -998.0]])
        out = softmax(x)
        assert np.all(np.isfinite(out))
        assert np.allclose(out.sum(axis=-1), 1.0, atol=1e-8)

    def test_softmax_invariant_to_constant_shift(self):
        """Adding a constant to all logits does not change softmax (10 seeds)."""
        np.random.seed(205)
        for c in [0.0, 1.0, -5.0, 100.0, -100.0]:
            x = np.random.randn(6, 5)
            assert np.allclose(softmax(x), softmax(x + c), atol=1e-7), \
                f"softmax not shift-invariant for c={c}"

    def test_softmax_uniform_is_one_over_C(self):
        """Uniform logits => uniform distribution."""
        for C in [2, 3, 5, 10]:
            x = np.zeros((4, C))
            out = softmax(x)
            assert np.allclose(out, 1.0 / C, atol=1e-8)

    def test_softmax_one_dominant_class(self):
        """Very large logit for one class => probability near 1, rest near 0."""
        x = np.array([[0.0, 500.0, 0.0]])
        out = softmax(x)
        assert out[0, 1] > 0.9999
        assert np.allclose(out.sum(axis=-1), 1.0, atol=1e-8)

    def test_softmax_preserves_shape(self):
        """Output shape equals input shape."""
        np.random.seed(206)
        for shape in [(1, 2), (5, 3), (10, 10), (2, 4, 6)]:
            x = np.random.randn(*shape)
            assert softmax(x).shape == shape

    def test_softmax_single_row(self):
        """Works on a single example (N=1)."""
        x = np.array([[1.0, 2.0, 3.0]])
        out = softmax(x)
        assert out.shape == (1, 3)
        assert np.allclose(out.sum(), 1.0, atol=1e-8)

    def test_softmax_2_classes(self):
        """Two-class softmax is equivalent to sigmoid: p[1] = sigmoid(z1-z0)."""
        np.random.seed(207)
        x = np.random.randn(10, 2)
        out = softmax(x)
        sigmoid = 1.0 / (1.0 + np.exp(-(x[:, 1] - x[:, 0])))
        assert np.allclose(out[:, 1], sigmoid, atol=1e-7)


# ============================================================
# linear — oracle vs torch, shapes, edge-cases
# ============================================================

class TestLinearOracle:
    def test_linear_vs_torch_random(self):
        """linear matches torch.nn.functional.linear on random shapes."""
        np.random.seed(300)
        for seed in range(300, 315):
            np.random.seed(seed)
            N = np.random.randint(1, 10)
            in_ = np.random.randint(1, 8)
            out_ = np.random.randint(1, 8)
            x = np.random.randn(N, in_)
            W = np.random.randn(in_, out_)
            b = np.random.randn(out_)

            np_out = linear(x, W, b)
            # torch linear: y = x @ W^T + b, but our W is (in, out) already
            # equivalent: F.linear(x, W.T, b)
            t_out = F.linear(
                _to_torch(x).float(),
                _to_torch(W).T.float(),
                _to_torch(b).float(),
            ).double().numpy()
            # Use float32-compatible tolerance since we convert
            t_out = (
                (_to_torch(x) @ _to_torch(W) + _to_torch(b)).numpy()
            )
            assert np.allclose(np_out, t_out, atol=1e-8), \
                f"linear mismatch at seed={seed}"

    def test_linear_output_shape(self):
        """Output shape is (N, out) for all tested configs."""
        np.random.seed(301)
        configs = [(1, 1, 1), (5, 3, 4), (10, 8, 2), (3, 7, 7)]
        for N, in_, out_ in configs:
            x = np.random.randn(N, in_)
            W = np.random.randn(in_, out_)
            b = np.random.randn(out_)
            assert linear(x, W, b).shape == (N, out_)

    def test_linear_bias_broadcasts(self):
        """Bias is added to every row (broadcasting check)."""
        np.random.seed(302)
        x = np.random.randn(4, 3)
        W = np.eye(3)
        b = np.array([10.0, 20.0, 30.0])
        out = linear(x, W, b)
        expected = x + b  # W = I so x @ W = x
        assert np.allclose(out, expected, atol=1e-8)

    def test_linear_zero_weights(self):
        """Zero weight matrix: output equals the bias repeated for each row."""
        np.random.seed(303)
        x = np.random.randn(5, 3)
        W = np.zeros((3, 4))
        b = np.array([1.0, 2.0, 3.0, 4.0])
        out = linear(x, W, b)
        assert np.allclose(out, np.tile(b, (5, 1)), atol=1e-8)

    def test_linear_single_example(self):
        """Works for a single example (N=1)."""
        x = np.array([[1.0, 0.0, -1.0]])
        W = np.eye(3)
        b = np.zeros(3)
        assert np.allclose(linear(x, W, b), x, atol=1e-8)

    def test_linear_matches_matmul(self):
        """linear(x, W, b) == x @ W + b for varied seeds."""
        np.random.seed(304)
        for _ in range(10):
            N, in_, out_ = (
                np.random.randint(2, 12),
                np.random.randint(2, 8),
                np.random.randint(2, 8),
            )
            x = np.random.randn(N, in_)
            W = np.random.randn(in_, out_)
            b = np.random.randn(out_)
            assert np.allclose(linear(x, W, b), x @ W + b, atol=1e-8)


# ============================================================
# cross_entropy — oracle vs torch, finite-difference gradient check, invariants
# ============================================================

class TestCrossEntropyOracle:
    def test_cross_entropy_vs_torch_random(self):
        """cross_entropy matches torch.nn.functional.cross_entropy on random inputs."""
        np.random.seed(400)
        for seed in range(400, 420):
            np.random.seed(seed)
            N = np.random.randint(1, 16)
            C = np.random.randint(2, 8)
            logits = np.random.randn(N, C)
            y = np.random.randint(0, C, size=N)

            np_loss = cross_entropy(logits, y)

            t_logits = _to_torch(logits).float()
            t_y = torch.tensor(y, dtype=torch.long)
            t_loss = float(F.cross_entropy(t_logits, t_y).item())

            assert abs(np_loss - t_loss) < 2e-5, \
                f"cross_entropy mismatch at seed={seed}: np={np_loss:.6f} torch={t_loss:.6f}"

    def test_cross_entropy_returns_python_float(self):
        """Return type is a plain Python float, not ndarray or tensor."""
        np.random.seed(401)
        logits = np.random.randn(5, 3)
        y = np.random.randint(0, 3, size=5)
        loss = cross_entropy(logits, y)
        assert type(loss) is float, f"expected float, got {type(loss)}"

    def test_cross_entropy_single_example(self):
        """N=1 case matches torch."""
        np.random.seed(402)
        logits = np.array([[2.0, 1.0, 0.5]])
        y = np.array([0])
        np_loss = cross_entropy(logits, y)
        t_loss = float(F.cross_entropy(
            _to_torch(logits).float(),
            torch.tensor([0], dtype=torch.long)
        ).item())
        assert abs(np_loss - t_loss) < 1e-5

    def test_cross_entropy_uniform_equals_log_C(self):
        """Uniform logits (zeros) => loss = log(C)."""
        np.random.seed(403)
        for C in [2, 3, 5, 10]:
            N = 8
            logits = np.zeros((N, C))
            y = np.random.randint(0, C, size=N)
            loss = cross_entropy(logits, y)
            assert abs(loss - np.log(C)) < 1e-7, \
                f"uniform loss should be log({C})={np.log(C):.6f}, got {loss:.6f}"

    def test_cross_entropy_near_zero_for_confident_correct(self):
        """Correct class with logit >> others => near-zero loss."""
        logits = np.array([[100.0, 0.0, 0.0],
                           [0.0, 100.0, 0.0],
                           [0.0, 0.0, 100.0]])
        y = np.array([0, 1, 2])
        loss = cross_entropy(logits, y)
        assert loss < 1e-5

    def test_cross_entropy_numerically_stable_extreme_logits(self):
        """No inf/nan with extreme logit magnitudes."""
        logits = np.array([[1000.0, -1000.0],
                           [-1000.0, 1000.0]])
        y = np.array([0, 1])
        loss = cross_entropy(logits, y)
        assert np.isfinite(loss)
        assert loss < 1e-5

    def test_cross_entropy_nonnegative(self):
        """Cross-entropy is always >= 0 (it is -log of a probability in [0,1])."""
        np.random.seed(404)
        for seed in range(404, 424):
            np.random.seed(seed)
            N, C = np.random.randint(1, 10), np.random.randint(2, 8)
            logits = np.random.randn(N, C)
            y = np.random.randint(0, C, size=N)
            assert cross_entropy(logits, y) >= 0.0

    def test_cross_entropy_scalar_output(self):
        """Return value is a scalar (not a vector of per-sample losses)."""
        np.random.seed(405)
        logits = np.random.randn(7, 4)
        y = np.random.randint(0, 4, size=7)
        loss = cross_entropy(logits, y)
        assert np.ndim(loss) == 0 or (isinstance(loss, float))

    def test_cross_entropy_shift_invariant(self):
        """Adding constant to ALL logits in a row does not change cross-entropy."""
        np.random.seed(406)
        logits = np.random.randn(6, 4)
        y = np.random.randint(0, 4, size=6)
        loss1 = cross_entropy(logits, y)
        loss2 = cross_entropy(logits + 50.0, y)
        assert abs(loss1 - loss2) < 1e-8

    def test_cross_entropy_finite_difference_gradient_check(self):
        """
        Numeric finite-difference gradient of cross_entropy w.r.t. logits must
        match the analytic gradient (softmax(logits) - one_hot(y)) / N.

        This is a genuine gradient check: it will FAIL if the loss formula is
        wrong even when the loss value itself looks plausible.
        """
        np.random.seed(407)
        N, C = 5, 4
        logits = np.random.randn(N, C)
        y = np.random.randint(0, C, size=N)
        eps = 1e-5

        # Numeric gradient via central differences
        grad_num = np.zeros_like(logits)
        for i in range(N):
            for j in range(C):
                logits_p = logits.copy()
                logits_m = logits.copy()
                logits_p[i, j] += eps
                logits_m[i, j] -= eps
                grad_num[i, j] = (
                    cross_entropy(logits_p, y) - cross_entropy(logits_m, y)
                ) / (2 * eps)

        # Analytic gradient: (p - one_hot) / N
        p = np.exp(logits - logits.max(axis=1, keepdims=True))
        p /= p.sum(axis=1, keepdims=True)
        grad_ana = p.copy()
        grad_ana[np.arange(N), y] -= 1.0
        grad_ana /= N

        assert np.allclose(grad_num, grad_ana, atol=1e-5), \
            "finite-difference gradient does not match analytic gradient of cross_entropy"

    def test_cross_entropy_gradient_vs_torch_autograd(self):
        """
        Analytic gradient (softmax - one_hot)/N matches torch autograd gradient.
        Validates that both the loss formula and the gradient formula are correct.
        """
        np.random.seed(408)
        N, C = 6, 5
        logits_np = np.random.randn(N, C)
        y_np = np.random.randint(0, C, size=N)

        # Torch autograd
        t_logits = _to_torch(logits_np).float().requires_grad_(True)
        t_y = torch.tensor(y_np, dtype=torch.long)
        t_loss = F.cross_entropy(t_logits, t_y)
        t_loss.backward()
        grad_torch = t_logits.grad.double().numpy()

        # Analytic (numpy)
        p = np.exp(logits_np - logits_np.max(axis=1, keepdims=True))
        p /= p.sum(axis=1, keepdims=True)
        grad_ana = p.copy()
        grad_ana[np.arange(N), y_np] -= 1.0
        grad_ana /= N

        assert np.allclose(grad_ana, grad_torch, atol=1e-5), \
            "analytic gradient diverges from torch autograd"


# ============================================================
# forward2 — oracle vs torch, shape, end-to-end
# ============================================================

class TestForward2Oracle:
    def _make_net(self, D, H, C, seed):
        rng = np.random.RandomState(seed)
        W1 = rng.randn(D, H) * 0.1
        b1 = rng.randn(H) * 0.1
        W2 = rng.randn(H, C) * 0.1
        b2 = rng.randn(C) * 0.1
        return W1, b1, W2, b2

    def _torch_forward2(self, x, W1, b1, W2, b2):
        """Equivalent PyTorch forward pass (float64 precision)."""
        tx = _to_torch(x)
        tW1, tb1 = _to_torch(W1), _to_torch(b1)
        tW2, tb2 = _to_torch(W2), _to_torch(b2)
        h = torch.relu(tx @ tW1 + tb1)
        logits = h @ tW2 + tb2
        return logits.numpy()

    def test_forward2_vs_torch_random(self):
        """forward2 matches the PyTorch-based reference across random configs."""
        np.random.seed(500)
        configs = [
            (10, 4, 8, 3),
            (1, 2, 4, 2),
            (32, 16, 64, 10),
            (7, 5, 12, 5),
        ]
        for seed, (N, D, H, C) in enumerate(configs, start=500):
            np.random.seed(seed)
            x = np.random.randn(N, D)
            W1, b1, W2, b2 = self._make_net(D, H, C, seed=seed + 1)
            np_out = forward2(x, W1, b1, W2, b2)
            t_out = self._torch_forward2(x, W1, b1, W2, b2)
            assert np.allclose(np_out, t_out, atol=1e-8), \
                f"forward2 mismatch for config {(N,D,H,C)} seed={seed}"

    def test_forward2_output_shape(self):
        """Output shape is always (N, C)."""
        np.random.seed(501)
        for N, D, H, C in [(1, 1, 1, 1), (5, 4, 8, 3), (20, 10, 32, 7)]:
            x = np.random.randn(N, D)
            W1, b1, W2, b2 = self._make_net(D, H, C, seed=501)
            out = forward2(x, W1, b1, W2, b2)
            assert out.shape == (N, C), \
                f"expected ({N},{C}), got {out.shape}"

    def test_forward2_relu_in_hidden_layer(self):
        """Removing ReLU from the hidden layer produces a different result (non-linear vs linear)."""
        np.random.seed(502)
        N, D, H, C = 8, 4, 10, 3
        x = np.random.randn(N, D)
        W1, b1, W2, b2 = self._make_net(D, H, C, seed=502)
        with_relu = forward2(x, W1, b1, W2, b2)
        # linear only (no ReLU)
        linear_only = (x @ W1 + b1) @ W2 + b2
        # For networks with negative pre-activations the two will differ
        assert not np.allclose(with_relu, linear_only), \
            "forward2 should apply ReLU in the hidden layer"

    def test_forward2_single_example(self):
        """Works for a single example (N=1) without shape errors."""
        np.random.seed(503)
        D, H, C = 5, 8, 4
        x = np.random.randn(1, D)
        W1, b1, W2, b2 = self._make_net(D, H, C, seed=503)
        out = forward2(x, W1, b1, W2, b2)
        assert out.shape == (1, C)

    def test_forward2_zero_input(self):
        """Zero input: hidden layer (after relu) and logits computed correctly."""
        np.random.seed(504)
        D, H, C = 4, 6, 3
        x = np.zeros((5, D))
        W1, b1, W2, b2 = self._make_net(D, H, C, seed=504)
        out = forward2(x, W1, b1, W2, b2)
        # x=0 => h = relu(b1), logits = relu(b1) @ W2 + b2
        expected_h = np.maximum(0, b1)
        expected_logits = expected_h @ W2 + b2
        assert np.allclose(out, np.tile(expected_logits, (5, 1)), atol=1e-8)

    def test_forward2_logits_unbounded(self):
        """Logits are not constrained (can be negative or > 1) — no activation on output."""
        np.random.seed(505)
        N, D, H, C = 20, 5, 10, 3
        x = np.random.randn(N, D) * 5.0
        W1, b1, W2, b2 = self._make_net(D, H, C, seed=505)
        out = forward2(x, W1, b1, W2, b2)
        # If any logit is > 1.0 or < 0.0 the output is not probabilities (correct)
        # We just check shape and no nan/inf
        assert out.shape == (N, C)
        assert np.all(np.isfinite(out))

    def test_forward2_no_relu_on_output(self):
        """Output layer has NO ReLU — logits can be negative."""
        np.random.seed(506)
        D, H, C = 3, 4, 2
        x = np.random.randn(10, D)
        # Use large negative biases on b2 to force negative logits if no relu
        W1 = np.zeros((D, H))
        b1 = np.zeros(H)
        W2 = np.zeros((H, C))
        b2 = np.array([-100.0, -100.0])
        out = forward2(x, W1, b1, W2, b2)
        # logits should be exactly b2 (since W1=0 => h=0 => output = 0@W2 + b2)
        assert np.allclose(out, np.tile(b2, (10, 1)), atol=1e-8), \
            "Output layer must not apply ReLU"


# ============================================================
# End-to-end pipeline: forward2 + cross_entropy vs torch
# ============================================================

class TestPipelineOracle:
    def test_pipeline_vs_torch_full(self):
        """forward2 + cross_entropy pipeline matches torch end-to-end."""
        np.random.seed(600)
        for seed in range(600, 610):
            np.random.seed(seed)
            N, D, H, C = (
                np.random.randint(2, 16),
                np.random.randint(2, 8),
                np.random.randint(2, 16),
                np.random.randint(2, 6),
            )
            x = np.random.randn(N, D)
            rng = np.random.RandomState(seed)
            W1 = rng.randn(D, H) * 0.1
            b1 = rng.randn(H) * 0.1
            W2 = rng.randn(H, C) * 0.1
            b2 = rng.randn(C) * 0.1
            y = np.random.randint(0, C, size=N)

            np_loss = cross_entropy(forward2(x, W1, b1, W2, b2), y)

            tx = _to_torch(x).float()
            tW1, tb1 = _to_torch(W1).float(), _to_torch(b1).float()
            tW2, tb2 = _to_torch(W2).float(), _to_torch(b2).float()
            ty = torch.tensor(y, dtype=torch.long)
            t_h = torch.relu(tx @ tW1 + tb1)
            t_logits = t_h @ tW2 + tb2
            t_loss = float(F.cross_entropy(t_logits, ty).item())

            assert abs(np_loss - t_loss) < 2e-4, \
                f"pipeline loss mismatch at seed={seed}: np={np_loss:.6f} torch={t_loss:.6f}"

    def test_pipeline_loss_is_finite_and_nonneg(self):
        """Forward + cross_entropy never produces nan/inf or negative values."""
        np.random.seed(601)
        for _ in range(20):
            N, D, H, C = (
                np.random.randint(1, 10),
                np.random.randint(2, 6),
                np.random.randint(2, 8),
                np.random.randint(2, 5),
            )
            x = np.random.randn(N, D)
            rng = np.random.RandomState(np.random.randint(1000))
            W1 = rng.randn(D, H) * 0.1
            b1 = rng.randn(H) * 0.1
            W2 = rng.randn(H, C) * 0.1
            b2 = rng.randn(C) * 0.1
            y = np.random.randint(0, C, size=N)
            loss = cross_entropy(forward2(x, W1, b1, W2, b2), y)
            assert np.isfinite(loss) and loss >= 0.0


# ============================================================
# init_xavier / init_he — variance oracle, shape/finite invariants,
# symmetry-breaking, and the "dead network" integration experiment
# ============================================================

class TestWeightInitVarianceOracle:
    """Empirical std of a large draw must match the theoretical target."""

    # (fan_in, fan_out) configs spanning rectangular and square layers.
    SHAPES = [(64, 64), (128, 32), (16, 256), (200, 50), (50, 200)]

    def test_xavier_empirical_std_matches_theory(self):
        """std of init_xavier draw ~ sqrt(2/(fan_in+fan_out)) within 10% (seeded)."""
        for i, (fan_in, fan_out) in enumerate(self.SHAPES):
            rng = np.random.default_rng(1000 + i)
            W = init_xavier(fan_in, fan_out, rng=rng)
            target = np.sqrt(2.0 / (fan_in + fan_out))
            emp = float(W.std())
            assert abs(emp - target) <= 0.10 * target, (
                f"xavier std {emp:.5f} not within 10% of target {target:.5f} "
                f"for ({fan_in},{fan_out})"
            )

    def test_he_empirical_std_matches_theory(self):
        """std of init_he draw ~ sqrt(2/fan_in) within 10% (seeded)."""
        for i, (fan_in, fan_out) in enumerate(self.SHAPES):
            rng = np.random.default_rng(2000 + i)
            W = init_he(fan_in, fan_out, rng=rng)
            target = np.sqrt(2.0 / fan_in)
            emp = float(W.std())
            assert abs(emp - target) <= 0.10 * target, (
                f"he std {emp:.5f} not within 10% of target {target:.5f} "
                f"for ({fan_in},{fan_out})"
            )

    def test_xavier_std_over_10000_draws(self):
        """Pooled std over 10000 small draws converges to the Xavier target."""
        fan_in, fan_out = 5, 7
        target = np.sqrt(2.0 / (fan_in + fan_out))
        rng = np.random.default_rng(123)
        draws = np.stack(
            [init_xavier(fan_in, fan_out, rng=rng) for _ in range(10000)]
        )
        emp = float(draws.std())
        assert abs(emp - target) <= 0.05 * target, (
            f"pooled xavier std {emp:.5f} vs target {target:.5f}"
        )

    def test_he_mean_is_approximately_zero(self):
        """He init is zero-centred (symmetric noise), so the mean ~ 0."""
        rng = np.random.default_rng(7)
        W = init_he(128, 128, rng=rng)
        assert abs(float(W.mean())) < 0.02, "He init should be zero-centred"


class TestWeightInitInvariants:
    def test_shapes_are_exact(self):
        """Both initialisers return exactly (fan_in, fan_out)."""
        for fan_in, fan_out in [(1, 1), (3, 8), (10, 4), (64, 128)]:
            rng = np.random.default_rng(0)
            assert init_xavier(fan_in, fan_out, rng=rng).shape == (fan_in, fan_out)
            assert init_he(fan_in, fan_out, rng=rng).shape == (fan_in, fan_out)

    def test_all_values_finite(self):
        """No nan/inf in either initialiser."""
        rng = np.random.default_rng(11)
        assert np.all(np.isfinite(init_xavier(50, 60, rng=rng)))
        assert np.all(np.isfinite(init_he(60, 50, rng=rng)))

    def test_random_init_breaks_symmetry_nonzero(self):
        """Random init is non-degenerate: columns differ (symmetry broken)."""
        rng = np.random.default_rng(42)
        W = init_he(8, 5, rng=rng)
        # Not all zeros, and no two columns identical (would mean tied neurons).
        assert not np.allclose(W, 0.0)
        for a in range(W.shape[1]):
            for b in range(a + 1, W.shape[1]):
                assert not np.allclose(W[:, a], W[:, b]), \
                    "two hidden units initialised identically — symmetry not broken"

    def test_reproducible_with_seeded_rng(self):
        """Same seed => identical draw; different seed => different draw."""
        a = init_xavier(20, 30, rng=np.random.default_rng(99))
        b = init_xavier(20, 30, rng=np.random.default_rng(99))
        c = init_xavier(20, 30, rng=np.random.default_rng(100))
        assert np.allclose(a, b), "same seed must reproduce the same matrix"
        assert not np.allclose(a, c), "different seed must give a different matrix"

    def test_default_rng_runs_without_explicit_generator(self):
        """rng=None path works (uses default_rng) and returns the right shape."""
        W = init_he(16, 9)
        assert W.shape == (16, 9)
        assert np.all(np.isfinite(W))


class TestDeadNetworkExperiment:
    """
    The pedagogical payoff: chain a deep stack of linear+ReLU layers and watch
    the pre-activation std stay alive with He init but collapse to exactly 0
    with all-zero weights.
    """

    def _deep_forward_std(self, W_list, x):
        """Run x through len(W_list) linear+ReLU layers; return std of final pre-acts."""
        h = x
        for k, W in enumerate(W_list):
            z = linear(h, W, np.zeros(W.shape[1]))   # pre-activation
            if k == len(W_list) - 1:
                return float(z.std())                # std BEFORE the last ReLU
            h = relu(z)
        return float(h.std())

    def test_he_init_keeps_network_alive(self):
        """10 layers, He init: final pre-activation std stays > 0.1 (signal survives)."""
        rng = np.random.default_rng(2024)
        N, width, depth = 256, 64, 10
        x = rng.standard_normal((N, width))
        W_list = [init_he(width, width, rng=rng) for _ in range(depth)]
        std = self._deep_forward_std(W_list, x)
        assert std > 0.1, f"He-init network is dead: final std={std:.4f}"
        assert np.isfinite(std)

    def test_zero_init_kills_network(self):
        """10 layers, zero weights: final pre-activation std is exactly 0 (dead)."""
        rng = np.random.default_rng(2025)
        N, width, depth = 256, 64, 10
        x = rng.standard_normal((N, width))
        W_list = [np.zeros((width, width)) for _ in range(depth)]
        std = self._deep_forward_std(W_list, x)
        assert std == 0.0, f"zero-init network should be dead, got std={std:.4f}"

    def test_he_alive_and_zero_dead_side_by_side(self):
        """Same input: He stays alive, zero collapses — the contrast is the lesson."""
        rng = np.random.default_rng(7)
        N, width, depth = 128, 48, 10
        x = rng.standard_normal((N, width))
        he = self._deep_forward_std(
            [init_he(width, width, rng=rng) for _ in range(depth)], x
        )
        dead = self._deep_forward_std(
            [np.zeros((width, width)) for _ in range(depth)], x
        )
        assert he > 0.1 and dead == 0.0 and he > dead
