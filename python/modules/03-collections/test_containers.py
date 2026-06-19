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
