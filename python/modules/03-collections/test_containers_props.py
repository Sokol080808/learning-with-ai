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
    split_head_tail,
    sort_by_length,
    Multiset,
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


# ---------------------------------------------------------------------------
# Multiset: property tests
# ---------------------------------------------------------------------------

# Стратегия: список небольших целых (удобно для подсчёта через Counter-оракул)
small_ints = st.integers(min_value=-50, max_value=50)
small_int_lists = st.lists(small_ints, max_size=60)


# --- Инвариант: Multiset([xs]).count(x) == xs.count(x) для всех x -----------

@given(xs=small_int_lists)
def test_multiset_count_matches_list_count(xs):
    ms = Multiset(xs)
    for x in set(xs):
        assert ms.count(x) == xs.count(x)


# --- len == сумма счётчиков / сумма вхождений исходного списка ---------------

@given(xs=small_int_lists)
def test_multiset_len_equals_input_length(xs):
    assert len(Multiset(xs)) == len(xs)


# --- add увеличивает count ровно на переданное количество --------------------

@given(xs=small_int_lists, x=small_ints, k=st.integers(min_value=1, max_value=20))
def test_multiset_add_increases_by_k(xs, x, k):
    ms = Multiset(xs)
    before = ms.count(x)
    ms.add(x, count=k)
    assert ms.count(x) == before + k


# --- discard уменьшает count, не ниже нуля ----------------------------------

@given(xs=small_int_lists, x=small_ints, k=st.integers(min_value=1, max_value=20))
def test_multiset_discard_reduces_clamped(xs, x, k):
    ms = Multiset(xs)
    before = ms.count(x)
    ms.discard(x, count=k)
    expected = max(0, before - k)
    assert ms.count(x) == expected


# --- __contains__ согласован с count ----------------------------------------

@given(xs=small_int_lists, x=small_ints)
def test_multiset_contains_iff_count_positive(xs, x):
    ms = Multiset(xs)
    assert (x in ms) == (ms.count(x) > 0)


# --- most_common: счётчики убывают, множество элементов совпадает -----------

@given(xs=small_int_lists)
def test_multiset_most_common_sorted_descending(xs):
    ms = Multiset(xs)
    result = ms.most_common()
    counts = [cnt for _, cnt in result]
    assert counts == sorted(counts, reverse=True)


@given(xs=small_int_lists)
def test_multiset_most_common_all_elements_present(xs):
    ms = Multiset(xs)
    result = ms.most_common()
    # все уникальные элементы с count > 0 должны появиться
    expected_elements = {x for x in set(xs)}  # xs.count(x) > 0 всегда для элементов из xs
    result_elements = {elem for elem, _ in result}
    assert result_elements == expected_elements


@given(xs=small_int_lists, n=st.integers(min_value=0, max_value=30))
def test_multiset_most_common_n_length(xs, n):
    ms = Multiset(xs)
    unique_count = len(set(xs))
    result = ms.most_common(n)
    assert len(result) == min(n, unique_count)


# --- union: count(x) = max(a.count(x), b.count(x)) -------------------------

@given(xs=small_int_lists, ys=small_int_lists)
def test_multiset_union_is_max(xs, ys):
    a = Multiset(xs)
    b = Multiset(ys)
    u = a | b
    all_elements = set(xs) | set(ys)
    for x in all_elements:
        assert u.count(x) == max(a.count(x), b.count(x))


@given(xs=small_int_lists, ys=small_int_lists)
def test_multiset_union_len_from_max(xs, ys):
    a = Multiset(xs)
    b = Multiset(ys)
    u = a | b
    all_elements = set(xs) | set(ys)
    expected_len = sum(max(a.count(x), b.count(x)) for x in all_elements)
    assert len(u) == expected_len


@given(xs=small_int_lists, ys=small_int_lists)
def test_multiset_union_commutative(xs, ys):
    a = Multiset(xs)
    b = Multiset(ys)
    u1 = a | b
    u2 = b | a
    all_elements = set(xs) | set(ys)
    for x in all_elements:
        assert u1.count(x) == u2.count(x)


# --- intersection: count(x) = min(a.count(x), b.count(x)) ------------------

@given(xs=small_int_lists, ys=small_int_lists)
def test_multiset_intersection_is_min(xs, ys):
    a = Multiset(xs)
    b = Multiset(ys)
    i = a & b
    all_elements = set(xs) | set(ys)
    for x in all_elements:
        assert i.count(x) == min(a.count(x), b.count(x))


@given(xs=small_int_lists, ys=small_int_lists)
def test_multiset_intersection_commutative(xs, ys):
    a = Multiset(xs)
    b = Multiset(ys)
    i1 = a & b
    i2 = b & a
    all_elements = set(xs) | set(ys)
    for x in all_elements:
        assert i1.count(x) == i2.count(x)


# --- intersection <= union (поэлементно) ------------------------------------

@given(xs=small_int_lists, ys=small_int_lists)
def test_multiset_intersection_leq_union(xs, ys):
    a = Multiset(xs)
    b = Multiset(ys)
    u = a | b
    i = a & b
    all_elements = set(xs) | set(ys)
    for x in all_elements:
        assert i.count(x) <= u.count(x)


# ---------------------------------------------------------------------------
# split_head_tail: property tests
# ---------------------------------------------------------------------------

any_lists = st.lists(st.integers(min_value=-100, max_value=100), max_size=50)


@given(xs=any_lists)
@settings(derandomize=True)
def test_split_head_tail_oracle_head(xs):
    """head совпадает с xs[0], или None для пустого — оракул через срез."""
    head, _ = split_head_tail(xs)
    if xs:
        assert head == xs[0]
    else:
        assert head is None


@given(xs=any_lists)
@settings(derandomize=True)
def test_split_head_tail_oracle_tail(xs):
    """tail совпадает с xs[1:] — оракул через срез."""
    _, tail = split_head_tail(xs)
    assert tail == xs[1:]


@given(xs=any_lists)
@settings(derandomize=True)
def test_split_head_tail_reconstruct(xs):
    """Голова + хвост восстанавливают исходный список (round-trip)."""
    head, tail = split_head_tail(xs)
    if xs:
        assert [head] + tail == xs
    else:
        assert head is None and tail == []


@given(xs=any_lists)
@settings(derandomize=True)
def test_split_head_tail_tail_length(xs):
    """Длина хвоста ровно max(0, len(xs)-1)."""
    _, tail = split_head_tail(xs)
    assert len(tail) == max(0, len(xs) - 1)


# ---------------------------------------------------------------------------
# sort_by_length: property tests
# ---------------------------------------------------------------------------

word_strategy = st.text(alphabet=st.characters(whitelist_categories=("Lu", "Ll")), max_size=15)
word_list_strategy = st.lists(word_strategy, max_size=60)


@given(words=word_list_strategy)
@settings(derandomize=True)
def test_sort_by_length_oracle(words):
    """Результат совпадает с оракулом sorted(words, key=len)."""
    assert sort_by_length(words) == sorted(words, key=len)


@given(words=word_list_strategy)
@settings(derandomize=True)
def test_sort_by_length_lengths_non_decreasing(words):
    """Длины слов в результате не убывают."""
    result = sort_by_length(words)
    lengths = [len(w) for w in result]
    assert lengths == sorted(lengths)


@given(words=word_list_strategy)
@settings(derandomize=True)
def test_sort_by_length_same_multiset(words):
    """Результат содержит ровно те же слова (как мультимножество)."""
    result = sort_by_length(words)
    assert sorted(result) == sorted(words)


@given(words=word_list_strategy)
@settings(derandomize=True)
def test_sort_by_length_stable(words):
    """Стабильность: среди слов одинаковой длины исходный порядок сохраняется."""
    result = sort_by_length(words)
    # Для каждой длины слова в результате должны идти в том же порядке, что в words.
    from itertools import groupby
    # Собираем индексы исходного списка по длине
    original_order = {w: [] for w in set(words)}
    for i, w in enumerate(words):
        original_order[w].append(i)
    # В результате: для слов одинаковой длины относительный порядок (по исходным индексам)
    # должен совпадать с тем, что даёт sorted(..., key=len) — он стабилен по определению.
    assert result == sorted(words, key=len)
