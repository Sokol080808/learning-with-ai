# Эти тесты трогать не нужно — это эталон поведения.
#
# Модуль 00 — разминка. Тесты крошечные, детерминированные (фиксируем сид),
# гоняются на CPU мгновенно. Импортируем то, что ты реализуешь в warmup.py.

import numpy as np
import pytest

from warmup import vector_add, mean


def test_vector_add_basic():
    a = np.array([1.0, 2.0, 3.0])
    b = np.array([10.0, 20.0, 30.0])
    result = vector_add(a, b)
    expected = np.array([11.0, 22.0, 33.0])
    assert np.allclose(result, expected)


def test_vector_add_preserves_shape():
    np.random.seed(0)
    a = np.random.randn(7)
    b = np.random.randn(7)
    result = vector_add(a, b)
    # Поэлементное сложение сохраняет форму: (7,) + (7,) -> (7,).
    assert np.asarray(result).shape == (7,)


def test_vector_add_matches_numpy_reference():
    np.random.seed(1)
    a = np.random.randn(5)
    b = np.random.randn(5)
    result = vector_add(a, b)
    # Эталон — обычное сложение numpy.
    assert np.allclose(result, a + b)


def test_vector_add_with_zeros_is_identity():
    np.random.seed(2)
    a = np.random.randn(4)
    zeros = np.zeros(4)
    assert np.allclose(vector_add(a, zeros), a)


def test_mean_basic():
    xs = np.array([2.0, 4.0, 6.0])
    assert np.isclose(mean(xs), 4.0)


def test_mean_returns_python_float():
    xs = np.array([1.0, 2.0, 3.0, 4.0])
    result = mean(xs)
    # Контракт требует именно питоновский float, а не numpy-массив ранга 0.
    assert isinstance(result, float)


def test_mean_matches_numpy_reference():
    np.random.seed(3)
    xs = np.random.randn(50)
    assert np.isclose(mean(xs), float(np.mean(xs)))


def test_mean_constant_array():
    xs = np.full(10, 5.0)
    # Среднее массива из одинаковых чисел равно этому числу.
    assert np.isclose(mean(xs), 5.0)
