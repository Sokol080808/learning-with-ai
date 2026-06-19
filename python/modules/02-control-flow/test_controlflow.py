# Эти тесты трогать не нужно — это эталон поведения.
# Запуск: ./python/run.sh 02
import pytest

from controlflow import fizzbuzz, factorial, fib, count_vowels, gcd


# --- fizzbuzz ---------------------------------------------------------------

def test_fizzbuzz_first_five():
    assert fizzbuzz(5) == ["1", "2", "Fizz", "4", "Buzz"]


def test_fizzbuzz_fifteen_has_fizzbuzz():
    assert fizzbuzz(15) == [
        "1", "2", "Fizz", "4", "Buzz",
        "Fizz", "7", "8", "Fizz", "Buzz",
        "11", "Fizz", "13", "14", "FizzBuzz",
    ]


def test_fizzbuzz_returns_strings():
    result = fizzbuzz(3)
    assert all(isinstance(item, str) for item in result)
    assert result == ["1", "2", "Fizz"]


def test_fizzbuzz_length_matches_n():
    assert len(fizzbuzz(20)) == 20


def test_fizzbuzz_zero_is_empty():
    assert fizzbuzz(0) == []


# --- factorial --------------------------------------------------------------

@pytest.mark.parametrize("n, expected", [
    (0, 1),
    (1, 1),
    (2, 2),
    (3, 6),
    (5, 120),
    (6, 720),
    (10, 3628800),
])
def test_factorial(n, expected):
    assert factorial(n) == expected


# --- fib --------------------------------------------------------------------

@pytest.mark.parametrize("n, expected", [
    (0, 0),
    (1, 1),
    (2, 1),
    (3, 2),
    (4, 3),
    (5, 5),
    (6, 8),
    (10, 55),
    (15, 610),
])
def test_fib(n, expected):
    assert fib(n) == expected


# --- count_vowels -----------------------------------------------------------

def test_count_vowels_basic():
    assert count_vowels("hello") == 2


def test_count_vowels_all_vowels():
    assert count_vowels("aeiou") == 5


def test_count_vowels_case_insensitive():
    assert count_vowels("AEIOU") == 5
    assert count_vowels("HeLLo WoRLD") == 3


def test_count_vowels_empty():
    assert count_vowels("") == 0


def test_count_vowels_no_vowels():
    assert count_vowels("rhythm xyz") == 0


def test_count_vowels_ignores_digits_and_spaces():
    assert count_vowels("a1 e2 i3") == 3


# --- gcd --------------------------------------------------------------------

@pytest.mark.parametrize("a, b, expected", [
    (48, 18, 6),
    (54, 24, 6),
    (17, 5, 1),
    (100, 10, 10),
    (21, 14, 7),
    (0, 5, 5),
    (7, 0, 7),
    (13, 13, 13),
])
def test_gcd(a, b, expected):
    assert gcd(a, b) == expected
