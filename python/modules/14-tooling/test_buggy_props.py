# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_buggy.py проверяет ФИКСИРОВАННЫЕ примеры (sum_first_n([10,20,30,40],2) == 30 и т.п.).
# Здесь — РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов (включая злые —
# пустые списки, одиночные элементы, сплошь отрицательные числа, нули, границы среза, times == 0).
# Они проверяют не конкретный ответ, а ИНВАРИАНТЫ — свойства правильного поведения, которые именно
# спрятанные баги (off-by-one, неверная инициализация, лишняя +1, сравнение элемента с собой,
# times-1) и нарушают. Если починишь не до конца — минимальный контрпример всплывёт здесь.

from hypothesis import given, settings
from hypothesis import strategies as st

from buggy import average, has_duplicate, max_in, repeat, sum_first_n

ints = st.integers(min_value=-10**9, max_value=10**9)
int_lists = st.lists(ints, min_size=0, max_size=30)
nonempty_int_lists = st.lists(ints, min_size=1, max_size=30)


# --- sum_first_n: ровно первые n элементов, ни больше ни меньше ---

@given(xs=int_lists, data=st.data())
def test_sum_first_n_matches_slice(xs, data):
    n = data.draw(st.integers(min_value=0, max_value=len(xs)))
    # эталон: сумма среза xs[:n] — ровно n первых элементов
    assert sum_first_n(xs, n) == sum(xs[:n])


@given(xs=int_lists, data=st.data())
def test_sum_first_n_count_is_exactly_n(xs, data):
    # длиннее проверка: каждый добавленный элемент n обязан что-то прибавлять/убавлять корректно.
    # sum_first_n(xs, n) - sum_first_n(xs, n-1) == xs[n-1] для 1 <= n <= len(xs).
    if len(xs) >= 1:
        n = data.draw(st.integers(min_value=1, max_value=len(xs)))
        assert sum_first_n(xs, n) - sum_first_n(xs, n - 1) == xs[n - 1]


@given(xs=int_lists)
def test_sum_first_n_zero_is_zero(xs):
    # сумма нуля первых элементов — всегда 0
    assert sum_first_n(xs, 0) == 0


@given(xs=int_lists)
def test_sum_first_n_whole_list_is_total(xs):
    # n == len(xs) — это сумма всего списка
    assert sum_first_n(xs, len(xs)) == sum(xs)


def test_sum_first_n_edge_cases():
    assert sum_first_n([5], 1) == 5
    assert sum_first_n([10, 20, 30, 40], 2) == 30
    assert sum_first_n([-3, -7], 1) == -3


# --- max_in: настоящий максимум, в том числе для сплошь отрицательных списков ---

@given(xs=nonempty_int_lists)
def test_max_in_matches_builtin(xs):
    # эталон — встроенный max; баг с инициализацией нулём это сломает на отрицательных
    assert max_in(xs) == max(xs)


@given(xs=nonempty_int_lists)
def test_max_in_is_a_member_and_upper_bound(xs):
    m = max_in(xs)
    assert m in xs               # максимум — реальный элемент списка
    assert all(x <= m for x in xs)  # и не меньше любого из них


@given(xs=st.lists(st.integers(min_value=-10**9, max_value=-1), min_size=1, max_size=30))
def test_max_in_all_negative(xs):
    # ключевой случай: все числа строго отрицательные, ноль не должен «победить»
    assert max_in(xs) == max(xs)
    assert max_in(xs) < 0


def test_max_in_edge_cases():
    assert max_in([7]) == 7
    assert max_in([-5, -2, -9]) == -2
    assert max_in([0, -1, -3]) == 0


# --- average: ровно sum/len как float, без лишних единиц ---

@given(xs=nonempty_int_lists)
def test_average_matches_formula(xs):
    # эталон — sum/len; лишняя +1 в числителе это сломает (особенно на нулях)
    assert average(xs) == sum(xs) / len(xs)


@given(xs=nonempty_int_lists)
def test_average_within_min_max(xs):
    avg = average(xs)
    assert min(xs) <= avg <= max(xs)


@given(xs=nonempty_int_lists)
def test_average_is_float(xs):
    assert isinstance(average(xs), float)


@given(value=ints, count=st.integers(min_value=1, max_value=30))
def test_average_of_constant_list_is_that_value(value, count):
    # среднее списка из одинаковых чисел равно самому числу — лишняя +1 это нарушит
    assert average([value] * count) == float(value)


def test_average_zeros_is_zero():
    assert average([0, 0, 0]) == 0.0


# --- has_duplicate: True ровно тогда, когда есть два РАЗНЫХ равных элемента ---

@given(xs=int_lists)
def test_has_duplicate_matches_set_criterion(xs):
    # эталон: дубликаты есть <=> множество короче списка. Сравнение элемента с собой это ломает.
    assert has_duplicate(xs) == (len(set(xs)) < len(xs))


@given(xs=int_lists)
def test_has_duplicate_returns_real_bool(xs):
    assert has_duplicate(xs) is True or has_duplicate(xs) is False


@given(xs=st.lists(ints, min_size=0, max_size=20))
def test_has_duplicate_distinct_lists_are_false(xs):
    # из списка делаем заведомо уникальный — дубликатов быть не должно
    unique = list(dict.fromkeys(xs))  # сохраняет порядок, убирает повторы
    assert has_duplicate(unique) is False


@given(xs=int_lists)
def test_has_duplicate_appending_existing_flips_false_to_true(xs):
    # берём заведомо уникальный список (на нём ответ ОБЯЗАН быть False),
    # затем дописываем копию первого элемента — и ответ обязан стать True.
    # Баг «сравнение с собой» врёт уже на первом шаге (говорит True без повторов).
    unique = list(dict.fromkeys(xs))  # без повторов, порядок сохранён
    assert has_duplicate(unique) is False
    if unique:
        assert has_duplicate(unique + [unique[0]]) is True


def test_has_duplicate_edge_cases():
    assert has_duplicate([]) is False
    assert has_duplicate([5]) is False          # один элемент — не дубликат сам себе
    assert has_duplicate([1, 2, 3]) is False
    assert has_duplicate([1, 2, 2]) is True
    assert has_duplicate([7, 1, 2, 3, 7]) is True


# --- repeat: длина == len(s)*times, повтор ровно times раз ---

@given(s=st.text(max_size=10), times=st.integers(min_value=0, max_value=50))
def test_repeat_matches_multiplication(s, times):
    # эталон — встроенное умножение строки; баг times-1 это сломает
    assert repeat(s, times) == s * times


@given(s=st.text(max_size=10), times=st.integers(min_value=0, max_value=50))
def test_repeat_length(s, times):
    assert len(repeat(s, times)) == len(s) * times


@given(s=st.text(min_size=1, max_size=10), times=st.integers(min_value=0, max_value=30))
def test_repeat_count_of_occurrences(s, times):
    # непустую строку результат содержит ровно times раз
    result = repeat(s, times)
    assert result == s * times
    if s:
        assert result.count(s) >= times  # как минимум столько вхождений


@settings(max_examples=120)
@given(s=st.text(max_size=8), a=st.integers(min_value=0, max_value=20),
       b=st.integers(min_value=0, max_value=20))
def test_repeat_additive(s, a, b):
    # повторить (a+b) раз == повторить a раз и дописать ещё b раз
    assert repeat(s, a + b) == repeat(s, a) + repeat(s, b)


def test_repeat_edge_cases():
    assert repeat("ab", 0) == ""
    assert repeat("x", 1) == "x"
    assert repeat("ab", 3) == "ababab"
    assert len(repeat("-", 5)) == 5
