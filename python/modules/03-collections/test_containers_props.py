# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_containers.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — РАНДОМИЗИРОВАННЫЕ
# property-тесты: hypothesis сам подбирает сотни входов (включая злые — пустые списки,
# огромные значения, отрицательные числа, дубликаты, юникод-строки) и проверяет не
# конкретный ответ, а ИНВАРИАНТЫ — свойства, которые обязаны выполняться для ЛЮБОГО
# входа. Если реализация верна на примерах, но ломается на краевом случае, эти тесты
# это поймают и покажут минимальный контрпример.

from hypothesis import given, settings
from hypothesis import strategies as st

from containers import (
    unique_sorted,
    word_lengths,
    flatten,
    intersection,
    invert_dict,
)

ints = st.integers(min_value=-10**9, max_value=10**9)
int_lists = st.lists(ints, max_size=200)
# Слова берём с уникальными ключами там, где это важно; здесь — произвольный текст.
words = st.text(max_size=20)
word_lists = st.lists(words, max_size=100)


# --- unique_sorted: результат отсортирован, без дублей, и это перестановка set(xs) ---

@given(xs=int_lists)
def test_unique_sorted_is_sorted(xs):
    result = unique_sorted(xs)
    assert result == sorted(result)


@given(xs=int_lists)
def test_unique_sorted_has_no_duplicates(xs):
    result = unique_sorted(xs)
    assert len(result) == len(set(result))


@given(xs=int_lists)
def test_unique_sorted_same_value_set_as_input(xs):
    # Содержимое не теряется и не добавляется: множество значений совпадает с set(xs).
    result = unique_sorted(xs)
    assert set(result) == set(xs)


@given(xs=int_lists)
def test_unique_sorted_equals_sorted_set(xs):
    # Сильная проверка корректности: ровно sorted(set(xs)).
    assert unique_sorted(xs) == sorted(set(xs))


@given(xs=int_lists)
def test_unique_sorted_idempotent(xs):
    once = unique_sorted(xs)
    assert unique_sorted(once) == once


@given(xs=int_lists)
def test_unique_sorted_returns_list(xs):
    assert isinstance(unique_sorted(xs), list)


def test_unique_sorted_empty_and_huge():
    assert unique_sorted([]) == []
    big = 10**100
    assert unique_sorted([big, -big, 0, big]) == [-big, 0, big]


# --- word_lengths: каждое значение — длина своего ключа ---

@given(ws=word_lists)
def test_word_lengths_values_are_lengths(ws):
    result = word_lengths(ws)
    for w, length in result.items():
        assert length == len(w)


@given(ws=word_lists)
def test_word_lengths_keys_are_unique_words(ws):
    result = word_lengths(ws)
    assert set(result.keys()) == set(ws)


@given(ws=word_lists)
def test_word_lengths_returns_dict(ws):
    assert isinstance(word_lengths(ws), dict)


def test_word_lengths_empty_string_and_unicode():
    assert word_lengths([""]) == {"": 0}
    assert word_lengths(["мир", "🙂"]) == {"мир": 3, "🙂": 1}


# --- flatten: порядок и дубликаты сохраняются; длина = сумма длин строк ---

@given(matrix=st.lists(int_lists, max_size=30))
def test_flatten_length_is_sum_of_rows(matrix):
    result = flatten(matrix)
    assert len(result) == sum(len(row) for row in matrix)


@given(matrix=st.lists(int_lists, max_size=30))
def test_flatten_concatenation_order_preserved(matrix):
    # Сильная проверка: flatten — это ровно конкатенация строк по порядку.
    expected = []
    for row in matrix:
        expected.extend(row)
    assert flatten(matrix) == expected


@given(matrix=st.lists(int_lists, max_size=30))
def test_flatten_returns_list(matrix):
    assert isinstance(flatten(matrix), list)


def test_flatten_extreme_cases():
    assert flatten([]) == []
    assert flatten([[], [], []]) == []
    assert flatten([[1, 1], [2], [1]]) == [1, 1, 2, 1]


# --- intersection: множество, лежащее и в a, и в b ---

@given(a=int_lists, b=int_lists)
def test_intersection_matches_set_intersection(a, b):
    assert intersection(a, b) == set(a) & set(b)


@given(a=int_lists, b=int_lists)
def test_intersection_elements_are_in_both(a, b):
    result = intersection(a, b)
    for x in result:
        assert x in a and x in b


@given(a=int_lists, b=int_lists)
def test_intersection_is_commutative(a, b):
    assert intersection(a, b) == intersection(b, a)


@given(a=int_lists, b=int_lists)
def test_intersection_returns_set(a, b):
    assert isinstance(intersection(a, b), set)


def test_intersection_extreme_cases():
    assert intersection([], [1, 2, 3]) == set()
    assert intersection([5, 5, 5], [5, 5]) == {5}


# --- invert_dict: ключи и значения меняются местами; двойная инверсия = тождество ---

# Словари с уникальными хешируемыми значениями: ключ-int, значение-int (биекция гарантируется
# тем, что строим dict из пар с уникальными значениями).
@st.composite
def bijective_dicts(draw):
    keys = draw(st.lists(st.integers(min_value=-1000, max_value=1000), unique=True, max_size=30))
    vals = draw(st.lists(st.integers(min_value=2000, max_value=4000), unique=True, min_size=len(keys), max_size=len(keys)))
    return dict(zip(keys, vals))


@given(d=bijective_dicts())
def test_invert_dict_swaps_keys_and_values(d):
    inv = invert_dict(d)
    for k, v in d.items():
        assert inv[v] == k


@given(d=bijective_dicts())
def test_invert_dict_double_inversion_is_identity(d):
    assert invert_dict(invert_dict(d)) == d


@given(d=bijective_dicts())
def test_invert_dict_size_preserved(d):
    assert len(invert_dict(d)) == len(d)


@given(d=bijective_dicts())
def test_invert_dict_returns_dict(d):
    assert isinstance(invert_dict(d), dict)


def test_invert_dict_empty_and_mixed_types():
    assert invert_dict({}) == {}
    assert invert_dict({1: "one", 2: "two"}) == {"one": 1, "two": 2}
