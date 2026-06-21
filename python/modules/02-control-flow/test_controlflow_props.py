# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_controlflow.py проверяет ФИКСИРОВАННЫЕ примеры (fizzbuzz(5) == [...] и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# ноль, отрицательные, юникод, длинные строки) и проверяет не конкретный ответ, а
# ИНВАРИАНТЫ — свойства, которые обязаны выполняться для ЛЮБОГО входа. Если твоя реализация
# верна на примерах, но ломается на краевом случае, эти тесты это поймают и покажут
# минимальный контрпример.

from hypothesis import given, settings
from hypothesis import strategies as st

from controlflow import fizzbuzz, factorial, fib, count_vowels, gcd, pairwise_sum


# --- fizzbuzz: длина, элементы и правило замены ------------------------------

@given(n=st.integers(min_value=1, max_value=300))
def test_fizzbuzz_length_equals_n(n):
    assert len(fizzbuzz(n)) == n


@given(n=st.integers(min_value=-50, max_value=0))
def test_fizzbuzz_nonpositive_is_empty(n):
    assert fizzbuzz(n) == []


@given(n=st.integers(min_value=1, max_value=300))
@settings(max_examples=100, deadline=None)
def test_fizzbuzz_each_position_follows_rule(n):
    result = fizzbuzz(n)
    for i, item in enumerate(result, start=1):
        assert isinstance(item, str)
        if i % 15 == 0:
            assert item == "FizzBuzz"
        elif i % 3 == 0:
            assert item == "Fizz"
        elif i % 5 == 0:
            assert item == "Buzz"
        else:
            assert item == str(i)


@given(n=st.integers(min_value=1, max_value=300))
def test_fizzbuzz_is_prefix_stable(n):
    # Результат для n — это ровно первые n элементов результата для n+1.
    assert fizzbuzz(n) == fizzbuzz(n + 1)[:n]


# --- factorial: рекуррентная связь и положительность ------------------------

@given(n=st.integers(min_value=1, max_value=200))
def test_factorial_recurrence(n):
    # n! == n * (n-1)!
    assert factorial(n) == n * factorial(n - 1)


def test_factorial_base_cases():
    assert factorial(0) == 1
    assert factorial(1) == 1


@given(n=st.integers(min_value=0, max_value=200))
def test_factorial_positive_and_int(n):
    value = factorial(n)
    assert isinstance(value, int)
    assert value >= 1


@given(n=st.integers(min_value=2, max_value=200))
def test_factorial_strictly_grows(n):
    # При n >= 2 факториал строго растёт.
    assert factorial(n) > factorial(n - 1)


# --- fib: рекуррентная связь и тождество суммы ------------------------------

def test_fib_base_cases():
    assert fib(0) == 0
    assert fib(1) == 1


@given(n=st.integers(min_value=2, max_value=500))
def test_fib_recurrence(n):
    assert fib(n) == fib(n - 1) + fib(n - 2)


@given(n=st.integers(min_value=1, max_value=500))
def test_fib_sum_identity(n):
    # Сумма первых n чисел Фибоначчи == fib(n+2) - 1.
    total = sum(fib(i) for i in range(1, n + 1))
    assert total == fib(n + 2) - 1


@given(n=st.integers(min_value=0, max_value=500))
def test_fib_nonnegative_int(n):
    value = fib(n)
    assert isinstance(value, int)
    assert value >= 0


# --- count_vowels: подсчёт гласных, инварианты ------------------------------

VOWELS = "aeiou"
text_any = st.text(max_size=200)


@given(s=text_any)
def test_count_vowels_matches_manual(s):
    expected = sum(1 for ch in s.lower() if ch in VOWELS)
    assert count_vowels(s) == expected


@given(s=text_any)
def test_count_vowels_within_bounds(s):
    n = count_vowels(s)
    assert 0 <= n <= len(s)


@given(s=text_any)
def test_count_vowels_case_insensitive(s):
    assert count_vowels(s) == count_vowels(s.upper())
    assert count_vowels(s) == count_vowels(s.lower())


@given(a=text_any, b=text_any)
def test_count_vowels_additive(a, b):
    # Гласные считаются посимвольно, значит счётчик аддитивен по конкатенации.
    assert count_vowels(a + b) == count_vowels(a) + count_vowels(b)


def test_count_vowels_empty_and_unicode():
    assert count_vowels("") == 0
    # Не-английские буквы и эмодзи не считаются гласными.
    assert count_vowels("йцукён 🚀 ω") == 0


# --- gcd: делимость и базовые свойства --------------------------------------

pos = st.integers(min_value=1, max_value=10**9)


@given(a=pos, b=pos)
def test_gcd_divides_both(a, b):
    g = gcd(a, b)
    assert g >= 1
    assert a % g == 0
    assert b % g == 0


@given(a=pos, b=pos)
def test_gcd_matches_math(a, b):
    import math
    assert gcd(a, b) == math.gcd(a, b)


@given(a=st.integers(min_value=0, max_value=10**9))
def test_gcd_with_zero(a):
    assert gcd(a, 0) == a
    assert gcd(0, a) == a


@given(a=pos, b=pos)
def test_gcd_symmetric(a, b):
    assert gcd(a, b) == gcd(b, a)


@given(a=pos)
def test_gcd_self(a):
    assert gcd(a, a) == a


# --- pairwise_sum: длина, значения, инварианты -------------------------------

int_list = st.lists(st.integers(min_value=-10**6, max_value=10**6))


@given(xs=int_list)
@settings(derandomize=True)
def test_pairwise_sum_length(xs):
    # Результат всегда на один элемент короче входа (или пуст, если вход пуст/одноэлементный).
    result = pairwise_sum(xs)
    expected_len = max(0, len(xs) - 1)
    assert len(result) == expected_len


@given(xs=int_list)
@settings(derandomize=True)
def test_pairwise_sum_values_match_oracle(xs):
    # Оракул: каждый элемент результата равен xs[i] + xs[i+1].
    result = pairwise_sum(xs)
    for i, val in enumerate(result):
        assert val == xs[i] + xs[i + 1]


@given(xs=st.lists(st.integers(min_value=-10**6, max_value=10**6), min_size=1))
@settings(derandomize=True)
def test_pairwise_sum_nonempty_singleton_or_longer(xs):
    # Для списка из одного элемента результат пуст; для двух и более — непуст.
    result = pairwise_sum(xs)
    if len(xs) == 1:
        assert result == []
    else:
        assert len(result) == len(xs) - 1
        assert len(result) >= 1


@given(xs=st.lists(st.integers(min_value=0, max_value=10**6), min_size=2))
@settings(derandomize=True)
def test_pairwise_sum_nonneg_inputs_give_nonneg(xs):
    # Если все входы неотрицательны — все суммы тоже неотрицательны.
    for val in pairwise_sum(xs):
        assert val >= 0
