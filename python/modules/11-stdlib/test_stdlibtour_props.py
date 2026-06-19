# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_stdlibtour.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — РАНДОМИЗИРОВАННЫЕ
# property-тесты: hypothesis сам подбирает сотни входов (включая злые — юникод, пустые
# списки, экстремальные даты) и проверяет не конкретный ответ, а ИНВАРИАНТЫ — свойства,
# которые обязаны выполняться для ЛЮБОГО входа. Сравниваем наши функции с эталонными
# реализациями из самой стандартной библиотеки (Counter, json, datetime).

import json
from collections import Counter
from datetime import date, timedelta

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from stdlibtour import (
    days_between,
    from_json_str,
    group_by_first_letter,
    most_common_word,
    to_json_str,
)

# Слова без пробельных символов — чтобы str.split() видел их как цельные токены.
words_st = st.text(
    alphabet=st.characters(blacklist_categories=("Cc", "Cs", "Zs", "Zl", "Zp")),
    min_size=1,
    max_size=8,
).filter(lambda w: w.split() == [w])

word_lists = st.lists(words_st, min_size=1, max_size=40)

# JSON-совместимые объекты для проверки round-trip.
json_scalars = st.one_of(
    st.none(),
    st.booleans(),
    st.integers(min_value=-10**9, max_value=10**9),
    st.floats(allow_nan=False, allow_infinity=False, min_value=-1e9, max_value=1e9),
    st.text(max_size=20),
)
json_values = st.recursive(
    json_scalars,
    lambda children: st.one_of(
        st.lists(children, max_size=5),
        st.dictionaries(st.text(max_size=10), children, max_size=5),
    ),
    max_leaves=15,
)

# Даты в широком, но безопасном для date диапазоне.
dates_st = st.dates(min_value=date(1900, 1, 1), max_value=date(2100, 12, 31))


# --- most_common_word: должен согласовываться с Counter и быть реальным максимумом ---

@given(words=word_lists)
@settings(max_examples=200, deadline=None)
def test_most_common_word_is_a_true_mode(words):
    text = " ".join(words)
    result = most_common_word(text)
    counts = Counter(words)
    top_freq = counts.most_common(1)[0][1]
    # Результат — реально встречающееся слово с максимальной частотой.
    assert result in counts
    assert counts[result] == top_freq


@given(words=word_lists)
@settings(max_examples=200, deadline=None)
def test_most_common_word_no_word_beats_the_winner(words):
    text = " ".join(words)
    winner = most_common_word(text)
    counts = Counter(words)
    # Никакое слово не встречается чаще победителя.
    assert all(counts[winner] >= freq for freq in counts.values())


@given(spaces=st.text(alphabet=" \t\n\r", max_size=20))
def test_most_common_word_blank_text_is_empty(spaces):
    # Пустой или чисто пробельный текст всегда даёт "".
    assert most_common_word(spaces) == ""


def test_most_common_word_single_repeated_word():
    assert most_common_word("zzz zzz zzz") == "zzz"


# --- group_by_first_letter: ключи, порядок и полнота относительно фильтра пустых ---

@given(words=st.lists(st.text(max_size=8), max_size=40))
@settings(max_examples=200, deadline=None)
def test_group_by_first_letter_invariants(words):
    result = group_by_first_letter(words)
    nonempty = [w for w in words if w]

    # 1) Каждый ключ — это первая буква, а каждое слово в группе реально с неё начинается.
    for key, group in result.items():
        assert all(w[0] == key for w in group)
        assert all(len(key) == 1 for _ in [key])

    # 2) Сплющивание групп возвращает ровно непустые слова в ИСХОДНОМ порядке.
    flattened_by_key = {w[0]: [] for w in nonempty}
    for w in nonempty:
        flattened_by_key[w[0]].append(w)
    assert result == flattened_by_key

    # 3) Пустые строки полностью отброшены — общее число элементов совпадает.
    assert sum(len(g) for g in result.values()) == len(nonempty)


@given(words=st.lists(st.text(min_size=1, max_size=8), min_size=1, max_size=40))
@settings(max_examples=200, deadline=None)
def test_group_by_first_letter_preserves_order(words):
    result = group_by_first_letter(words)
    for key, group in result.items():
        expected = [w for w in words if w and w[0] == key]
        assert group == expected


def test_group_by_first_letter_empty_input_is_empty_dict():
    assert group_by_first_letter([]) == {}


def test_group_by_first_letter_all_empty_strings():
    assert group_by_first_letter(["", "", ""]) == {}


# --- json: to/from обратны друг другу и согласуются со стандартным json ---

@given(obj=json_values)
@settings(max_examples=200, deadline=None)
def test_json_roundtrip_identity(obj):
    # from_json_str(to_json_str(x)) == x для любого JSON-совместимого x.
    assert from_json_str(to_json_str(obj)) == obj


@given(obj=json_values)
@settings(max_examples=200, deadline=None)
def test_to_json_str_agrees_with_stdlib(obj):
    # Наша строка должна разбираться штатным json и давать тот же объект.
    assert json.loads(to_json_str(obj)) == obj


@given(obj=json_values)
@settings(max_examples=200, deadline=None)
def test_from_json_str_agrees_with_stdlib(obj):
    # Парсинг должен совпадать с json.loads на канонической строке.
    s = json.dumps(obj)
    assert from_json_str(s) == json.loads(s)


# --- days_between: законы арифметики дат, согласие с datetime ---

@given(a=dates_st, b=dates_st)
@settings(max_examples=200, deadline=None)
def test_days_between_matches_timedelta(a, b):
    # Должно совпадать с настоящим вычитанием date.
    assert days_between(a.isoformat(), b.isoformat()) == (b - a).days


@given(a=dates_st, b=dates_st)
@settings(max_examples=200, deadline=None)
def test_days_between_is_antisymmetric(a, b):
    # days_between(a, b) == -days_between(b, a) — знак направленный.
    forward = days_between(a.isoformat(), b.isoformat())
    backward = days_between(b.isoformat(), a.isoformat())
    assert forward == -backward


@given(d=dates_st)
def test_days_between_same_date_is_zero(d):
    assert days_between(d.isoformat(), d.isoformat()) == 0


@given(d=dates_st, k=st.integers(min_value=0, max_value=10000))
@settings(max_examples=200, deadline=None)
def test_days_between_counts_added_days(d, k):
    # Если сдвинуть дату ровно на k дней вперёд (оставаясь в диапазоне), разница == k.
    later = d + timedelta(days=k)
    if later <= date(2100, 12, 31):
        assert days_between(d.isoformat(), later.isoformat()) == k


def test_days_between_rejects_non_iso_format():
    # fromisoformat строгий — не-ISO формат должен поднимать ValueError.
    with pytest.raises(ValueError):
        days_between("31/12/2024", "2024-12-31")
