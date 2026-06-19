# Эти тесты трогать не нужно — это эталон поведения.
#
# Они описывают, КАК должны вести себя функции из containers.py. Сейчас они красные
# (стаб кидает NotImplementedError) — твоя задача сделать их зелёными, реализовав функции.

from containers import (
    unique_sorted,
    word_lengths,
    flatten,
    intersection,
    invert_dict,
    Multiset,
)


# --- unique_sorted -----------------------------------------------------------

def test_unique_sorted_removes_dups_and_sorts():
    assert unique_sorted([3, 1, 2, 3, 1]) == [1, 2, 3]


def test_unique_sorted_already_sorted_unique():
    assert unique_sorted([1, 2, 3]) == [1, 2, 3]


def test_unique_sorted_reversed_input():
    assert unique_sorted([5, 4, 3, 2, 1]) == [1, 2, 3, 4, 5]


def test_unique_sorted_with_negatives():
    assert unique_sorted([0, -2, 5, -2, 0, 3]) == [-2, 0, 3, 5]


def test_unique_sorted_empty():
    assert unique_sorted([]) == []


def test_unique_sorted_single():
    assert unique_sorted([7]) == [7]


def test_unique_sorted_all_same():
    assert unique_sorted([4, 4, 4, 4]) == [4]


def test_unique_sorted_returns_list():
    result = unique_sorted([2, 1])
    assert isinstance(result, list)


# --- word_lengths ------------------------------------------------------------

def test_word_lengths_basic():
    assert word_lengths(["кот", "пёс"]) == {"кот": 3, "пёс": 3}


def test_word_lengths_varied():
    assert word_lengths(["a", "bb", "ccc"]) == {"a": 1, "bb": 2, "ccc": 3}


def test_word_lengths_empty_string_has_zero_length():
    assert word_lengths([""]) == {"": 0}


def test_word_lengths_empty_list():
    assert word_lengths([]) == {}


def test_word_lengths_duplicate_word_kept_once():
    # Одинаковые слова — это один ключ; результат всё равно корректная длина.
    assert word_lengths(["ok", "ok"]) == {"ok": 2}


def test_word_lengths_returns_dict():
    assert isinstance(word_lengths(["x"]), dict)


# --- flatten -----------------------------------------------------------------

def test_flatten_basic():
    assert flatten([[1, 2], [3], [4, 5]]) == [1, 2, 3, 4, 5]


def test_flatten_keeps_order_and_duplicates():
    assert flatten([[1, 1], [2], [1]]) == [1, 1, 2, 1]


def test_flatten_with_empty_rows():
    assert flatten([[], [1], [], [2, 3], []]) == [1, 2, 3]


def test_flatten_single_row():
    assert flatten([[10, 20, 30]]) == [10, 20, 30]


def test_flatten_empty_matrix():
    assert flatten([]) == []


def test_flatten_all_empty_rows():
    assert flatten([[], [], []]) == []


def test_flatten_returns_list():
    assert isinstance(flatten([[1]]), list)


# --- intersection ------------------------------------------------------------

def test_intersection_basic():
    assert intersection([1, 2, 2, 3], [2, 3, 4]) == {2, 3}


def test_intersection_no_common():
    assert intersection([1, 2], [3, 4]) == set()


def test_intersection_identical():
    assert intersection([1, 2, 3], [3, 2, 1]) == {1, 2, 3}


def test_intersection_with_empty():
    assert intersection([], [1, 2, 3]) == set()
    assert intersection([1, 2, 3], []) == set()


def test_intersection_result_is_unique():
    # Дубликаты в исходных списках не должны размножаться в результате.
    assert intersection([5, 5, 5], [5, 5]) == {5}


def test_intersection_returns_set():
    assert isinstance(intersection([1], [1]), set)


# --- invert_dict -------------------------------------------------------------

def test_invert_dict_basic():
    assert invert_dict({"a": 1, "b": 2}) == {1: "a", 2: "b"}


def test_invert_dict_empty():
    assert invert_dict({}) == {}


def test_invert_dict_int_keys_str_values():
    assert invert_dict({1: "one", 2: "two"}) == {"one": 1, "two": 2}


def test_invert_dict_single_pair():
    assert invert_dict({"only": 42}) == {42: "only"}


def test_invert_dict_roundtrip():
    # Инверсия инверсии возвращает исходный словарь (при уникальных значениях).
    original = {"x": 10, "y": 20, "z": 30}
    assert invert_dict(invert_dict(original)) == original


def test_invert_dict_returns_dict():
    assert isinstance(invert_dict({"k": "v"}), dict)


# --- Multiset: init & count --------------------------------------------------

def test_multiset_empty_init():
    ms = Multiset()
    assert len(ms) == 0


def test_multiset_init_from_iterable():
    ms = Multiset([1, 1, 2, 3])
    assert ms.count(1) == 2
    assert ms.count(2) == 1
    assert ms.count(3) == 1
    assert ms.count(99) == 0


def test_multiset_init_empty_iterable():
    ms = Multiset([])
    assert len(ms) == 0


def test_multiset_count_absent_element():
    ms = Multiset([10, 20])
    assert ms.count(999) == 0


# --- Multiset: add -----------------------------------------------------------

def test_multiset_add_increases_count():
    ms = Multiset()
    ms.add("a")
    ms.add("a")
    ms.add("b")
    assert ms.count("a") == 2
    assert ms.count("b") == 1


def test_multiset_add_with_count_arg():
    ms = Multiset()
    ms.add("x", count=5)
    assert ms.count("x") == 5


def test_multiset_add_invalid_count_raises():
    ms = Multiset()
    import pytest as _pytest
    with _pytest.raises(ValueError):
        ms.add("z", count=0)
    with _pytest.raises(ValueError):
        ms.add("z", count=-3)


# --- Multiset: discard -------------------------------------------------------

def test_multiset_discard_reduces_count():
    ms = Multiset([1, 1, 1])
    ms.discard(1)
    assert ms.count(1) == 2


def test_multiset_discard_clamps_to_zero():
    ms = Multiset([7, 7])
    ms.discard(7, count=10)   # больше чем есть — не падает, просто ноль
    assert ms.count(7) == 0
    assert 7 not in ms


def test_multiset_discard_absent_element_no_error():
    ms = Multiset()
    ms.discard(42)   # должен пройти без исключений
    assert ms.count(42) == 0


def test_multiset_discard_invalid_count_raises():
    ms = Multiset([1])
    import pytest as _pytest
    with _pytest.raises(ValueError):
        ms.discard(1, count=0)


# --- Multiset: __len__ and __contains__ --------------------------------------

def test_multiset_len_is_total_count():
    ms = Multiset([1, 1, 2, 3, 3, 3])
    assert len(ms) == 6


def test_multiset_contains_true():
    ms = Multiset(["hello", "world", "world"])
    assert "hello" in ms
    assert "world" in ms


def test_multiset_contains_false_after_discard():
    ms = Multiset([5])
    ms.discard(5)
    assert 5 not in ms


def test_multiset_contains_never_seen():
    ms = Multiset([1, 2])
    assert 99 not in ms


# --- Multiset: most_common ---------------------------------------------------

def test_multiset_most_common_all():
    ms = Multiset([1, 1, 1, 2, 2, 3])
    result = ms.most_common()
    # первый — с наибольшим числом вхождений
    assert result[0] == (1, 3)
    assert result[1] == (2, 2)
    assert result[2] == (3, 1)


def test_multiset_most_common_n():
    ms = Multiset([10, 10, 10, 20, 20, 30])
    top2 = ms.most_common(2)
    assert len(top2) == 2
    assert top2[0] == (10, 3)
    assert top2[1] == (20, 2)


def test_multiset_most_common_zero():
    ms = Multiset([1, 2, 3])
    assert ms.most_common(0) == []


def test_multiset_most_common_n_exceeds_size():
    ms = Multiset([7, 7])
    result = ms.most_common(100)
    assert len(result) == 1
    assert result[0] == (7, 2)


def test_multiset_most_common_excludes_zero_count():
    ms = Multiset([4, 4, 5])
    ms.discard(5)   # убрали — счётчик стал 0
    result = ms.most_common()
    assert all(cnt > 0 for _, cnt in result)
    assert (5, 0) not in result


# --- Multiset: union (|) -----------------------------------------------------

def test_multiset_union_takes_max():
    a = Multiset([1, 1, 2])
    b = Multiset([1, 2, 2, 3])
    u = a | b
    assert u.count(1) == 2   # max(2, 1)
    assert u.count(2) == 2   # max(1, 2)
    assert u.count(3) == 1   # max(0, 1)


def test_multiset_union_with_empty():
    ms = Multiset([1, 2, 2])
    result = ms | Multiset()
    assert result.count(1) == 1
    assert result.count(2) == 2


def test_multiset_union_returns_new_multiset():
    a = Multiset([1])
    b = Multiset([1])
    u = a | b
    assert isinstance(u, Multiset)
    # исходные не меняются
    assert a.count(1) == 1
    assert b.count(1) == 1


# --- Multiset: intersection (&) ----------------------------------------------

def test_multiset_intersection_takes_min():
    a = Multiset([1, 1, 1, 2, 2])
    b = Multiset([1, 1, 2, 2, 2, 3])
    i = a & b
    assert i.count(1) == 2   # min(3, 2)
    assert i.count(2) == 2   # min(2, 3)
    assert i.count(3) == 0   # min(0, 1) → не включается


def test_multiset_intersection_disjoint():
    a = Multiset([1, 2])
    b = Multiset([3, 4])
    assert len(a & b) == 0


def test_multiset_intersection_with_empty():
    ms = Multiset([1, 1, 2])
    result = ms & Multiset()
    assert len(result) == 0


def test_multiset_intersection_returns_new_multiset():
    a = Multiset([5, 5])
    b = Multiset([5])
    result = a & b
    assert isinstance(result, Multiset)
    assert result.count(5) == 1
