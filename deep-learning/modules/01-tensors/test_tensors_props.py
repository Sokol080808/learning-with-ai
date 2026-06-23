# Property-based and oracle tests for tensors.py
#
# Each test is fully self-contained (no module-level solution calls), so this
# file still imports cleanly against an unfilled stub on the main branch.
# Oracles: PyTorch equivalents or closed-form numpy references.
# Seeds vary per test so random inputs are independent.

import numpy as np
import torch
import pytest

from tensors import scale, row_normalize, add_bias, relu_np, standardize


# ===================================================================== scale

class TestScaleProps:
    """Randomised + oracle tests for scale()."""

    def test_scale_vs_torch_several_shapes(self):
        """scale(x, k) must match x*k for varied shapes and scalars."""
        np.random.seed(10)
        torch.manual_seed(10)
        shapes = [(1,), (5,), (3, 4), (2, 3, 4), (1, 1)]
        ks = [0.5, -2.0, 1e6, 1e-6, 3.14159]
        for shape in shapes:
            for k in ks:
                x = np.random.randn(*shape).astype(np.float64)
                xt = torch.from_numpy(x)
                expected = (xt * k).numpy()
                got = scale(x, k)
                assert got.shape == x.shape, f"shape mismatch for shape={shape}, k={k}"
                assert np.allclose(got, expected, atol=1e-12), (
                    f"value mismatch for shape={shape}, k={k}"
                )

    def test_scale_does_not_share_memory_with_input(self):
        """scale must return a new array; mutating the result must not affect input."""
        np.random.seed(11)
        x = np.random.randn(4, 4)
        before = x.copy()
        out = scale(x, 2.0)
        out[:] = 999.0
        assert np.allclose(x, before), "scale() output shares memory with input"

    def test_scale_identity_k_one(self):
        """scale(x, 1.0) must produce values identical to x (not necessarily same object)."""
        np.random.seed(12)
        x = np.random.randn(6, 7)
        out = scale(x, 1.0)
        assert out.shape == x.shape
        assert np.allclose(out, x, atol=1e-15)

    def test_scale_negation(self):
        """scale(x, -1) must flip all signs; two applications must recover original."""
        np.random.seed(13)
        x = np.random.randn(5, 5)
        neg = scale(x, -1.0)
        recovered = scale(neg, -1.0)
        assert np.allclose(recovered, x, atol=1e-15)

    def test_scale_large_magnitude_no_inf(self):
        """scale with large k on modest x must not produce inf or nan."""
        np.random.seed(14)
        x = np.random.randn(10, 10)  # values in ~[-3, 3]
        out = scale(x, 1e10)
        assert np.all(np.isfinite(out)), "scale produced inf/nan with large k"

    def test_scale_single_element(self):
        """scale on a 1-element array must work correctly."""
        x = np.array([[5.0]])
        out = scale(x, -3.0)
        assert out.shape == (1, 1)
        assert np.isclose(out[0, 0], -15.0)

    def test_scale_preserves_zeros(self):
        """scale(zeros, k) must remain zeros for any k."""
        np.random.seed(15)
        x = np.zeros((4, 6))
        for k in [-100.0, 0.0, 1.0, 1e9]:
            out = scale(x, k)
            assert np.allclose(out, 0.0), f"scale(zeros, {k}) is not zero"


# =============================================================== row_normalize

class TestRowNormalizeProps:
    """Randomised + oracle tests for row_normalize()."""

    def test_rows_sum_to_one_varied_sizes(self):
        """After row_normalize every row sums to 1, across several (N, D) shapes."""
        np.random.seed(20)
        for N, D in [(1, 1), (1, 5), (10, 1), (8, 6), (50, 20)]:
            x = np.random.rand(N, D) + 0.1  # strictly positive rows
            out = row_normalize(x)
            assert out.shape == (N, D), f"shape changed for ({N},{D})"
            sums = out.sum(axis=1)
            assert np.allclose(sums, 1.0, atol=1e-12), (
                f"rows do not sum to 1 for shape ({N},{D}): {sums}"
            )

    def test_row_normalize_vs_numpy_reference(self):
        """row_normalize must match the closed-form numpy formula x/x.sum(axis=1,keepdims=True)."""
        np.random.seed(21)
        for _ in range(8):
            N = np.random.randint(2, 15)
            D = np.random.randint(2, 12)
            x = np.random.rand(N, D) * 5.0 + 0.01
            expected = x / x.sum(axis=1, keepdims=True)
            got = row_normalize(x)
            assert np.allclose(got, expected, atol=1e-12)

    def test_row_normalize_nonnegative_input_stays_in_0_1(self):
        """For non-negative rows, every element of the output must be in [0, 1]."""
        np.random.seed(22)
        x = np.abs(np.random.randn(12, 8)) + 1e-3
        out = row_normalize(x)
        assert np.all(out >= -1e-12), "output has negative values for non-negative input"
        assert np.all(out <= 1.0 + 1e-12), "output has values > 1 for non-negative input"

    def test_row_normalize_output_shape_preserved(self):
        """Shape must be identical to input shape."""
        np.random.seed(23)
        x = np.random.rand(7, 9) + 0.1
        out = row_normalize(x)
        assert out.shape == x.shape

    def test_row_normalize_does_not_mutate_input(self):
        """row_normalize must not modify the input array."""
        np.random.seed(24)
        x = np.random.rand(5, 5) + 0.1
        before = x.copy()
        _ = row_normalize(x)
        assert np.allclose(x, before), "row_normalize mutated the input"

    def test_row_normalize_single_row(self):
        """Single-row matrix: the one row must sum to 1."""
        np.random.seed(25)
        x = (np.random.rand(1, 6) + 0.1).reshape(1, 6)
        out = row_normalize(x)
        assert out.shape == (1, 6)
        assert np.isclose(out.sum(), 1.0, atol=1e-12)

    def test_row_normalize_uniform_row_gives_equal_fractions(self):
        """A row of all-ones of length D must give 1/D in every cell."""
        for D in [1, 2, 5, 10]:
            x = np.ones((3, D))
            out = row_normalize(x)
            assert np.allclose(out, 1.0 / D, atol=1e-12), (
                f"uniform row of length {D} did not give 1/{D}"
            )

    def test_row_normalize_negative_values_row_sum_invariant(self):
        """Even with mixed-sign values (row sum != 0), rows still sum to 1."""
        np.random.seed(26)
        # Build rows whose sum is safely away from zero but not necessarily positive
        x = np.random.randn(6, 5)
        # Shift so each row sum is at least 1
        x = x - x.sum(axis=1, keepdims=True) + 2.0
        out = row_normalize(x)
        assert np.allclose(out.sum(axis=1), 1.0, atol=1e-11)

    def test_row_normalize_large_values_numerical_stability(self):
        """row_normalize must handle rows with large magnitudes without inf/nan."""
        np.random.seed(27)
        x = np.random.rand(4, 6) * 1e8 + 1.0
        out = row_normalize(x)
        assert np.all(np.isfinite(out)), "row_normalize produced inf/nan for large values"
        assert np.allclose(out.sum(axis=1), 1.0, atol=1e-6)


# ================================================================= add_bias

class TestAddBiasProps:
    """Randomised + oracle tests for add_bias()."""

    def test_add_bias_vs_torch_several_shapes(self):
        """add_bias(x, b) must match torch.Tensor broadcasting x + b."""
        np.random.seed(30)
        torch.manual_seed(30)
        for N, D in [(1, 1), (1, 5), (7, 1), (8, 10), (50, 32)]:
            x = np.random.randn(N, D)
            b = np.random.randn(D)
            expected = (torch.from_numpy(x) + torch.from_numpy(b)).numpy()
            got = add_bias(x, b)
            assert got.shape == (N, D), f"shape wrong for ({N},{D})"
            assert np.allclose(got, expected, atol=1e-13), (
                f"value mismatch for shape ({N},{D})"
            )

    def test_add_bias_zero_bias_identity(self):
        """b=0 must leave x unchanged."""
        np.random.seed(31)
        x = np.random.randn(9, 7)
        b = np.zeros(7)
        out = add_bias(x, b)
        assert np.allclose(out, x, atol=1e-15)

    def test_add_bias_bias_added_to_every_row(self):
        """b must be added identically to every row."""
        np.random.seed(32)
        N, D = 10, 5
        x = np.random.randn(N, D)
        b = np.random.randn(D)
        out = add_bias(x, b)
        for i in range(N):
            assert np.allclose(out[i] - x[i], b, atol=1e-13), (
                f"row {i}: out[i]-x[i] != b"
            )

    def test_add_bias_does_not_mutate_input(self):
        """add_bias must not modify x or b."""
        np.random.seed(33)
        x = np.random.randn(4, 3)
        b = np.random.randn(3)
        x_before = x.copy()
        b_before = b.copy()
        _ = add_bias(x, b)
        assert np.allclose(x, x_before), "add_bias mutated x"
        assert np.allclose(b, b_before), "add_bias mutated b"

    def test_add_bias_shape_preserved(self):
        """Output shape must equal input shape (N, D)."""
        np.random.seed(34)
        N, D = 6, 8
        x = np.random.randn(N, D)
        b = np.random.randn(D)
        out = add_bias(x, b)
        assert out.shape == (N, D)

    def test_add_bias_single_row(self):
        """add_bias must work for N=1 (single-row batch)."""
        np.random.seed(35)
        x = np.random.randn(1, 4)
        b = np.array([1.0, -2.0, 3.0, -4.0])
        out = add_bias(x, b)
        assert out.shape == (1, 4)
        assert np.allclose(out[0], x[0] + b, atol=1e-13)

    def test_add_bias_large_magnitude_no_inf(self):
        """add_bias on large-magnitude inputs must not produce inf/nan."""
        np.random.seed(36)
        x = np.random.randn(5, 6) * 1e7
        b = np.random.randn(6) * 1e7
        out = add_bias(x, b)
        assert np.all(np.isfinite(out)), "add_bias produced inf/nan for large values"

    def test_add_bias_negative_bias(self):
        """Negative bias must reduce every row element correctly."""
        np.random.seed(37)
        x = np.random.randn(5, 4)
        b = -np.abs(np.random.randn(4)) * 10  # all negative, large magnitude
        out = add_bias(x, b)
        expected = x + b
        assert np.allclose(out, expected, atol=1e-13)


# =================================================================== relu_np

class TestReLUProps:
    """Randomised + oracle tests for relu_np()."""

    def test_relu_vs_torch_varied_shapes(self):
        """relu_np must match torch.relu element-by-element across varied shapes."""
        np.random.seed(40)
        torch.manual_seed(40)
        shapes = [(1,), (8,), (4, 5), (2, 3, 6), (1, 1), (20, 20)]
        for shape in shapes:
            x = np.random.randn(*shape)
            xt = torch.from_numpy(x)
            expected = torch.relu(xt).numpy()
            got = relu_np(x)
            assert got.shape == x.shape, f"shape mismatch for shape={shape}"
            assert np.allclose(got, expected, atol=1e-15), (
                f"value mismatch for shape={shape}"
            )

    def test_relu_output_always_nonneg(self):
        """relu_np output must have no negative values for any random input."""
        np.random.seed(41)
        for _ in range(10):
            x = np.random.randn(np.random.randint(1, 20), np.random.randint(1, 20))
            out = relu_np(x)
            assert np.all(out >= 0.0), "relu_np produced negative values"

    def test_relu_positive_unchanged(self):
        """Positive entries must pass through unchanged."""
        np.random.seed(42)
        x = np.abs(np.random.randn(8, 8)) + 1e-3  # strictly positive
        out = relu_np(x)
        assert np.allclose(out, x, atol=1e-15), "relu_np altered positive values"

    def test_relu_negative_zeroed(self):
        """Strictly negative entries must become exactly zero."""
        np.random.seed(43)
        x = -(np.abs(np.random.randn(6, 6)) + 1e-3)  # strictly negative
        out = relu_np(x)
        assert np.allclose(out, 0.0, atol=1e-15), "relu_np did not zero negative values"

    def test_relu_does_not_mutate_input(self):
        """relu_np must not modify the input array."""
        np.random.seed(44)
        x = np.random.randn(5, 5)
        before = x.copy()
        _ = relu_np(x)
        assert np.allclose(x, before), "relu_np mutated the input"

    def test_relu_shape_preserved(self):
        """Output shape must equal input shape for any rank."""
        np.random.seed(45)
        for shape in [(3,), (4, 5), (2, 3, 4)]:
            x = np.random.randn(*shape)
            out = relu_np(x)
            assert out.shape == shape, f"shape changed: expected {shape}, got {out.shape}"

    def test_relu_zero_boundary(self):
        """relu_np(0.0) must be exactly 0.0 (boundary case)."""
        x = np.array([0.0, 0.0, 0.0])
        out = relu_np(x)
        assert np.allclose(out, 0.0, atol=1e-15)

    def test_relu_all_zeros_input(self):
        """relu_np on an all-zero array must return all zeros."""
        x = np.zeros((5, 5))
        out = relu_np(x)
        assert np.allclose(out, 0.0)

    def test_relu_large_magnitude_stability(self):
        """relu_np on very large positive and large negative values must stay finite."""
        x = np.array([-1e15, -1.0, 0.0, 1.0, 1e15])
        out = relu_np(x)
        assert np.all(np.isfinite(out)), "relu_np produced inf/nan on extreme inputs"
        assert np.isclose(out[0], 0.0)
        assert np.isclose(out[-1], 1e15)

    def test_relu_single_element(self):
        """relu_np must work on a 1-element array, both negative and positive."""
        assert np.isclose(relu_np(np.array([-5.0]))[0], 0.0)
        assert np.isclose(relu_np(np.array([7.0]))[0], 7.0)

    def test_relu_idempotent(self):
        """Applying relu_np twice must give the same result as once."""
        np.random.seed(46)
        x = np.random.randn(8, 8)
        once = relu_np(x)
        twice = relu_np(once)
        assert np.allclose(once, twice, atol=1e-15), "relu_np is not idempotent"

    def test_relu_mixed_sign_correct_split(self):
        """For mixed-sign input the negative/non-negative split must be exact."""
        np.random.seed(47)
        x = np.random.randn(10, 10)
        out = relu_np(x)
        neg_mask = x < 0
        pos_mask = x >= 0
        assert np.allclose(out[neg_mask], 0.0, atol=1e-15)
        assert np.allclose(out[pos_mask], x[pos_mask], atol=1e-15)


# ================================================================ standardize

class TestStandardizeProps:
    """Randomised + oracle tests for standardize() (per-feature z-score)."""

    def test_standardize_vs_numpy_reference_varied_shapes(self):
        """standardize must match closed-form (x - x.mean(0)) / x.std(0)."""
        np.random.seed(50)
        for N, D in [(2, 1), (3, 4), (8, 6), (20, 3), (50, 10)]:
            # scale/shift each feature so the transform has real work to do;
            # keep N >= 2 so column std is well-defined and away from zero.
            x = np.random.randn(N, D) * (np.random.rand(D) * 5 + 0.5) + np.random.randn(D) * 3
            expected = (x - x.mean(axis=0)) / x.std(axis=0)
            got = standardize(x)
            assert got.shape == (N, D), f"shape changed for ({N},{D})"
            assert np.allclose(got, expected, atol=1e-12), (
                f"value mismatch vs numpy reference for shape ({N},{D})"
            )

    def test_standardize_vs_torch_oracle(self):
        """standardize must match torch's (xt - xt.mean(0)) / xt.std(0, unbiased=False)."""
        np.random.seed(51)
        torch.manual_seed(51)
        for N, D in [(3, 2), (10, 5), (16, 8), (32, 4)]:
            x = np.random.randn(N, D) * 2.0 + 1.0
            xt = torch.from_numpy(x)
            # unbiased=False so torch divides by N (matches numpy's default std)
            expected = ((xt - xt.mean(dim=0)) / xt.std(dim=0, unbiased=False)).numpy()
            got = standardize(x)
            assert np.allclose(got, expected, atol=1e-10), (
                f"value mismatch vs torch oracle for shape ({N},{D})"
            )

    def test_standardize_columns_zero_mean_unit_std(self):
        """Each output column must have mean ~0 and std ~1 (the defining invariant)."""
        np.random.seed(52)
        for N, D in [(5, 1), (12, 7), (40, 4), (100, 10)]:
            x = np.random.randn(N, D) * 7.0 - 3.0
            out = standardize(x)
            assert np.allclose(out.mean(axis=0), 0.0, atol=1e-12), (
                f"column means not ~0 for shape ({N},{D})"
            )
            assert np.allclose(out.std(axis=0), 1.0, atol=1e-12), (
                f"column stds not ~1 for shape ({N},{D})"
            )

    def test_standardize_shape_preserved(self):
        """Output shape must equal input shape (N, D)."""
        np.random.seed(53)
        N, D = 9, 6
        x = np.random.randn(N, D) + 4.0
        out = standardize(x)
        assert out.shape == (N, D)

    def test_standardize_does_not_mutate_input(self):
        """standardize must not modify the input array."""
        np.random.seed(54)
        x = np.random.randn(7, 5) * 3.0 + 2.0
        before = x.copy()
        _ = standardize(x)
        assert np.allclose(x, before), "standardize mutated the input"

    def test_standardize_does_not_share_memory_with_input(self):
        """standardize must return a fresh array; mutating it must not touch input."""
        np.random.seed(55)
        x = np.random.randn(4, 4) + 1.0
        before = x.copy()
        out = standardize(x)
        out[:] = 999.0
        assert np.allclose(x, before), "standardize() output shares memory with input"

    def test_standardize_already_standardized_is_idempotent(self):
        """Standardizing already-standardized data leaves it (numerically) unchanged."""
        np.random.seed(56)
        x = np.random.randn(30, 5) * 4.0 + 6.0
        once = standardize(x)
        twice = standardize(once)
        assert np.allclose(once, twice, atol=1e-10), "standardize is not idempotent on z-scored data"

    def test_standardize_invariant_to_affine_per_column(self):
        """z-score is invariant to per-column scale a>0 and shift c: std(a*x+c) cancels a."""
        np.random.seed(57)
        N, D = 25, 4
        x = np.random.randn(N, D)
        a = np.random.rand(D) * 5 + 0.5       # positive per-column scale
        c = np.random.randn(D) * 10           # per-column shift
        out_x = standardize(x)
        out_affine = standardize(a * x + c)
        assert np.allclose(out_x, out_affine, atol=1e-10), (
            "standardize is not invariant to positive affine per-column transform"
        )

    def test_standardize_large_magnitude_no_inf(self):
        """Large-magnitude (but non-degenerate) features must not produce inf/nan."""
        np.random.seed(58)
        x = np.random.randn(10, 6) * 1e7 + 1e7
        out = standardize(x)
        assert np.all(np.isfinite(out)), "standardize produced inf/nan for large values"
        assert np.allclose(out.mean(axis=0), 0.0, atol=1e-8)
        assert np.allclose(out.std(axis=0), 1.0, atol=1e-8)
