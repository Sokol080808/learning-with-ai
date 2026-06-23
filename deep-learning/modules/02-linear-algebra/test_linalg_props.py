# Randomised property-based tests for deep-learning/modules/02-linear-algebra.
#
# Design principles:
#   - Every test has a deterministic but unique seed so failures are reproducible.
#   - All calls to the module happen INSIDE test functions — no module-level
#     solution calls — so the file imports cleanly against stubs that raise
#     NotImplementedError, and only fails at runtime (red on main, green on
#     reference).
#   - PyTorch is used as the oracle wherever a direct torch equivalent exists.
#   - Finite-difference gradient checks guard every numeric derivative.
#   - Invariants (range, non-negativity, symmetry, shape, python-float return
#     type) are verified independently of the oracle.

import numpy as np
import pytest
import torch
import torch.nn.functional as F

from linalg import dot, matmul, l2_norm, cosine_similarity, linear_forward


# ===========================================================================
# Helpers
# ===========================================================================

def _fd_gradient(f, x, eps=1e-5):
    """Central finite-difference gradient of scalar f w.r.t. 1-D array x."""
    grad = np.zeros_like(x, dtype=float)
    for i in range(len(x)):
        xp = x.copy(); xp[i] += eps
        xm = x.copy(); xm[i] -= eps
        grad[i] = (f(xp) - f(xm)) / (2 * eps)
    return grad


# ===========================================================================
# dot — oracle: torch.dot
# ===========================================================================

class TestDotOracle:
    """Compare dot(a, b) against torch.dot for random inputs."""

    def test_dot_vs_torch_small_vectors(self):
        np.random.seed(100); torch.manual_seed(100)
        for _ in range(30):
            n = np.random.randint(1, 10)
            a = np.random.randn(n)
            b = np.random.randn(n)
            ta = torch.tensor(a, dtype=torch.float64)
            tb = torch.tensor(b, dtype=torch.float64)
            expected = torch.dot(ta, tb).item()
            assert np.isclose(dot(a, b), expected, atol=1e-9), (
                f"dot mismatch: got {dot(a, b)}, expected {expected}")

    def test_dot_vs_torch_larger_vectors(self):
        np.random.seed(101); torch.manual_seed(101)
        for _ in range(10):
            n = np.random.randint(50, 200)
            a = np.random.randn(n)
            b = np.random.randn(n)
            ta = torch.tensor(a, dtype=torch.float64)
            tb = torch.tensor(b, dtype=torch.float64)
            expected = torch.dot(ta, tb).item()
            assert np.isclose(dot(a, b), expected, atol=1e-7)

    def test_dot_returns_float_always(self):
        np.random.seed(102)
        for _ in range(20):
            n = np.random.randint(1, 8)
            a = np.random.randn(n)
            b = np.random.randn(n)
            assert type(dot(a, b)) is float

    def test_dot_commutativity(self):
        """dot(a, b) == dot(b, a) always."""
        np.random.seed(103)
        for _ in range(20):
            n = np.random.randint(1, 12)
            a = np.random.randn(n)
            b = np.random.randn(n)
            assert np.isclose(dot(a, b), dot(b, a), atol=1e-12)

    def test_dot_linearity_in_first_arg(self):
        """dot(alpha*a + beta*c, b) == alpha*dot(a,b) + beta*dot(c,b)."""
        np.random.seed(104)
        for _ in range(15):
            n = np.random.randint(2, 8)
            alpha, beta = np.random.randn(2)
            a = np.random.randn(n)
            b = np.random.randn(n)
            c = np.random.randn(n)
            lhs = dot(alpha * a + beta * c, b)
            rhs = alpha * dot(a, b) + beta * dot(c, b)
            assert np.isclose(lhs, rhs, atol=1e-9)

    # --- edge cases ---

    def test_dot_single_element(self):
        np.random.seed(105)
        for _ in range(10):
            x = np.random.randn(1)
            y = np.random.randn(1)
            assert np.isclose(dot(x, y), x[0] * y[0], atol=1e-12)

    def test_dot_zeros(self):
        np.random.seed(106)
        n = 7
        a = np.random.randn(n)
        z = np.zeros(n)
        assert dot(a, z) == 0.0
        assert dot(z, z) == 0.0

    def test_dot_negative_values(self):
        a = np.array([-1.0, -2.0, -3.0])
        b = np.array([1.0, 2.0, 3.0])
        # -1-4-9 = -14
        assert np.isclose(dot(a, b), -14.0, atol=1e-12)

    def test_dot_large_magnitude_numerical_stability(self):
        """With large values the result should still match numpy."""
        np.random.seed(107)
        n = 20
        a = np.random.randn(n) * 1e8
        b = np.random.randn(n) * 1e8
        expected = float(np.sum(a * b))
        # Allow slightly looser tolerance for very large magnitudes
        assert np.isclose(dot(a, b), expected, rtol=1e-6)

    def test_dot_mismatched_lengths_raises(self):
        np.random.seed(108)
        for la, lb in [(3, 4), (1, 5), (10, 9)]:
            a = np.random.randn(la)
            b = np.random.randn(lb)
            with pytest.raises(ValueError):
                dot(a, b)


# ===========================================================================
# matmul — oracle: torch.mm / torch.matmul
# ===========================================================================

class TestMatmulOracle:

    def test_matmul_vs_torch_random_shapes(self):
        """C = matmul(A, B) must match torch.mm on random (n,k)@(k,m)."""
        np.random.seed(200); torch.manual_seed(200)
        for _ in range(25):
            n = np.random.randint(1, 8)
            k = np.random.randint(1, 8)
            m = np.random.randint(1, 8)
            A = np.random.randn(n, k)
            B = np.random.randn(k, m)
            tA = torch.tensor(A, dtype=torch.float64)
            tB = torch.tensor(B, dtype=torch.float64)
            expected = torch.mm(tA, tB).numpy()
            result = matmul(A, B)
            assert np.allclose(result, expected, atol=1e-9), (
                f"matmul mismatch for shapes ({n},{k})@({k},{m})")

    def test_matmul_output_shape(self):
        np.random.seed(201)
        for _ in range(20):
            n = np.random.randint(1, 10)
            k = np.random.randint(1, 10)
            m = np.random.randint(1, 10)
            A = np.random.randn(n, k)
            B = np.random.randn(k, m)
            C = matmul(A, B)
            assert C.shape == (n, m), f"Expected shape ({n},{m}), got {C.shape}"

    def test_matmul_output_dtype_is_float(self):
        np.random.seed(202)
        A = np.random.randn(3, 4)
        B = np.random.randn(4, 2)
        C = matmul(A, B)
        assert np.issubdtype(C.dtype, np.floating), (
            f"Expected float dtype, got {C.dtype}")

    def test_matmul_associativity(self):
        """(A @ B) @ C == A @ (B @ C)."""
        np.random.seed(203)
        for _ in range(10):
            n, k, m, p = (np.random.randint(1, 5) for _ in range(4))
            A = np.random.randn(n, k)
            B = np.random.randn(k, m)
            C = np.random.randn(m, p)
            lhs = matmul(matmul(A, B), C)
            rhs = matmul(A, matmul(B, C))
            assert np.allclose(lhs, rhs, atol=1e-9)

    def test_matmul_identity(self):
        np.random.seed(204)
        for _ in range(10):
            n = np.random.randint(1, 8)
            A = np.random.randn(n, n)
            I = np.eye(n)
            assert np.allclose(matmul(A, I), A, atol=1e-12)
            assert np.allclose(matmul(I, A), A, atol=1e-12)

    def test_matmul_distributivity_over_addition(self):
        """A @ (B + C) == A@B + A@C."""
        np.random.seed(205)
        n, k, m = 4, 3, 5
        A = np.random.randn(n, k)
        B = np.random.randn(k, m)
        C = np.random.randn(k, m)
        lhs = matmul(A, B + C)
        rhs = matmul(A, B) + matmul(A, C)
        assert np.allclose(lhs, rhs, atol=1e-12)

    def test_matmul_transpose_rule(self):
        """(A @ B)^T == B^T @ A^T."""
        np.random.seed(206)
        for _ in range(10):
            n, k, m = (np.random.randint(1, 6) for _ in range(3))
            A = np.random.randn(n, k)
            B = np.random.randn(k, m)
            lhs = matmul(A, B).T
            rhs = matmul(B.T, A.T)
            assert np.allclose(lhs, rhs, atol=1e-9)

    def test_matmul_batch_as_matrix_product(self):
        """X @ W for a (N,D)@(D,H) DL-style forward pass."""
        np.random.seed(207)
        N, D, H = 16, 8, 4
        X = np.random.randn(N, D)
        W = np.random.randn(D, H)
        result = matmul(X, W)
        assert result.shape == (N, H)
        expected = X @ W   # numpy reference
        assert np.allclose(result, expected, atol=1e-9)

    # --- edge cases ---

    def test_matmul_single_row_column(self):
        """(1,k) @ (k,1) -> (1,1): dot product as matrix product."""
        np.random.seed(208)
        k = 6
        a = np.random.randn(1, k)
        b = np.random.randn(k, 1)
        result = matmul(a, b)
        assert result.shape == (1, 1)
        assert np.isclose(result[0, 0], float(np.sum(a[0] * b[:, 0])), atol=1e-12)

    def test_matmul_zero_matrix(self):
        np.random.seed(209)
        A = np.random.randn(3, 4)
        Z = np.zeros((4, 5))
        result = matmul(A, Z)
        assert np.allclose(result, np.zeros((3, 5)), atol=1e-15)

    def test_matmul_shape_mismatch_raises(self):
        np.random.seed(210)
        for (n, k, k2, m) in [(3, 4, 5, 2), (1, 2, 3, 4), (5, 1, 2, 5)]:
            A = np.random.randn(n, k)
            B = np.random.randn(k2, m)
            with pytest.raises(ValueError):
                matmul(A, B)

    def test_matmul_1x1(self):
        """(1,1) @ (1,1) is just scalar multiplication."""
        np.random.seed(211)
        a = np.random.randn(1, 1)
        b = np.random.randn(1, 1)
        result = matmul(a, b)
        assert result.shape == (1, 1)
        assert np.isclose(result[0, 0], a[0, 0] * b[0, 0], atol=1e-12)


# ===========================================================================
# l2_norm — oracle: torch.norm
# ===========================================================================

class TestL2NormOracle:

    def test_l2_norm_vs_torch_random(self):
        np.random.seed(300); torch.manual_seed(300)
        for _ in range(30):
            n = np.random.randint(1, 15)
            v = np.random.randn(n)
            tv = torch.tensor(v, dtype=torch.float64)
            expected = torch.linalg.norm(tv).item()
            assert np.isclose(l2_norm(v), expected, atol=1e-9)

    def test_l2_norm_returns_float_always(self):
        np.random.seed(301)
        for _ in range(15):
            v = np.random.randn(np.random.randint(1, 8))
            assert type(l2_norm(v)) is float

    def test_l2_norm_nonnegative(self):
        np.random.seed(302)
        for _ in range(30):
            v = np.random.randn(np.random.randint(1, 10))
            assert l2_norm(v) >= 0.0

    def test_l2_norm_scaling(self):
        """||alpha * v|| == |alpha| * ||v||."""
        np.random.seed(303)
        for _ in range(20):
            alpha = np.random.randn()
            v = np.random.randn(np.random.randint(1, 8))
            assert np.isclose(l2_norm(alpha * v), abs(alpha) * l2_norm(v), atol=1e-10)

    def test_l2_norm_triangle_inequality(self):
        """||a + b|| <= ||a|| + ||b||."""
        np.random.seed(304)
        for _ in range(20):
            n = np.random.randint(2, 10)
            a = np.random.randn(n)
            b = np.random.randn(n)
            assert l2_norm(a + b) <= l2_norm(a) + l2_norm(b) + 1e-10

    def test_l2_norm_unit_vector(self):
        """Unit vectors along axes have norm 1."""
        for i, n in [(0, 1), (0, 3), (1, 3), (2, 5)]:
            e = np.zeros(n); e[i] = 1.0
            assert np.isclose(l2_norm(e), 1.0, atol=1e-12)

    def test_l2_norm_pythagorean_triple(self):
        """Several classic Pythagorean triples."""
        for triple, norm in [([3, 4], 5), ([5, 12], 13), ([8, 15], 17)]:
            v = np.array(triple, dtype=float)
            assert np.isclose(l2_norm(v), float(norm), atol=1e-10)

    # --- edge cases ---

    def test_l2_norm_zero_vector(self):
        for n in [1, 3, 10]:
            assert l2_norm(np.zeros(n)) == 0.0

    def test_l2_norm_single_element(self):
        for val in [-5.0, 0.0, 3.14]:
            assert np.isclose(l2_norm(np.array([val])), abs(val), atol=1e-12)

    def test_l2_norm_large_magnitude(self):
        """Should not overflow for large but representable floats."""
        np.random.seed(305)
        v = np.random.randn(10) * 1e10
        expected = float(np.linalg.norm(v))
        assert np.isclose(l2_norm(v), expected, rtol=1e-6)

    def test_l2_norm_gradient_via_finite_difference(self):
        """d/dv ||v|| == v / ||v|| (away from origin)."""
        np.random.seed(306)
        for _ in range(10):
            n = np.random.randint(2, 8)
            v = np.random.randn(n)
            # Ensure far from zero
            if l2_norm(v) < 0.1:
                v = v + 1.0
            analytic_grad = v / l2_norm(v)
            fd_grad = _fd_gradient(l2_norm, v)
            assert np.allclose(analytic_grad, fd_grad, atol=1e-6), (
                f"Gradient check failed: analytic={analytic_grad}, fd={fd_grad}")


# ===========================================================================
# cosine_similarity — oracle: torch.nn.functional.cosine_similarity
# ===========================================================================

class TestCosineSimilarityOracle:

    def test_cosine_vs_torch_random(self):
        """Compare against torch.nn.functional.cosine_similarity (1-D)."""
        np.random.seed(400); torch.manual_seed(400)
        for _ in range(30):
            n = np.random.randint(1, 15)
            a = np.random.randn(n)
            b = np.random.randn(n)
            # torch.nn.functional.cosine_similarity works on batched tensors;
            # reshape to (1, n) and pass dim=1 to get a single value.
            ta = torch.tensor(a, dtype=torch.float64).unsqueeze(0)
            tb = torch.tensor(b, dtype=torch.float64).unsqueeze(0)
            expected = F.cosine_similarity(ta, tb, dim=1, eps=0.0).item()
            result = cosine_similarity(a, b)
            assert np.isclose(result, expected, atol=1e-9), (
                f"cosine mismatch: got {result}, expected {expected}")

    def test_cosine_returns_float_always(self):
        np.random.seed(401)
        for _ in range(15):
            n = np.random.randint(1, 8)
            a = np.random.randn(n) + 0.5  # keep away from zero
            b = np.random.randn(n) + 0.5
            assert type(cosine_similarity(a, b)) is float

    def test_cosine_range_invariant(self):
        """Result is always in [-1, 1]."""
        np.random.seed(402)
        for _ in range(50):
            n = np.random.randint(1, 12)
            a = np.random.randn(n)
            b = np.random.randn(n)
            # Skip zero vectors (they raise ValueError by contract)
            if np.linalg.norm(a) == 0 or np.linalg.norm(b) == 0:
                continue
            c = cosine_similarity(a, b)
            assert -1.0 - 1e-9 <= c <= 1.0 + 1e-9, (
                f"cosine out of range: {c}")

    def test_cosine_symmetry(self):
        """cosine(a, b) == cosine(b, a)."""
        np.random.seed(403)
        for _ in range(20):
            n = np.random.randint(1, 10)
            a = np.random.randn(n) + 0.1
            b = np.random.randn(n) + 0.1
            assert np.isclose(cosine_similarity(a, b), cosine_similarity(b, a), atol=1e-12)

    def test_cosine_scale_invariant(self):
        """cosine_similarity(alpha*a, beta*b) == sign(alpha*beta)*cosine(a,b)."""
        np.random.seed(404)
        for _ in range(20):
            n = np.random.randint(1, 8)
            a = np.random.randn(n) + 0.5
            b = np.random.randn(n) + 0.5
            # Positive scaling: result unchanged
            alpha = np.random.uniform(0.1, 10.0)
            beta = np.random.uniform(0.1, 10.0)
            assert np.isclose(cosine_similarity(alpha * a, beta * b),
                               cosine_similarity(a, b), atol=1e-10)
            # Negating one vector flips sign
            assert np.isclose(cosine_similarity(-a, b),
                               -cosine_similarity(a, b), atol=1e-10)

    def test_cosine_self_similarity_is_one(self):
        """cosine(v, v) == 1 for any non-zero v."""
        np.random.seed(405)
        for _ in range(20):
            n = np.random.randint(1, 10)
            v = np.random.randn(n) + 0.1
            assert np.isclose(cosine_similarity(v, v), 1.0, atol=1e-12)

    def test_cosine_opposite_is_minus_one(self):
        """cosine(v, -v) == -1 for any non-zero v."""
        np.random.seed(406)
        for _ in range(20):
            n = np.random.randint(1, 10)
            v = np.random.randn(n) + 0.1
            assert np.isclose(cosine_similarity(v, -v), -1.0, atol=1e-12)

    def test_cosine_orthogonal_is_zero(self):
        """Standard-basis vectors e_i, e_j (i != j) are orthogonal."""
        n = 8
        np.random.seed(407)
        for _ in range(15):
            i, j = np.random.choice(n, size=2, replace=False)
            ei = np.zeros(n); ei[i] = 1.0
            ej = np.zeros(n); ej[j] = 1.0
            assert np.isclose(cosine_similarity(ei, ej), 0.0, atol=1e-12)

    def test_cosine_relation_to_dot_and_norm(self):
        """cosine(a, b) == dot(a,b) / (l2_norm(a) * l2_norm(b))."""
        np.random.seed(408)
        for _ in range(20):
            n = np.random.randint(2, 10)
            a = np.random.randn(n) + 0.3
            b = np.random.randn(n) + 0.3
            expected = dot(a, b) / (l2_norm(a) * l2_norm(b))
            assert np.isclose(cosine_similarity(a, b), expected, atol=1e-12)

    # --- edge cases and error paths ---

    def test_cosine_single_element_vectors(self):
        """cosine([[x]], [[y]]) == sign(x*y) for 1-D case."""
        np.random.seed(409)
        for _ in range(10):
            # Both positive
            a = np.array([np.random.uniform(0.1, 5.0)])
            b = np.array([np.random.uniform(0.1, 5.0)])
            assert np.isclose(cosine_similarity(a, b), 1.0, atol=1e-12)
            # One negative
            assert np.isclose(cosine_similarity(-a, b), -1.0, atol=1e-12)

    def test_cosine_large_magnitude_numerics(self):
        """Large-magnitude vectors: cosine should remain in [-1, 1]."""
        np.random.seed(410)
        for _ in range(10):
            n = np.random.randint(2, 8)
            a = np.random.randn(n) * 1e10
            b = np.random.randn(n) * 1e10
            c = cosine_similarity(a, b)
            assert -1.0 - 1e-7 <= c <= 1.0 + 1e-7

    def test_cosine_length_mismatch_raises(self):
        np.random.seed(411)
        for la, lb in [(3, 4), (1, 5), (10, 9)]:
            a = np.random.randn(la)
            b = np.random.randn(lb)
            with pytest.raises(ValueError):
                cosine_similarity(a, b)

    def test_cosine_zero_first_arg_raises(self):
        np.random.seed(412)
        for n in [1, 3, 8]:
            a = np.zeros(n)
            b = np.random.randn(n) + 1.0
            with pytest.raises(ValueError):
                cosine_similarity(a, b)

    def test_cosine_zero_second_arg_raises(self):
        np.random.seed(413)
        for n in [1, 3, 8]:
            a = np.random.randn(n) + 1.0
            b = np.zeros(n)
            with pytest.raises(ValueError):
                cosine_similarity(a, b)

    def test_cosine_both_zero_raises(self):
        np.random.seed(414)
        for n in [1, 5]:
            with pytest.raises(ValueError):
                cosine_similarity(np.zeros(n), np.zeros(n))


# ===========================================================================
# linear_forward — oracle: torch.nn.functional.linear / torch.nn.Linear
# ===========================================================================

class TestLinearForwardOracle:
    """Y = X @ W + b must match PyTorch's Linear semantics.

    Note: torch.nn.functional.linear(x, weight, bias) computes
    x @ weight.T + bias, i.e. it stores the weight TRANSPOSED relative to our
    (D, H) convention. So the oracle passes W.T as the torch weight to get
    X @ W + b. This is exactly the transpose subtlety taught in Идея 5.
    """

    def test_linear_forward_vs_torch_F_linear(self):
        np.random.seed(600); torch.manual_seed(600)
        for _ in range(25):
            N = np.random.randint(1, 8)
            D = np.random.randint(1, 8)
            H = np.random.randint(1, 8)
            X = np.random.randn(N, D)
            W = np.random.randn(D, H)
            b = np.random.randn(H)
            tX = torch.tensor(X, dtype=torch.float64)
            tW = torch.tensor(W, dtype=torch.float64)   # (D, H)
            tb = torch.tensor(b, dtype=torch.float64)
            # F.linear expects weight of shape (H, D) -> pass W.T
            expected = F.linear(tX, tW.T, tb).numpy()
            result = linear_forward(X, W, b)
            assert np.allclose(result, expected, atol=1e-9), (
                f"linear_forward mismatch for shapes ({N},{D})@({D},{H})")

    def test_linear_forward_vs_torch_nn_Linear_module(self):
        """Cross-check against an nn.Linear module with weights assigned."""
        np.random.seed(601); torch.manual_seed(601)
        for _ in range(10):
            N = np.random.randint(1, 6)
            D = np.random.randint(1, 6)
            H = np.random.randint(1, 6)
            X = np.random.randn(N, D)
            W = np.random.randn(D, H)
            b = np.random.randn(H)
            layer = torch.nn.Linear(D, H).double()
            with torch.no_grad():
                # nn.Linear stores weight as (H, D) -> assign W.T
                layer.weight.copy_(torch.tensor(W.T, dtype=torch.float64))
                layer.bias.copy_(torch.tensor(b, dtype=torch.float64))
                expected = layer(torch.tensor(X, dtype=torch.float64)).numpy()
            assert np.allclose(linear_forward(X, W, b), expected, atol=1e-9)

    def test_linear_forward_output_shape(self):
        np.random.seed(602)
        for _ in range(20):
            N = np.random.randint(1, 10)
            D = np.random.randint(1, 10)
            H = np.random.randint(1, 10)
            X = np.random.randn(N, D)
            W = np.random.randn(D, H)
            b = np.random.randn(H)
            Y = linear_forward(X, W, b)
            assert Y.shape == (N, H), f"Expected ({N},{H}), got {Y.shape}"

    def test_linear_forward_output_dtype_is_float(self):
        np.random.seed(603)
        X = np.random.randn(3, 4)
        W = np.random.randn(4, 2)
        b = np.random.randn(2)
        Y = linear_forward(X, W, b)
        assert np.issubdtype(Y.dtype, np.floating)

    def test_linear_forward_bias_broadcast_over_batch(self):
        """Fixed W, b; vary N. Each row i equals matmul(X[i:i+1], W)[0] + b."""
        np.random.seed(604)
        D, H = 5, 3
        W = np.random.randn(D, H)
        b = np.random.randn(H)
        for N in (1, 4, 16):
            X = np.random.randn(N, D)
            Y = linear_forward(X, W, b)
            assert Y.shape == (N, H)
            for i in range(N):
                row = matmul(X[i:i + 1, :], W)[0] + b
                assert np.allclose(Y[i, :], row, atol=1e-9)

    def test_linear_forward_zero_bias_equals_matmul(self):
        """b = 0 reduces linear_forward to plain matmul(X, W)."""
        np.random.seed(605)
        for _ in range(15):
            N = np.random.randint(1, 6)
            D = np.random.randint(1, 6)
            H = np.random.randint(1, 6)
            X = np.random.randn(N, D)
            W = np.random.randn(D, H)
            b = np.zeros(H)
            assert np.allclose(linear_forward(X, W, b), matmul(X, W), atol=1e-12)

    def test_linear_forward_additive_in_bias(self):
        """linear_forward(X, W, b1 + b2) == linear_forward(X, W, b1) + (b2 broadcast)."""
        np.random.seed(606)
        N, D, H = 6, 4, 3
        X = np.random.randn(N, D)
        W = np.random.randn(D, H)
        b1 = np.random.randn(H)
        b2 = np.random.randn(H)
        lhs = linear_forward(X, W, b1 + b2)
        rhs = linear_forward(X, W, b1) + b2
        assert np.allclose(lhs, rhs, atol=1e-12)

    def test_linear_forward_does_not_mutate_inputs(self):
        np.random.seed(607)
        X = np.random.randn(4, 3)
        W = np.random.randn(3, 2)
        b = np.random.randn(2)
        X0, W0, b0 = X.copy(), W.copy(), b.copy()
        _ = linear_forward(X, W, b)
        assert np.array_equal(X, X0)
        assert np.array_equal(W, W0)
        assert np.array_equal(b, b0)

    # --- error paths ---

    def test_linear_forward_inner_dim_mismatch_raises(self):
        np.random.seed(608)
        for (N, D, Drows, H) in [(3, 4, 5, 2), (1, 2, 3, 4), (5, 1, 2, 5)]:
            X = np.random.randn(N, D)
            W = np.random.randn(Drows, H)   # Drows != D
            b = np.zeros(H)
            with pytest.raises(ValueError):
                linear_forward(X, W, b)

    def test_linear_forward_bias_length_mismatch_raises(self):
        np.random.seed(609)
        for (N, D, H, bad_b) in [(3, 4, 5, 4), (2, 3, 2, 3), (4, 2, 6, 5)]:
            X = np.random.randn(N, D)
            W = np.random.randn(D, H)
            b = np.zeros(bad_b)             # len != H
            with pytest.raises(ValueError):
                linear_forward(X, W, b)


# ===========================================================================
# Cross-function invariants: dot / l2_norm / cosine_similarity consistency
# ===========================================================================

class TestCrossFunctionInvariants:

    def test_l2_norm_equals_sqrt_of_self_dot(self):
        """||v||^2 == dot(v, v)."""
        np.random.seed(500)
        for _ in range(20):
            n = np.random.randint(1, 10)
            v = np.random.randn(n)
            assert np.isclose(l2_norm(v) ** 2, dot(v, v), atol=1e-9)

    def test_cosine_via_norms_and_dot(self):
        """The formula cosine(a,b) = dot(a,b)/(||a||*||b||) holds."""
        np.random.seed(501)
        for _ in range(20):
            n = np.random.randint(2, 10)
            a = np.random.randn(n) + 0.5
            b = np.random.randn(n) + 0.5
            manual = dot(a, b) / (l2_norm(a) * l2_norm(b))
            assert np.isclose(cosine_similarity(a, b), manual, atol=1e-12)

    def test_matmul_row_dot_column(self):
        """matmul(A, B)[i, j] == dot(A[i, :], B[:, j])."""
        np.random.seed(502)
        for _ in range(10):
            n, k, m = (np.random.randint(1, 6) for _ in range(3))
            A = np.random.randn(n, k)
            B = np.random.randn(k, m)
            C = matmul(A, B)
            for i in range(n):
                for j in range(m):
                    assert np.isclose(C[i, j], dot(A[i, :], B[:, j]), atol=1e-12), (
                        f"C[{i},{j}] = {C[i,j]} != dot(row,col) = {dot(A[i,:], B[:,j])}")

    def test_dot_as_matmul_row_times_column(self):
        """dot(a, b) == matmul(a.reshape(1, n), b.reshape(n, 1))[0, 0]."""
        np.random.seed(503)
        for _ in range(15):
            n = np.random.randint(1, 8)
            a = np.random.randn(n)
            b = np.random.randn(n)
            mm_result = matmul(a.reshape(1, n), b.reshape(n, 1))[0, 0]
            assert np.isclose(dot(a, b), mm_result, atol=1e-12)

    def test_linear_forward_entrywise_dot_plus_bias(self):
        """linear_forward(X, W, b)[i, j] == dot(X[i, :], W[:, j]) + b[j]."""
        np.random.seed(504)
        for _ in range(10):
            N, D, H = (np.random.randint(1, 6) for _ in range(3))
            X = np.random.randn(N, D)
            W = np.random.randn(D, H)
            b = np.random.randn(H)
            Y = linear_forward(X, W, b)
            for i in range(N):
                for j in range(H):
                    expected = dot(X[i, :], W[:, j]) + b[j]
                    assert np.isclose(Y[i, j], expected, atol=1e-12), (
                        f"Y[{i},{j}] = {Y[i,j]} != dot(row,col)+b = {expected}")
