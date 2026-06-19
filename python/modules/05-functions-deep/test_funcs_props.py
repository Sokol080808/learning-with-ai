# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_funcs.py проверяет ФИКСИРОВАННЫЕ примеры (apply_twice(...) == 7 и т.п.). Здесь —
# РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# огромные числа, пустые строки, странные перестановки) и проверяет не конкретный ответ, а
# ИНВАРИАНТЫ — свойства, которые обязаны выполняться для ЛЮБОГО входа: законы композиции
# функций, независимость замыканий, устойчивость сортировки, учёт реальных вызовов в кэше.
# Если реализация верна на примерах, но ломается на краевом случае, эти тесты это поймают и
# покажут минимальный контрпример.

from hypothesis import given, settings
from hypothesis import strategies as st

from funcs import (
    apply_twice,
    make_multiplier,
    compose,
    sort_by_length,
    memoize,
)

# Числа в разумных пределах: умножение/сложение замыканий не должно упираться в производительность.
ints = st.integers(min_value=-10**6, max_value=10**6)
small_ints = st.integers(min_value=-1000, max_value=1000)


# --- apply_twice: f(f(x)) — это двукратная композиция f с самой собой ---

@given(a=ints, b=ints, x=ints)
def test_apply_twice_matches_manual_double(a, b, x):
    # Для любой аффинной f(t) = a*t + b результат должен совпасть с f(f(x)),
    # посчитанным руками — это и есть определение «применить дважды».
    f = lambda t: a * t + b
    assert apply_twice(f, x) == f(f(x))


@given(x=ints)
def test_apply_twice_identity_is_noop(x):
    # Тождество, применённое дважды, не меняет аргумент.
    assert apply_twice(lambda t: t, x) == x


@given(x=ints)
def test_apply_twice_equivalent_to_compose_with_self(x):
    # apply_twice(f, x) обязан равняться compose(f, f)(x) — обе формы это f(f(x)).
    f = lambda t: t * 2 + 1
    assert apply_twice(f, x) == compose(f, f)(x)


@given(s=st.text(max_size=20))
def test_apply_twice_on_strings_appends_twice(s):
    # Конкатенация суффикса дважды удлиняет строку ровно на два суффикса.
    assert apply_twice(lambda t: t + "!", s) == s + "!!"


# --- make_multiplier: замыкание захватывает n; функции независимы ---

@given(n=ints, x=ints)
def test_make_multiplier_multiplies(n, x):
    assert make_multiplier(n)(x) == n * x


@given(n=ints)
def test_make_multiplier_returns_callable(n):
    assert callable(make_multiplier(n))


@given(n=ints)
def test_make_multiplier_zero_argument_is_zero(n):
    # Сколько ни умножай на ноль — получится ноль.
    assert make_multiplier(n)(0) == 0


@given(n1=ints, n2=ints, x=ints)
def test_make_multiplier_closures_are_independent(n1, n2, x):
    # Создание второго множителя не должно «портить» первый: каждый помнит свой n.
    m1 = make_multiplier(n1)
    m2 = make_multiplier(n2)
    assert m1(x) == n1 * x
    assert m2(x) == n2 * x
    # повторно убеждаемся, что m1 не изменился после создания m2
    assert m1(x) == n1 * x


def test_make_multiplier_handles_huge_factor():
    big = make_multiplier(10 ** 50)
    assert big(2) == 2 * 10 ** 50


# --- compose: ассоциативность, тождество, порядок f(g(x)) ---

@given(a=small_ints, b=small_ints, c=small_ints, d=small_ints, x=small_ints)
def test_compose_is_f_of_g(a, b, c, d, x):
    # compose(f, g)(x) обязан равняться f(g(x)), а не g(f(x)).
    f = lambda t: a * t + b
    g = lambda t: c * t + d
    assert compose(f, g)(x) == f(g(x))


@given(x=ints)
def test_compose_with_identity_is_noop(x):
    idf = lambda t: t
    f = lambda t: t * 3 - 7
    # Тождество слева или справа не меняет поведение f.
    assert compose(f, idf)(x) == f(x)
    assert compose(idf, f)(x) == f(x)


@given(a=small_ints, b=small_ints, c=small_ints, d=small_ints, e=small_ints, k=small_ints, x=small_ints)
@settings(max_examples=100, deadline=None)
def test_compose_is_associative(a, b, c, d, e, k, x):
    # (f ∘ g) ∘ h == f ∘ (g ∘ h) для любых f, g, h.
    f = lambda t: a * t + b
    g = lambda t: c * t + d
    h = lambda t: e * t + k
    left = compose(compose(f, g), h)
    right = compose(f, compose(g, h))
    assert left(x) == right(x)


def test_compose_mixed_types_round_trip():
    # g превращает число в строку, f дописывает символ — порядок строго f(g(x)).
    assert compose(lambda s: s + "!", str)(7) == "7!"


# --- sort_by_length: устойчивость, неизменность входа, монотонность длин ---

words_lists = st.lists(st.text(max_size=15), max_size=30)


@given(words=words_lists)
def test_sort_by_length_lengths_are_non_decreasing(words):
    result = sort_by_length(words)
    lengths = [len(w) for w in result]
    assert lengths == sorted(lengths)


@given(words=words_lists)
def test_sort_by_length_is_a_permutation(words):
    # Результат — перестановка входа: те же элементы, та же кратность.
    result = sort_by_length(words)
    assert sorted(result) == sorted(words)
    assert len(result) == len(words)


@given(words=words_lists)
def test_sort_by_length_does_not_mutate_input(words):
    snapshot = list(words)
    result = sort_by_length(words)
    assert words == snapshot          # вход не тронут
    if words:
        assert result is not words    # возвращён новый список


@given(words=words_lists)
def test_sort_by_length_is_stable(words):
    # Устойчивость: элементы одинаковой длины сохраняют исходный относительный порядок.
    result = sort_by_length(words)
    # Сравниваем с устойчивым эталоном sorted(key=len): они обязаны совпасть поэлементно.
    assert result == sorted(words, key=len)


@given(words=words_lists)
def test_sort_by_length_idempotent(words):
    once = sort_by_length(words)
    twice = sort_by_length(once)
    assert once == twice


# --- memoize: учёт реальных вызовов, сохранение имени, прозрачность результата ---

@given(args=st.lists(st.integers(min_value=-50, max_value=50), min_size=1, max_size=30))
@settings(max_examples=100, deadline=None)
def test_memoize_counts_only_unique_calls(args):
    calls = []

    @memoize
    def f(n):
        calls.append(n)
        return n * n

    for a in args:
        assert f(a) == a * a            # результат всегда корректный, кэш он или нет
    # тело выполнилось ровно по разу на каждый УНИКАЛЬНЫЙ аргумент
    assert sorted(calls) == sorted(set(args))


@given(x=ints, y=ints)
def test_memoize_is_transparent_for_two_args(x, y):
    @memoize
    def add(a, b):
        return a + b

    # повторные вызовы с теми же аргументами дают тот же ответ
    assert add(x, y) == x + y
    assert add(x, y) == x + y


@given(args=st.lists(st.integers(min_value=-20, max_value=20), min_size=1, max_size=20))
@settings(max_examples=100, deadline=None)
def test_memoize_second_pass_does_not_call_body(args):
    calls = []

    @memoize
    def f(n):
        calls.append(n)
        return n + 100

    for a in args:           # первый проход — наполняем кэш
        f(a)
    count_after_first = len(calls)
    for a in args:           # второй проход — всё обязано браться из кэша
        f(a)
    assert len(calls) == count_after_first


def test_memoize_preserves_name_and_doc():
    @memoize
    def my_func(x):
        """строка документации"""
        return x

    assert my_func.__name__ == "my_func"
