# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_wordfreq.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — РАНДОМИЗИРОВАННЫЕ
# property-тесты: hypothesis сам подбирает сотни входов (включая злые — пустой текст,
# лишние пробелы, смешанный регистр, юникод) и проверяет ИНВАРИАНТЫ, обязательные для
# ЛЮБОГО входа: сумма частот = числу токенов, сортировка по (-частота, слово) и т.д.

from hypothesis import given, settings
from hypothesis import strategies as st

from wordfreq import word_count, top_n

# Слова — из небольшого алфавита (включая разный регистр), чтобы возникали повторы и ничьи.
word = st.text(alphabet="abcAB кот ", min_size=0, max_size=6)
texts = st.lists(word, max_size=30).map(lambda ws: " ".join(ws))


# --- word_count: сумма частот = числу токенов; ключи в нижнем регистре ---

@given(text=texts)
def test_word_count_sum_equals_token_count(text):
    counts = word_count(text)
    assert sum(counts.values()) == len(text.lower().split())


@given(text=texts)
def test_word_count_keys_are_lowercase_tokens(text):
    counts = word_count(text)
    assert set(counts.keys()) == set(text.lower().split())


@given(text=texts)
def test_word_count_all_counts_positive(text):
    counts = word_count(text)
    assert all(v >= 1 for v in counts.values())


@given(text=texts)
def test_word_count_case_insensitive(text):
    # Регистр не влияет: верх/низ дают одинаковый словарь.
    assert word_count(text) == word_count(text.upper())


@given(text=texts)
def test_word_count_returns_dict(text):
    assert isinstance(word_count(text), dict)


def test_word_count_extreme_cases():
    assert word_count("") == {}
    assert word_count("  a   b  a ") == {"a": 2, "b": 1}
    assert word_count("Кот кот КОТ") == {"кот": 3}


# --- top_n: верхушка частотного словаря по правилу (-частота, слово) ---

@given(text=texts, n=st.integers(min_value=0, max_value=50))
def test_top_n_matches_sorted_rule(text, n):
    # Сильная проверка корректности: ровно n первых из sorted(counts, key=(-freq, word)).
    counts = word_count(text)
    expected = sorted(counts.items(), key=lambda kv: (-kv[1], kv[0]))[:n]
    assert top_n(text, n) == expected


@given(text=texts, n=st.integers(min_value=0, max_value=50))
def test_top_n_length_bounded(text, n):
    counts = word_count(text)
    result = top_n(text, n)
    assert len(result) == min(n, len(counts))


@given(text=texts, n=st.integers(min_value=1, max_value=50))
def test_top_n_frequencies_non_increasing(text, n):
    result = top_n(text, n)
    freqs = [freq for _, freq in result]
    assert freqs == sorted(freqs, reverse=True)


@given(text=texts, n=st.integers(min_value=1, max_value=50))
def test_top_n_ties_alphabetical(text, n):
    # При равной частоте слова идут по алфавиту (возрастание).
    result = top_n(text, n)
    for (w1, f1), (w2, f2) in zip(result, result[1:]):
        if f1 == f2:
            assert w1 <= w2


@given(text=texts, n=st.integers(min_value=0, max_value=50))
def test_top_n_pairs_consistent_with_word_count(text, n):
    counts = word_count(text)
    for w, freq in top_n(text, n):
        assert counts[w] == freq


def test_top_n_extreme_cases():
    assert top_n("", 5) == []
    assert top_n("a a b", 0) == []
    assert top_n("b b a a c", 3) == [("a", 2), ("b", 2), ("c", 1)]
    assert top_n("a a b", 10) == [("a", 2), ("b", 1)]
