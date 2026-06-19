# Property-тесты для RecordAggregator — задание «существенное» модуля 11.
# hypothesis генерирует сотни входов; derandomize=True в conftest.py делает их воспроизводимыми.
# Трогать не нужно — реализуй класс в stdlibtour.py.

from collections import Counter
from datetime import date, timedelta

import pytest
from hypothesis import assume, given, settings
from hypothesis import strategies as st

from stdlibtour import RecordAggregator

# ---------------------------------------------------------------------------
# Стратегии
# ---------------------------------------------------------------------------

iso_dates_st = st.dates(
    min_value=date(2000, 1, 1),
    max_value=date(2099, 12, 31),
).map(lambda d: d.isoformat())

categories_st = st.text(
    alphabet="abcdefghijklmnopqrstuvwxyz",
    min_size=1,
    max_size=12,
)

values_st = st.one_of(
    st.integers(min_value=-10_000, max_value=10_000),
    st.floats(
        min_value=-10_000.0,
        max_value=10_000.0,
        allow_nan=False,
        allow_infinity=False,
    ),
)

record_st = st.fixed_dictionaries(
    {
        "date":     iso_dates_st,
        "category": categories_st,
        "value":    values_st,
    }
)

record_list_st = st.lists(record_st, min_size=1, max_size=50)


# ---------------------------------------------------------------------------
# Инварианты summary()
# ---------------------------------------------------------------------------

@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_summary_count_equals_number_of_added_records(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    assert agg.summary()["count"] == len(records)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_summary_total_value_matches_sum(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    expected = sum(float(r["value"]) for r in records)
    assert agg.summary()["total_value"] == pytest.approx(expected, rel=1e-6)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_summary_min_max_value_correct(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    s = agg.summary()
    values = [float(r["value"]) for r in records]
    assert s["min_value"] == pytest.approx(min(values), rel=1e-9)
    assert s["max_value"] == pytest.approx(max(values), rel=1e-9)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_summary_first_last_date_are_min_max(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    s = agg.summary()
    dates = [r["date"] for r in records]
    # ISO-строки сортируются лексикографически так же, как даты
    assert s["first_date"] == min(dates)
    assert s["last_date"] == max(dates)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_summary_categories_match_counter(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    expected_counts = Counter(r["category"] for r in records)
    assert agg.summary()["categories"] == dict(expected_counts)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_summary_category_counts_sum_to_total(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    s = agg.summary()
    assert sum(s["categories"].values()) == s["count"]


# ---------------------------------------------------------------------------
# Инварианты top_categories()
# ---------------------------------------------------------------------------

@given(records=record_list_st, n=st.integers(min_value=0, max_value=20))
@settings(max_examples=200, deadline=None)
def test_top_categories_length_at_most_n(records, n):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    top = agg.top_categories(n)
    assert len(top) <= n


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_top_categories_sorted_descending(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    top = agg.top_categories(len(records))  # достаточно большой запрос
    counts = [cnt for _, cnt in top]
    assert counts == sorted(counts, reverse=True)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_top_categories_winner_has_highest_count(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    top = agg.top_categories(1)
    if top:
        winner_name, winner_cnt = top[0]
        cats = agg.summary()["categories"]
        assert winner_cnt == max(cats.values())


# ---------------------------------------------------------------------------
# Инварианты daily_totals()
# ---------------------------------------------------------------------------

@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_daily_totals_keys_are_iso_dates(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    for key in agg.daily_totals():
        # fromisoformat не должен кидать — это и есть проверка формата
        date.fromisoformat(key)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_daily_totals_grand_sum_equals_total_value(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    grand = sum(agg.daily_totals().values())
    total = agg.summary()["total_value"]
    assert grand == pytest.approx(total, rel=1e-6)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_daily_totals_keys_match_unique_dates(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    expected_keys = {r["date"] for r in records}
    assert set(agg.daily_totals().keys()) == expected_keys


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_daily_totals_individual_day_sum_correct(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    dt = agg.daily_totals()
    # Проверяем каждый день вручную
    from collections import defaultdict
    expected: dict = defaultdict(float)
    for r in records:
        expected[r["date"]] += float(r["value"])
    for d_key, exp_val in expected.items():
        assert dt[d_key] == pytest.approx(exp_val, rel=1e-6)


# ---------------------------------------------------------------------------
# JSON round-trip
# ---------------------------------------------------------------------------

@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_json_roundtrip_summary_identical(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    restored = RecordAggregator.from_json(agg.to_json())
    original_s = agg.summary()
    restored_s = restored.summary()

    assert restored_s["count"] == original_s["count"]
    assert restored_s["first_date"] == original_s["first_date"]
    assert restored_s["last_date"] == original_s["last_date"]
    assert restored_s["total_value"] == pytest.approx(original_s["total_value"], rel=1e-6)
    assert restored_s["categories"] == original_s["categories"]


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_json_roundtrip_daily_totals_identical(records):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    restored = RecordAggregator.from_json(agg.to_json())
    original_dt = agg.daily_totals()
    restored_dt = restored.daily_totals()
    assert set(restored_dt.keys()) == set(original_dt.keys())
    for k in original_dt:
        assert restored_dt[k] == pytest.approx(original_dt[k], rel=1e-6)


@given(records=record_list_st)
@settings(max_examples=200, deadline=None)
def test_json_is_idempotent(records):
    """from_json(to_json(from_json(to_json(agg)))) должен давать тот же summary."""
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    s1 = agg.to_json()
    r1 = RecordAggregator.from_json(s1)
    s2 = r1.to_json()
    r2 = RecordAggregator.from_json(s2)
    assert r2.summary()["count"] == agg.summary()["count"]
    assert r2.summary()["categories"] == agg.summary()["categories"]


# ---------------------------------------------------------------------------
# Монотонность: add() наращивает счётчик
# ---------------------------------------------------------------------------

@given(records=record_list_st, extra=record_st)
@settings(max_examples=200, deadline=None)
def test_add_increments_count(records, extra):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    count_before = agg.summary()["count"]
    agg.add(extra)
    assert agg.summary()["count"] == count_before + 1


@given(records=record_list_st, extra=record_st)
@settings(max_examples=200, deadline=None)
def test_add_updates_total_value(records, extra):
    agg = RecordAggregator()
    for r in records:
        agg.add(r)
    total_before = agg.summary()["total_value"]
    agg.add(extra)
    assert agg.summary()["total_value"] == pytest.approx(
        total_before + float(extra["value"]), rel=1e-6
    )
