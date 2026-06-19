# Эти тесты трогать не нужно — это эталон поведения.
#
# Важный момент модуля: тест лежит рядом с ПАКЕТОМ mathx/ (каталог mathx с __init__.py).
# pytest кладёт каталог этого теста в sys.path, поэтому пакет `mathx` виден,
# и работает короткий импорт публичного API:
#
#     from mathx import add, mul, factorial, fib
#
# Эти имена доступны на уровне пакета только потому, что mathx/__init__.py их реэкспортирует.

import importlib

import pytest

from mathx import add, mul, factorial, fib


# --- add ---------------------------------------------------------------

def test_add_basic():
    assert add(2, 3) == 5


def test_add_with_zero():
    assert add(0, 0) == 0
    assert add(41, 0) == 41


def test_add_negative():
    assert add(-1, 1) == 0
    assert add(-3, -4) == -7


# --- mul ---------------------------------------------------------------

def test_mul_basic():
    assert mul(2, 3) == 6


def test_mul_by_zero():
    assert mul(7, 0) == 0
    assert mul(0, 99) == 0


def test_mul_negative():
    assert mul(-4, 5) == -20
    assert mul(-6, -7) == 42


# --- factorial ---------------------------------------------------------

def test_factorial_zero_is_one():
    assert factorial(0) == 1


def test_factorial_one():
    assert factorial(1) == 1


def test_factorial_small():
    assert factorial(5) == 120
    assert factorial(6) == 720


@pytest.mark.parametrize(
    "n, expected",
    [(0, 1), (1, 1), (2, 2), (3, 6), (4, 24), (5, 120)],
)
def test_factorial_table(n, expected):
    assert factorial(n) == expected


def test_factorial_negative_raises():
    with pytest.raises(ValueError):
        factorial(-1)


# --- fib ---------------------------------------------------------------

def test_fib_base_cases():
    assert fib(0) == 0
    assert fib(1) == 1


def test_fib_small():
    assert fib(2) == 1
    assert fib(7) == 13


@pytest.mark.parametrize(
    "n, expected",
    [(0, 0), (1, 1), (2, 1), (3, 2), (4, 3), (5, 5), (6, 8), (7, 13), (8, 21), (9, 34), (10, 55)],
)
def test_fib_table(n, expected):
    assert fib(n) == expected


def test_fib_negative_raises():
    with pytest.raises(ValueError):
        fib(-1)


# --- структура пакета: проверяем, что mathx — это действительно ПАКЕТ ----

def test_mathx_is_a_package():
    """У пакета (в отличие от модуля-файла) есть атрибут __path__."""
    mathx = importlib.import_module("mathx")
    assert hasattr(mathx, "__path__")


def test_submodules_exist():
    """Публичные имена можно достать и из подмодулей напрямую."""
    arithmetic = importlib.import_module("mathx.arithmetic")
    sequences = importlib.import_module("mathx.sequences")
    assert hasattr(arithmetic, "add")
    assert hasattr(arithmetic, "mul")
    assert hasattr(sequences, "factorial")
    assert hasattr(sequences, "fib")


def test_public_api_is_reexported():
    """То, что реэкспортировано в __init__, и то, что в подмодуле, — один и тот же объект."""
    import mathx
    import mathx.arithmetic as arithmetic
    assert mathx.add is arithmetic.add
