# Эти тесты трогать не нужно — это эталон поведения.
# Запуск: ./python/run.sh 04

from wordfreq import word_count, top_n


# --- word_count -------------------------------------------------------------

def test_word_count_basic():
    assert word_count("a A a b") == {"a": 3, "b": 1}


def test_word_count_single_word():
    assert word_count("python") == {"python": 1}


def test_word_count_case_insensitive():
    assert word_count("Кот кот КОТ") == {"кот": 3}


def test_word_count_empty():
    assert word_count("") == {}


def test_word_count_multiple_words():
    assert word_count("один два два три три три") == {"один": 1, "два": 2, "три": 3}


def test_word_count_ignores_extra_spaces():
    # split() без аргумента схлопывает любые пробелы и игнорирует края
    assert word_count("  a   b  a ") == {"a": 2, "b": 1}


# --- top_n ------------------------------------------------------------------

def test_top_n_orders_by_frequency_desc():
    text = "три три три два два один"
    assert top_n(text, 3) == [("три", 3), ("два", 2), ("один", 1)]


def test_top_n_limit_smaller_than_unique():
    text = "три три три два два один"
    assert top_n(text, 2) == [("три", 3), ("два", 2)]


def test_top_n_tie_break_alphabetical():
    # b и a имеют одинаковую частоту 2 -> при равенстве по алфавиту: a раньше b
    text = "b b a a c"
    assert top_n(text, 3) == [("a", 2), ("b", 2), ("c", 1)]


def test_top_n_all_tie_alphabetical():
    # все по разу -> чистая сортировка по алфавиту
    text = "груша банан яблоко"
    assert top_n(text, 3) == [("банан", 1), ("груша", 1), ("яблоко", 1)]


def test_top_n_n_larger_than_unique():
    text = "a a b"
    assert top_n(text, 10) == [("a", 2), ("b", 1)]


def test_top_n_case_insensitive():
    text = "Yes yes YES no"
    assert top_n(text, 2) == [("yes", 3), ("no", 1)]


def test_top_n_empty_text():
    assert top_n("", 5) == []


def test_top_n_zero():
    assert top_n("a a b", 0) == []
