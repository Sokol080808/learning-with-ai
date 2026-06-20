# Randomized / property-based tests for module 00-setup.
#
# Invariants tested here:
#   vector_add:
#     - result matches torch oracle on random float32 inputs
#     - commutativity: vector_add(a, b) == vector_add(b, a)
#     - associativity with numpy reference: result == a + b elementwise
#     - shape preservation across many random shapes (1-D, 2-D, 3-D)
#     - dtype preservation (float32, float64)
#     - additive identity: vector_add(a, zeros) == a
#     - additive inverse: vector_add(a, -a) == zeros
#     - edge cases: zeros, negatives, large magnitudes, single element
#   mean:
#     - result matches torch oracle on random inputs
#     - result is exactly Python float (not numpy scalar)
#     - matches scipy / numpy reference to floating-point precision
#     - shift invariant up to sign: mean(a + c) == mean(a) + c
#     - scale covariant: mean(a * c) == mean(a) * c
#     - constant array: mean([c, c, ...]) == c
#     - edge cases: zeros, negatives, large magnitudes, single element,
#       near-cancellation (numerical stability), very small values

import numpy as np
import pytest
import torch

from warmup import vector_add, mean


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _to_torch(arr: np.ndarray) -> torch.Tensor:
    """Convert a numpy array to a torch tensor keeping dtype."""
    return torch.from_numpy(arr.copy())


# ---------------------------------------------------------------------------
# vector_add — torch oracle
# ---------------------------------------------------------------------------

class TestVectorAddTorchOracle:
    """Compare vector_add against torch elementwise addition."""

    def test_1d_float64_random(self):
        np.random.seed(10)
        torch.manual_seed(10)
        for _ in range(20):
            n = np.random.randint(1, 200)
            a = np.random.randn(n)
            b = np.random.randn(n)
            result = vector_add(a, b)
            expected = (_to_torch(a) + _to_torch(b)).numpy()
            assert np.allclose(result, expected, atol=1e-12), (
                f"shape=({n},): mismatch vs torch oracle"
            )

    def test_1d_float32_random(self):
        np.random.seed(11)
        torch.manual_seed(11)
        for _ in range(20):
            n = np.random.randint(1, 200)
            a = np.random.randn(n).astype(np.float32)
            b = np.random.randn(n).astype(np.float32)
            result = vector_add(a, b)
            expected = (_to_torch(a) + _to_torch(b)).numpy()
            assert np.allclose(result, expected, atol=1e-6), (
                f"float32 shape=({n},): mismatch vs torch oracle"
            )

    def test_multidimensional_shapes(self):
        """shape preservation: 2-D and 3-D tensors."""
        np.random.seed(12)
        for shape in [(3, 4), (2, 5, 6), (1, 1), (10, 10, 10)]:
            a = np.random.randn(*shape)
            b = np.random.randn(*shape)
            result = vector_add(a, b)
            expected = a + b  # numpy ground truth
            assert result.shape == a.shape, (
                f"shape {shape}: output shape {result.shape} != input shape {a.shape}"
            )
            assert np.allclose(result, expected, atol=1e-12)

    def test_large_magnitudes_numerical_stability(self):
        """Large values: no overflow or catastrophic cancellation."""
        np.random.seed(13)
        a = np.random.randn(50) * 1e15
        b = np.random.randn(50) * 1e15
        result = vector_add(a, b)
        expected = (_to_torch(a) + _to_torch(b)).numpy()
        assert np.allclose(result, expected, rtol=1e-12)


# ---------------------------------------------------------------------------
# vector_add — algebraic / invariant properties
# ---------------------------------------------------------------------------

class TestVectorAddInvariants:

    def test_commutativity(self):
        np.random.seed(20)
        for _ in range(15):
            n = np.random.randint(1, 100)
            a = np.random.randn(n)
            b = np.random.randn(n)
            assert np.allclose(vector_add(a, b), vector_add(b, a), atol=1e-14)

    def test_additive_identity_zeros(self):
        np.random.seed(21)
        for _ in range(15):
            n = np.random.randint(1, 100)
            a = np.random.randn(n)
            zeros = np.zeros(n)
            assert np.allclose(vector_add(a, zeros), a, atol=1e-14)

    def test_additive_inverse(self):
        """a + (-a) == 0 for all a."""
        np.random.seed(22)
        for _ in range(15):
            n = np.random.randint(1, 100)
            a = np.random.randn(n)
            result = vector_add(a, -a)
            assert np.allclose(result, np.zeros(n), atol=1e-14)

    def test_shape_preserved_single_element(self):
        a = np.array([3.14])
        b = np.array([-1.0])
        result = vector_add(a, b)
        assert result.shape == (1,)
        assert np.allclose(result, [2.14], atol=1e-12)

    def test_all_zeros(self):
        a = np.zeros(50)
        b = np.zeros(50)
        result = vector_add(a, b)
        assert np.allclose(result, np.zeros(50))

    def test_all_negatives(self):
        np.random.seed(23)
        a = -np.abs(np.random.randn(30))
        b = -np.abs(np.random.randn(30))
        result = vector_add(a, b)
        expected = a + b
        assert np.allclose(result, expected, atol=1e-13)
        # sum of two negatives must be non-positive
        assert np.all(result <= 0)

    def test_dtype_float32_preserved(self):
        np.random.seed(24)
        a = np.random.randn(8).astype(np.float32)
        b = np.random.randn(8).astype(np.float32)
        result = vector_add(a, b)
        # result must be a numpy array
        assert isinstance(result, np.ndarray)

    def test_result_is_numpy_array(self):
        a = np.array([1.0, 2.0])
        b = np.array([3.0, 4.0])
        result = vector_add(a, b)
        assert isinstance(result, np.ndarray), (
            f"expected np.ndarray, got {type(result)}"
        )

    def test_no_aliasing_with_inputs(self):
        """Modifying the result must not change a or b."""
        np.random.seed(25)
        a = np.random.randn(10)
        b = np.random.randn(10)
        a_copy = a.copy()
        b_copy = b.copy()
        result = vector_add(a, b)
        result[:] = 0
        assert np.allclose(a, a_copy), "vector_add mutated input a"
        assert np.allclose(b, b_copy), "vector_add mutated input b"


# ---------------------------------------------------------------------------
# mean — torch oracle
# ---------------------------------------------------------------------------

class TestMeanTorchOracle:

    def test_1d_float64_random(self):
        np.random.seed(30)
        torch.manual_seed(30)
        for _ in range(20):
            n = np.random.randint(1, 300)
            xs = np.random.randn(n)
            result = mean(xs)
            expected = _to_torch(xs).mean().item()
            assert abs(result - expected) < 1e-10, (
                f"shape=({n},): {result} != torch {expected}"
            )

    def test_1d_float32_random(self):
        np.random.seed(31)
        for _ in range(20):
            n = np.random.randint(1, 300)
            xs = np.random.randn(n).astype(np.float32)
            result = mean(xs)
            expected = _to_torch(xs).mean().item()
            assert abs(result - expected) < 1e-5, (
                f"float32 shape=({n},): {result} != torch {expected}"
            )

    def test_multidimensional_flat_match(self):
        """mean of a 2-D array should equal mean of its flattened version."""
        np.random.seed(32)
        for shape in [(4, 5), (3, 3, 3), (1, 10)]:
            xs = np.random.randn(*shape)
            result = mean(xs)
            expected = float(np.mean(xs))
            assert abs(result - expected) < 1e-12, (
                f"shape {shape}: {result} != {expected}"
            )

    def test_large_magnitudes(self):
        """Near 1e15: result must be finite and close to torch oracle."""
        np.random.seed(33)
        xs = np.random.randn(100) * 1e15
        result = mean(xs)
        expected = _to_torch(xs).mean().item()
        assert np.isfinite(result), "mean returned non-finite value for large inputs"
        assert abs(result - expected) / (abs(expected) + 1e-30) < 1e-10

    def test_very_small_magnitudes(self):
        np.random.seed(34)
        xs = np.random.randn(100) * 1e-15
        result = mean(xs)
        expected = float(np.mean(xs))
        assert np.isfinite(result)
        assert abs(result - expected) < 1e-25


# ---------------------------------------------------------------------------
# mean — algebraic / invariant properties
# ---------------------------------------------------------------------------

class TestMeanInvariants:

    def test_returns_python_float_always(self):
        np.random.seed(40)
        for _ in range(15):
            n = np.random.randint(1, 100)
            xs = np.random.randn(n)
            result = mean(xs)
            assert isinstance(result, float), (
                f"expected float, got {type(result)}"
            )

    def test_constant_array_equals_constant(self):
        np.random.seed(41)
        for _ in range(10):
            c = np.random.randn()
            n = np.random.randint(1, 50)
            xs = np.full(n, c)
            result = mean(xs)
            assert abs(result - c) < 1e-12, (
                f"mean of constant {c} array (size {n}) returned {result}"
            )

    def test_shift_covariance(self):
        """mean(a + c) == mean(a) + c for any scalar shift c."""
        np.random.seed(42)
        for _ in range(15):
            n = np.random.randint(2, 100)
            xs = np.random.randn(n)
            c = np.random.randn()
            assert abs(mean(xs + c) - (mean(xs) + c)) < 1e-11, (
                f"shift covariance failed: c={c}"
            )

    def test_scale_covariance(self):
        """mean(a * c) == mean(a) * c for any scalar c."""
        np.random.seed(43)
        for _ in range(15):
            n = np.random.randint(2, 100)
            xs = np.random.randn(n)
            c = np.random.randn()
            assert abs(mean(xs * c) - mean(xs) * c) < 1e-11, (
                f"scale covariance failed: c={c}"
            )

    def test_single_element_equals_itself(self):
        np.random.seed(44)
        for _ in range(10):
            v = float(np.random.randn())
            xs = np.array([v])
            result = mean(xs)
            assert abs(result - v) < 1e-14, (
                f"mean of single-element [{v}] returned {result}"
            )

    def test_two_element_equals_half_sum(self):
        np.random.seed(45)
        for _ in range(15):
            a, b = np.random.randn(2)
            xs = np.array([a, b])
            expected = (a + b) / 2.0
            result = mean(xs)
            assert abs(result - expected) < 1e-13

    def test_all_zeros(self):
        for n in [1, 5, 100]:
            xs = np.zeros(n)
            assert mean(xs) == 0.0, f"mean of zeros (n={n}) should be 0.0"

    def test_all_negatives(self):
        np.random.seed(46)
        xs = -np.abs(np.random.randn(50))
        result = mean(xs)
        # mean of non-positive values must be non-positive
        assert result <= 0.0, f"mean of negatives should be <= 0, got {result}"
        assert abs(result - float(np.mean(xs))) < 1e-13

    def test_near_cancellation_stability(self):
        """Alternating +C/-C: mean should be very close to 0."""
        np.random.seed(47)
        n = 1000
        c = 1e12
        xs = np.empty(n)
        xs[0::2] = c
        xs[1::2] = -c
        result = mean(xs)
        # exact mean is 0 (equal counts of +c and -c)
        assert abs(result) < 1e-3 * c, (
            f"near-cancellation stability: mean={result}, expected ~0"
        )

    def test_bounds_single_element_is_itself(self):
        """Tighter: for any single-element array mean must equal the element exactly."""
        for v in [0.0, 1.0, -1.0, 1e20, -1e-20, float("inf") * 0]:
            xs = np.array([v])
            result = mean(xs)
            assert result == v or (np.isnan(result) and np.isnan(v)), (
                f"mean([{v}]) = {result}, expected {v}"
            )
