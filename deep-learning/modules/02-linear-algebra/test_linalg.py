# Эти тесты трогать не нужно — это эталон поведения.
#
# Идея проверок: сравниваем ТВОЮ реализацию с эталоном numpy (np.dot, np.matmul, np.linalg.norm)
# через np.allclose. Тебе самому этими функциями пользоваться нельзя — а тестам можно, они эталон.
# Всё детерминировано (np.random.seed), данные крошечные, считается на CPU мгновенно.

import numpy as np
import pytest

from linalg import dot, matmul, l2_norm, cosine_similarity, linear_forward


# ----------------------------- dot -----------------------------

def test_dot_small_example():
    a = np.array([1.0, 2.0, 3.0])
    b = np.array([4.0, 5.0, 6.0])
    # 1*4 + 2*5 + 3*6 = 32
    assert np.isclose(dot(a, b), 32.0)


def test_dot_returns_python_float():
    a = np.array([1.0, 2.0])
    b = np.array([3.0, 4.0])
    out = dot(a, b)
    assert isinstance(out, float)


def test_dot_matches_numpy_random():
    np.random.seed(0)
    for _ in range(20):
        n = np.random.randint(1, 8)
        a = np.random.randn(n)
        b = np.random.randn(n)
        assert np.isclose(dot(a, b), float(np.dot(a, b)), atol=1e-9)


def test_dot_orthogonal_is_zero():
    a = np.array([1.0, 0.0])
    b = np.array([0.0, 1.0])
    assert np.isclose(dot(a, b), 0.0)


def test_dot_length_mismatch_raises():
    a = np.array([1.0, 2.0, 3.0])
    b = np.array([1.0, 2.0])
    with pytest.raises(ValueError):
        dot(a, b)


# ----------------------------- matmul -----------------------------

def test_matmul_small_example():
    A = np.array([[1.0, 2.0],
                  [3.0, 4.0]])
    B = np.array([[5.0, 6.0],
                  [7.0, 8.0]])
    expected = np.array([[19.0, 22.0],
                         [43.0, 50.0]])
    assert np.allclose(matmul(A, B), expected)


def test_matmul_shape():
    np.random.seed(1)
    A = np.random.randn(3, 5)
    B = np.random.randn(5, 2)
    C = matmul(A, B)
    assert C.shape == (3, 2)


def test_matmul_matches_numpy_random():
    np.random.seed(2)
    for _ in range(20):
        n = np.random.randint(1, 5)
        k = np.random.randint(1, 5)
        m = np.random.randint(1, 5)
        A = np.random.randn(n, k)
        B = np.random.randn(k, m)
        assert np.allclose(matmul(A, B), A @ B, atol=1e-9)


def test_matmul_identity_is_neutral():
    np.random.seed(3)
    A = np.random.randn(4, 4)
    I = np.eye(4)
    assert np.allclose(matmul(A, I), A, atol=1e-9)
    assert np.allclose(matmul(I, A), A, atol=1e-9)


def test_matmul_non_commutative():
    A = np.array([[1.0, 2.0],
                  [3.0, 4.0]])
    B = np.array([[0.0, 1.0],
                  [1.0, 0.0]])
    # В общем случае A@B != B@A — убедимся, что реализация это воспроизводит.
    assert not np.allclose(matmul(A, B), matmul(B, A))


def test_matmul_shape_mismatch_raises():
    A = np.random.RandomState(4).randn(2, 3)  # (2, 3)
    B = np.random.RandomState(5).randn(4, 2)  # (4, 2): внутренние 3 != 4
    with pytest.raises(ValueError):
        matmul(A, B)


# ----------------------------- l2_norm -----------------------------

def test_l2_norm_345():
    v = np.array([3.0, 4.0])
    assert np.isclose(l2_norm(v), 5.0)


def test_l2_norm_returns_python_float():
    v = np.array([1.0, 1.0, 1.0])
    out = l2_norm(v)
    assert isinstance(out, float)


def test_l2_norm_matches_numpy_random():
    np.random.seed(6)
    for _ in range(20):
        n = np.random.randint(1, 8)
        v = np.random.randn(n)
        assert np.isclose(l2_norm(v), float(np.linalg.norm(v)), atol=1e-9)


def test_l2_norm_zero_vector():
    v = np.zeros(5)
    assert np.isclose(l2_norm(v), 0.0)


def test_l2_norm_is_nonnegative():
    np.random.seed(7)
    for _ in range(10):
        v = np.random.randn(np.random.randint(1, 6))
        assert l2_norm(v) >= 0.0


# ----------------------------- cosine_similarity -----------------------------

def test_cosine_identical_vectors_is_one():
    a = np.array([2.0, 1.0, 0.5])
    assert np.isclose(cosine_similarity(a, a), 1.0)


def test_cosine_scaled_vector_is_one():
    # Косинус зависит только от направления: масштаб не меняет ответ.
    a = np.array([1.0, 2.0, 3.0])
    b = 4.0 * a
    assert np.isclose(cosine_similarity(a, b), 1.0)


def test_cosine_opposite_vectors_is_minus_one():
    a = np.array([1.0, 0.0])
    b = np.array([-1.0, 0.0])
    assert np.isclose(cosine_similarity(a, b), -1.0)


def test_cosine_orthogonal_is_zero():
    a = np.array([1.0, 0.0])
    b = np.array([0.0, 1.0])
    assert np.isclose(cosine_similarity(a, b), 0.0)


def test_cosine_45_degrees():
    a = np.array([1.0, 0.0])
    b = np.array([1.0, 1.0])
    # cos 45° = 1 / sqrt(2)
    assert np.isclose(cosine_similarity(a, b), 1.0 / np.sqrt(2.0))


def test_cosine_matches_numpy_random():
    np.random.seed(8)
    for _ in range(20):
        n = np.random.randint(1, 8)
        a = np.random.randn(n)
        b = np.random.randn(n)
        # Гарантируем ненулевые нормы (на seeded данных это и так почти всегда так).
        expected = float(np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b)))
        assert np.isclose(cosine_similarity(a, b), expected, atol=1e-9)


def test_cosine_in_range():
    np.random.seed(9)
    for _ in range(20):
        n = np.random.randint(2, 6)
        a = np.random.randn(n)
        b = np.random.randn(n)
        c = cosine_similarity(a, b)
        assert -1.0 - 1e-9 <= c <= 1.0 + 1e-9


def test_cosine_returns_python_float():
    a = np.array([1.0, 2.0])
    b = np.array([2.0, 1.0])
    assert isinstance(cosine_similarity(a, b), float)


def test_cosine_length_mismatch_raises():
    a = np.array([1.0, 2.0, 3.0])
    b = np.array([1.0, 2.0])
    with pytest.raises(ValueError):
        cosine_similarity(a, b)


def test_cosine_zero_vector_raises():
    a = np.array([0.0, 0.0])
    b = np.array([1.0, 1.0])
    with pytest.raises(ValueError):
        cosine_similarity(a, b)


# ----------------------------- linear_forward -----------------------------

def test_linear_forward_small_example():
    # X (2, 3) @ W (3, 2) + b (2,)
    X = np.array([[1.0, 2.0, 3.0],
                  [4.0, 5.0, 6.0]])
    W = np.array([[1.0, 0.0],
                  [0.0, 1.0],
                  [1.0, 1.0]])
    b = np.array([10.0, 20.0])
    # row 0: [1*1+2*0+3*1, 1*0+2*1+3*1] + b = [4, 5] + [10, 20] = [14, 25]
    # row 1: [4+0+6, 0+5+6] + b = [10, 11] + [10, 20] = [20, 31]
    expected = np.array([[14.0, 25.0],
                         [20.0, 31.0]])
    assert np.allclose(linear_forward(X, W, b), expected)


def test_linear_forward_shape():
    np.random.seed(20)
    N, D, H = 7, 4, 3
    X = np.random.randn(N, D)
    W = np.random.randn(D, H)
    b = np.random.randn(H)
    Y = linear_forward(X, W, b)
    assert Y.shape == (N, H)


def test_linear_forward_matches_numpy_reference():
    np.random.seed(21)
    for _ in range(20):
        N = np.random.randint(1, 6)
        D = np.random.randint(1, 6)
        H = np.random.randint(1, 6)
        X = np.random.randn(N, D)
        W = np.random.randn(D, H)
        b = np.random.randn(H)
        assert np.allclose(linear_forward(X, W, b), X @ W + b, atol=1e-9)


def test_linear_forward_zero_bias_is_matmul():
    np.random.seed(22)
    X = np.random.randn(5, 3)
    W = np.random.randn(3, 4)
    b = np.zeros(4)
    assert np.allclose(linear_forward(X, W, b), matmul(X, W), atol=1e-12)


def test_linear_forward_bias_broadcast_across_rows():
    # With W = 0 the matmul part vanishes; every row must equal b exactly.
    np.random.seed(23)
    N, D, H = 4, 3, 2
    X = np.random.randn(N, D)
    W = np.zeros((D, H))
    b = np.array([7.0, -3.0])
    Y = linear_forward(X, W, b)
    for i in range(N):
        assert np.allclose(Y[i, :], b, atol=1e-12)


def test_linear_forward_inner_dim_mismatch_raises():
    X = np.random.RandomState(24).randn(2, 3)  # D = 3
    W = np.random.RandomState(25).randn(4, 2)  # rows = 4 != 3
    b = np.zeros(2)
    with pytest.raises(ValueError):
        linear_forward(X, W, b)


def test_linear_forward_bias_length_mismatch_raises():
    X = np.random.RandomState(26).randn(2, 3)  # D = 3
    W = np.random.RandomState(27).randn(3, 5)  # H = 5
    b = np.zeros(4)                             # len 4 != H = 5
    with pytest.raises(ValueError):
        linear_forward(X, W, b)
