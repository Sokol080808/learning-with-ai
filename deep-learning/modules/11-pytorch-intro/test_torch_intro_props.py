# Randomised / property-based tests for module 11 — PyTorch intro.
#
# Rules enforced here:
#   • torch.manual_seed() set at the top of every test (seed varies per test)
#   • All module calls happen INSIDE test functions  (import-time: safe on stub branch)
#   • Only numpy / torch / pytest / stdlib

import math

import numpy as np
import pytest
import torch

from torch_intro import to_tensor, grad_of, linear_forward, mse_torch


# ===========================================================================
# to_tensor — dtype, shape, values, various input kinds
# ===========================================================================

class TestToTensorRandomised:
    """Always float32 regardless of input dtype/kind; shape and values preserved."""

    def test_float32_from_int_list_various_lengths(self):
        """Outputs float32 for integer lists of lengths 1..20."""
        torch.manual_seed(10)
        np.random.seed(10)
        for n in range(1, 21):
            data = np.random.randint(-100, 100, size=n).tolist()
            t = to_tensor(data)
            assert t.dtype == torch.float32, f"failed for n={n}"
            assert tuple(t.shape) == (n,)
            # values must match
            expected = torch.tensor(data, dtype=torch.float32)
            assert torch.allclose(t, expected), f"values mismatch for n={n}"

    def test_float32_from_numpy_int32(self):
        """numpy int32 array → float32 tensor with correct values."""
        torch.manual_seed(11)
        np.random.seed(11)
        arr = np.random.randint(-50, 50, size=(4, 5)).astype(np.int32)
        t = to_tensor(arr)
        assert t.dtype == torch.float32
        assert tuple(t.shape) == (4, 5)
        assert torch.allclose(t, torch.tensor(arr, dtype=torch.float32))

    def test_float32_from_numpy_float64(self):
        """numpy float64 array → float32 tensor; values close (within float32 precision)."""
        torch.manual_seed(12)
        np.random.seed(12)
        arr = np.random.randn(3, 3).astype(np.float64)
        t = to_tensor(arr)
        assert t.dtype == torch.float32
        # float32 has ~7 decimal digits; atol=1e-5 is safe
        assert torch.allclose(t, torch.tensor(arr, dtype=torch.float32), atol=1e-5)

    def test_float32_from_existing_float32_tensor(self):
        """Passing a float32 tensor returns float32 (no-op / view)."""
        torch.manual_seed(13)
        src = torch.randn(6, dtype=torch.float32)
        t = to_tensor(src)
        assert t.dtype == torch.float32
        assert tuple(t.shape) == (6,)

    def test_float32_from_existing_float64_tensor(self):
        """Passing a float64 tensor must still return float32."""
        torch.manual_seed(14)
        src = torch.randn(5, dtype=torch.float64)
        t = to_tensor(src)
        assert t.dtype == torch.float32

    def test_shape_preserved_for_various_2d_sizes(self):
        """Shape (N, D) is preserved exactly for several random sizes."""
        torch.manual_seed(15)
        np.random.seed(15)
        for _ in range(5):
            N = np.random.randint(1, 10)
            D = np.random.randint(1, 10)
            data = np.random.randn(N, D).astype(np.float32)
            t = to_tensor(data)
            assert tuple(t.shape) == (N, D), f"shape mismatch: expected ({N},{D})"

    def test_single_element_list(self):
        """Edge: list of one element → shape (1,) float32."""
        t = to_tensor([42])
        assert t.dtype == torch.float32
        assert tuple(t.shape) == (1,)
        assert torch.allclose(t, torch.tensor([42.0]))

    def test_negative_and_zero_values_preserved(self):
        """Values including negatives and zero survive the conversion exactly."""
        data = [-3.0, 0.0, 1.5, -0.25]
        t = to_tensor(data)
        assert torch.allclose(t, torch.tensor(data, dtype=torch.float32))

    def test_large_magnitude_values(self):
        """Large magnitudes don't overflow float32 (within its ~3.4e38 range)."""
        data = [1e30, -1e30, 0.0]
        t = to_tensor(data)
        assert t.dtype == torch.float32
        assert torch.isfinite(t).all()

    def test_on_cpu(self):
        """Output always lives on CPU."""
        torch.manual_seed(16)
        data = torch.randn(8).tolist()
        t = to_tensor(data)
        assert t.device.type == "cpu"


# ===========================================================================
# grad_of — correctness against analytic derivatives
# ===========================================================================

class TestGradOfRandomised:
    """grad_of must match the analytic gradient for several functions and random points."""

    def test_sum_of_squares_multiple_random_inputs(self):
        """f(x) = sum(x²)  →  df/dx = 2x; tested at 10 random points."""
        torch.manual_seed(20)
        for _ in range(10):
            x = to_tensor(torch.randn(np.random.randint(2, 10)).tolist())
            g = grad_of(lambda t: (t ** 2).sum(), x)
            assert g.shape == x.shape
            assert torch.allclose(g, 2.0 * x, atol=1e-5), f"failed at x={x}"

    def test_cubic_analytic_gradient(self):
        """f(x) = sum(x³)  →  df/dx = 3x²; several random points."""
        torch.manual_seed(21)
        for _ in range(8):
            x = to_tensor(torch.randn(5).tolist())
            g = grad_of(lambda t: (t ** 3).sum(), x)
            expected = 3.0 * x ** 2
            assert torch.allclose(g, expected, atol=1e-5), f"cubic failed at x={x}"

    def test_affine_function_gradient_is_constant(self):
        """f(x) = sum(a*x + b)  →  df/dx = a (constant vector); 5 random (a,b) pairs."""
        torch.manual_seed(22)
        for _ in range(5):
            a = float(torch.randn(1).item())
            b = float(torch.randn(1).item())
            x = to_tensor(torch.randn(6).tolist())
            g = grad_of(lambda t: (a * t + b).sum(), x)
            expected = torch.full_like(x, a)
            assert torch.allclose(g, expected, atol=1e-5), \
                f"affine failed: a={a:.3f}, b={b:.3f}, x={x}"

    def test_gradient_shape_equals_input_shape_various_sizes(self):
        """Gradient always has the same shape as x, for random 1-D sizes 1..15."""
        torch.manual_seed(23)
        for n in [1, 2, 5, 10, 15]:
            x = to_tensor(torch.randn(n).tolist())
            g = grad_of(lambda t: (t ** 2).sum(), x)
            assert g.shape == x.shape, f"shape mismatch for n={n}"

    def test_does_not_mutate_input_requires_grad(self):
        """grad_of must not leave requires_grad=True on x."""
        torch.manual_seed(24)
        x = to_tensor([1.0, -2.0, 3.0])
        _ = grad_of(lambda t: (t ** 2).sum(), x)
        assert x.requires_grad is False
        assert x.grad is None

    def test_gradient_is_float32(self):
        """Returned gradient tensor is float32."""
        torch.manual_seed(25)
        x = to_tensor([1.0, 2.0, 3.0])
        g = grad_of(lambda t: (t ** 2).sum(), x)
        assert g.dtype == torch.float32

    def test_gradient_of_dot_product(self):
        """f(x) = x · c (fixed c)  →  df/dx = c; analytic oracle."""
        torch.manual_seed(26)
        c = torch.randn(4, dtype=torch.float32)
        x = to_tensor(torch.randn(4).tolist())
        g = grad_of(lambda t: (t * c).sum(), x)
        assert torch.allclose(g, c, atol=1e-5)

    def test_gradient_near_zero(self):
        """Edge: x close to zero; f(x)=sum(x²) → grad ≈ 0."""
        torch.manual_seed(27)
        x = to_tensor([1e-7, -1e-7, 0.0])
        g = grad_of(lambda t: (t ** 2).sum(), x)
        expected = 2.0 * x
        assert torch.allclose(g, expected, atol=1e-10)

    def test_gradient_large_magnitude_no_overflow(self):
        """Edge: large x values; gradient must be finite."""
        torch.manual_seed(28)
        x = to_tensor([1e3, -1e3, 5e2])
        g = grad_of(lambda t: (t ** 2).sum(), x)
        assert torch.isfinite(g).all()
        assert torch.allclose(g, 2.0 * x, atol=1.0)  # float32 precision at 1e3

    def test_gradient_single_element(self):
        """Edge: single-element tensor x; grad shape must be (1,)."""
        torch.manual_seed(29)
        x = to_tensor([3.0])
        g = grad_of(lambda t: (t ** 2).sum(), x)
        assert tuple(g.shape) == (1,)
        assert torch.allclose(g, torch.tensor([6.0]), atol=1e-5)


# ===========================================================================
# linear_forward — shape algebra, values, bias broadcasting, gradient flow
# ===========================================================================

class TestLinearForwardRandomised:
    """linear_forward(x, W, b) must satisfy x@W+b contract for random shapes/values."""

    def test_output_shape_various_ndd(self):
        """Output shape is exactly (N, D_out) for random N, D_in, D_out."""
        torch.manual_seed(30)
        configs = [(1, 1, 1), (2, 3, 4), (7, 5, 1), (1, 8, 8), (16, 32, 16)]
        for N, D_in, D_out in configs:
            x = torch.randn(N, D_in)
            W = torch.randn(D_in, D_out)
            b = torch.randn(D_out)
            out = linear_forward(x, W, b)
            assert tuple(out.shape) == (N, D_out), \
                f"shape failed for ({N},{D_in},{D_out})"

    def test_values_match_torch_matmul_plus_bias(self):
        """Values equal x @ W + b (torch oracle) for 8 random configurations."""
        torch.manual_seed(31)
        for _ in range(8):
            N = np.random.randint(1, 10)
            D_in = np.random.randint(1, 10)
            D_out = np.random.randint(1, 10)
            x = torch.randn(N, D_in)
            W = torch.randn(D_in, D_out)
            b = torch.randn(D_out)
            out = linear_forward(x, W, b)
            expected = x @ W + b
            assert torch.allclose(out, expected, atol=1e-5), \
                f"values mismatch for ({N},{D_in},{D_out})"

    def test_bias_is_added_to_every_row(self):
        """When W=0, every row of output equals b (pure bias broadcast)."""
        torch.manual_seed(32)
        for N in [1, 3, 7]:
            D_in, D_out = 4, 5
            x = torch.randn(N, D_in)
            W = torch.zeros(D_in, D_out)
            b = torch.randn(D_out)
            out = linear_forward(x, W, b)
            for row in range(N):
                assert torch.allclose(out[row], b, atol=1e-6), \
                    f"row {row} != b for N={N}"

    def test_zero_bias_matches_pure_matmul(self):
        """When b=0, output equals x @ W exactly."""
        torch.manual_seed(33)
        N, D_in, D_out = 4, 3, 5
        x = torch.randn(N, D_in)
        W = torch.randn(D_in, D_out)
        b = torch.zeros(D_out)
        out = linear_forward(x, W, b)
        assert torch.allclose(out, x @ W, atol=1e-6)

    def test_single_sample(self):
        """Edge: N=1; shape must be (1, D_out)."""
        torch.manual_seed(34)
        x = torch.randn(1, 6)
        W = torch.randn(6, 3)
        b = torch.randn(3)
        out = linear_forward(x, W, b)
        assert tuple(out.shape) == (1, 3)
        assert torch.allclose(out, x @ W + b, atol=1e-6)

    def test_single_feature_in_and_out(self):
        """Edge: D_in=1, D_out=1 — effectively a scalar linear transform per sample."""
        torch.manual_seed(35)
        N = 5
        x = torch.randn(N, 1)
        W = torch.randn(1, 1)
        b = torch.randn(1)
        out = linear_forward(x, W, b)
        assert tuple(out.shape) == (N, 1)
        assert torch.allclose(out, x @ W + b, atol=1e-6)

    def test_output_dtype_is_float32(self):
        """Output dtype stays float32."""
        torch.manual_seed(36)
        x = torch.randn(3, 4, dtype=torch.float32)
        W = torch.randn(4, 5, dtype=torch.float32)
        b = torch.randn(5, dtype=torch.float32)
        out = linear_forward(x, W, b)
        assert out.dtype == torch.float32

    def test_gradients_flow_through_all_inputs(self):
        """After a scalar loss backward, W, b, x all receive non-None, nonzero gradients."""
        torch.manual_seed(37)
        N, D_in, D_out = 3, 4, 2
        x = torch.randn(N, D_in, requires_grad=True)
        W = torch.randn(D_in, D_out, requires_grad=True)
        b = torch.randn(D_out, requires_grad=True)
        out = linear_forward(x, W, b)
        loss = out.sum()
        loss.backward()
        assert x.grad is not None and x.grad.abs().sum() > 0, "x has no gradient"
        assert W.grad is not None and W.grad.abs().sum() > 0, "W has no gradient"
        assert b.grad is not None and b.grad.abs().sum() > 0, "b has no gradient"

    def test_linearity_in_x(self):
        """f(alpha*x1 + beta*x2) == alpha*f(x1) + beta*f(x2) when b=0 (linearity)."""
        torch.manual_seed(38)
        N, D_in, D_out = 4, 3, 3
        x1 = torch.randn(N, D_in)
        x2 = torch.randn(N, D_in)
        W = torch.randn(D_in, D_out)
        b = torch.zeros(D_out)
        alpha, beta = 2.3, -1.7
        out_combo = linear_forward(alpha * x1 + beta * x2, W, b)
        out_lin = alpha * linear_forward(x1, W, b) + beta * linear_forward(x2, W, b)
        assert torch.allclose(out_combo, out_lin, atol=1e-5)

    def test_identity_weight_copies_input_columns(self):
        """W = I_D (square), b = 0 → output equals x exactly."""
        torch.manual_seed(39)
        D = 5
        x = torch.randn(4, D)
        W = torch.eye(D)
        b = torch.zeros(D)
        out = linear_forward(x, W, b)
        assert torch.allclose(out, x, atol=1e-6)


# ===========================================================================
# mse_torch — scalar output, non-negativity, symmetry, backward, numerical
# ===========================================================================

class TestMseTorchRandomised:
    """mse_torch(pred, y) = mean((pred-y)²); scalar; non-negative; symmetric; differentiable."""

    def test_output_is_scalar_various_shapes(self):
        """Shape is () for 1-D and 2-D inputs of various sizes."""
        torch.manual_seed(40)
        for shape in [(1,), (5,), (20,), (3, 4), (7, 2)]:
            pred = torch.randn(*shape)
            y = torch.randn(*shape)
            loss = mse_torch(pred, y)
            assert loss.shape == torch.Size([]), f"not scalar for shape {shape}"

    def test_non_negative_random_inputs(self):
        """MSE is always >= 0 for 15 random (pred, y) pairs."""
        torch.manual_seed(41)
        for _ in range(15):
            n = np.random.randint(2, 30)
            pred = torch.randn(n)
            y = torch.randn(n)
            loss = mse_torch(pred, y)
            assert loss.item() >= 0.0, f"negative MSE: {loss.item()}"

    def test_symmetry_in_pred_and_y(self):
        """mse_torch(pred, y) == mse_torch(y, pred) for 10 random pairs."""
        torch.manual_seed(42)
        for _ in range(10):
            n = np.random.randint(2, 20)
            pred = torch.randn(n)
            y = torch.randn(n)
            assert torch.allclose(mse_torch(pred, y), mse_torch(y, pred), atol=1e-6)

    def test_zero_when_identical(self):
        """MSE is 0 when pred == y, for several random vectors."""
        torch.manual_seed(43)
        for _ in range(8):
            n = np.random.randint(1, 20)
            v = torch.randn(n)
            loss = mse_torch(v, v)
            assert torch.allclose(loss, torch.tensor(0.0), atol=1e-7), \
                f"non-zero loss when pred==y: {loss.item()}"

    def test_known_analytic_values(self):
        """Cross-check against hand-computed MSE values."""
        torch.manual_seed(44)
        cases = [
            # (pred, y, expected)
            ([0.0], [1.0], 1.0),
            ([0.0, 0.0], [1.0, 1.0], 1.0),
            ([1.0, 2.0, 3.0], [4.0, 5.0, 6.0], 9.0),  # diffs [-3,-3,-3], sq 9, mean 9
            ([-1.0, 1.0], [1.0, -1.0], 4.0),           # diffs [-2,2], sq [4,4], mean 4
        ]
        for pred_l, y_l, expected in cases:
            pred = to_tensor(pred_l)
            y = to_tensor(y_l)
            loss = mse_torch(pred, y)
            assert torch.allclose(loss, torch.tensor(expected, dtype=torch.float32), atol=1e-5), \
                f"analytic check failed: pred={pred_l}, y={y_l}, got {loss.item()}, want {expected}"

    def test_matches_torch_nn_mse_loss(self):
        """mse_torch must equal torch.nn.functional.mse_loss for 10 random inputs."""
        torch.manual_seed(45)
        import torch.nn.functional as F
        for _ in range(10):
            n = np.random.randint(2, 50)
            pred = torch.randn(n)
            y = torch.randn(n)
            got = mse_torch(pred, y)
            ref = F.mse_loss(pred, y)
            assert torch.allclose(got, ref, atol=1e-5), \
                f"diverges from F.mse_loss: got={got.item():.6f}, ref={ref.item():.6f}"

    def test_gradient_flows_through_pred(self):
        """backward() must give pred.grad with the correct formula: 2*(pred-y)/n."""
        torch.manual_seed(46)
        n = 8
        pred = torch.randn(n, requires_grad=True)
        y = torch.randn(n)
        loss = mse_torch(pred, y)
        loss.backward()
        expected_grad = 2.0 * (pred.detach() - y) / n
        assert pred.grad is not None
        assert torch.allclose(pred.grad, expected_grad, atol=1e-5)

    def test_scaling_property(self):
        """mse_torch(c*pred, c*y) == c² * mse_torch(pred, y) for scalar c."""
        torch.manual_seed(47)
        for c in [2.0, 0.5, -3.0, 10.0]:
            pred = torch.randn(10)
            y = torch.randn(10)
            got = mse_torch(c * pred, c * y)
            expected = (c ** 2) * mse_torch(pred, y)
            assert torch.allclose(got, expected, atol=1e-4), \
                f"scaling failed for c={c}: got={got.item()}, expected={expected.item()}"

    def test_large_magnitude_numerical_stability(self):
        """Edge: large-magnitude inputs; result must be finite and positive."""
        torch.manual_seed(48)
        pred = torch.tensor([1e3, -1e3, 5e2], dtype=torch.float32)
        y = torch.zeros(3)
        loss = mse_torch(pred, y)
        assert torch.isfinite(loss)
        assert loss.item() > 0.0

    def test_single_element(self):
        """Edge: single-element tensors; MSE == (pred-y)²."""
        torch.manual_seed(49)
        pred = to_tensor([3.0])
        y = to_tensor([1.0])
        loss = mse_torch(pred, y)
        assert torch.allclose(loss, torch.tensor(4.0), atol=1e-6)


# ===========================================================================
# Integration — all four functions together in a gradient-descent loop
# ===========================================================================

class TestIntegration:
    """End-to-end: linear_forward + mse_torch + grad_of must converge on random tasks."""

    def test_training_step_decreases_loss(self):
        """A single gradient step with lr=0.1 must decrease MSE on a random linear task."""
        torch.manual_seed(50)
        N, D_in, D_out = 8, 4, 2
        x = torch.randn(N, D_in)
        W_true = torch.randn(D_in, D_out)
        b_true = torch.randn(D_out)
        y = linear_forward(x, W_true, b_true)

        W = torch.randn(D_in, D_out)
        b = torch.randn(D_out)
        params = torch.cat([W.reshape(-1), b.reshape(-1)])

        def loss_fn(p):
            Wp = p[:D_in * D_out].reshape(D_in, D_out)
            bp = p[D_in * D_out:]
            return mse_torch(linear_forward(x, Wp, bp), y)

        loss_before = loss_fn(params).item()
        g = grad_of(loss_fn, params)
        params_after = params - 0.1 * g
        loss_after = loss_fn(params_after).item()

        assert loss_after < loss_before, \
            f"loss did NOT decrease: before={loss_before:.4f}, after={loss_after:.4f}"

    def test_convergence_to_near_zero_on_three_seeds(self):
        """100 gradient steps with lr=0.05 must reduce loss to < 0.01 on three random seeds."""
        for seed in [60, 61, 62]:
            torch.manual_seed(seed)
            N, D_in, D_out = 16, 3, 1
            x = torch.randn(N, D_in)
            W_true = torch.randn(D_in, D_out)
            b_true = torch.randn(D_out)
            y = linear_forward(x, W_true, b_true)

            params = torch.zeros(D_in * D_out + D_out)

            def loss_fn(p):
                Wp = p[:D_in * D_out].reshape(D_in, D_out)
                bp = p[D_in * D_out:]
                return mse_torch(linear_forward(x, Wp, bp), y)

            lr = 0.05
            for _ in range(100):
                g = grad_of(loss_fn, params)
                params = params - lr * g

            final_loss = loss_fn(params).item()
            assert final_loss < 0.01, \
                f"seed {seed}: did not converge, final_loss={final_loss:.6f}"

    def test_grad_of_mse_wrt_pred_is_nonzero(self):
        """Gradient of MSE w.r.t. pred is non-zero when pred != y."""
        torch.manual_seed(70)
        pred = to_tensor([1.0, 2.0, 3.0])
        y = to_tensor([0.0, 0.0, 0.0])
        g = grad_of(lambda p: mse_torch(p, y), pred)
        assert g.shape == pred.shape
        assert g.abs().sum().item() > 0.0

    def test_to_tensor_output_flows_through_linear_and_mse(self):
        """to_tensor output can be fed into linear_forward and mse_torch without dtype errors."""
        torch.manual_seed(80)
        raw_x = [[1, 2, 3], [4, 5, 6]]      # ints — to_tensor must fix dtype
        x = to_tensor(raw_x)                  # (2, 3) float32
        W = torch.randn(3, 2)
        b = torch.randn(2)
        pred = linear_forward(x, W, b)        # (2, 2)
        y = torch.zeros(2, 2)
        loss = mse_torch(pred, y)
        assert loss.dtype == torch.float32
        assert loss.item() >= 0.0
