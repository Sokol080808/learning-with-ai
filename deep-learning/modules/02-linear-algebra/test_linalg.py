# Эти тесты трогать не нужно — это эталон поведения.
#
# Идея проверок: сравниваем ТВОЮ реализацию с эталоном numpy (np.dot, np.matmul, np.linalg.norm)
# через np.allclose. Тебе самому этими функциями пользоваться нельзя — а тестам можно, они эталон.
# Всё детерминировано (np.random.seed), данные крошечные, считается на CPU мгновенно.

import numpy as np
import pytest

from linalg import dot, matmul, l2_norm, cosine_similarity


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
