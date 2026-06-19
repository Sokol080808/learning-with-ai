# Тесты для RecordAggregator — задание «существенное» модуля 11.
# Трогать НЕ нужно — это эталон поведения. Реализуй класс в stdlibtour.py.

import json

import pytest

from stdlibtour import RecordAggregator


# ---------------------------------------------------------------------------
# Вспомогательные фикстуры
# ---------------------------------------------------------------------------

RECORDS = [
    {"date": "2024-01-05", "category": "food",    "value": 120.0},
    {"date": "2024-01-05", "category": "food",    "value":  80.0},
    {"date": "2024-01-06", "category": "travel",  "value": 350.0},
    {"date": "2024-01-07", "category": "food",    "value":  60.0},
    {"date": "2024-01-07", "category": "health",  "value": 200.0},
    {"date": "2024-01-08", "category": "travel",  "value": 150.0},
]


@pytest.fixture
def agg_with_records() -> RecordAggregator:
    agg = RecordAggregator()
    for r in RECORDS:
        agg.add(r)
    return agg


# ---------------------------------------------------------------------------
# Пустой агрегатор
# ---------------------------------------------------------------------------

def test_empty_aggregator_count_is_zero():
    agg = RecordAggregator()
    assert agg.summary()["count"] == 0


def test_empty_aggregator_total_value_is_zero():
    agg = RecordAggregator()
    assert agg.summary()["total_value"] == 0.0


def test_empty_aggregator_min_max_dates_are_none():
    agg = RecordAggregator()
    s = agg.summary()
    assert s["min_value"] is None
    assert s["max_value"] is None
    assert s["first_date"] is None
    assert s["last_date"] is None


def test_empty_aggregator_categories_empty():
    agg = RecordAggregator()
    assert agg.summary()["categories"] == {}


def test_empty_aggregator_top_categories_empty():
    agg = RecordAggregator()
    assert agg.top_categories() == []


def test_empty_aggregator_daily_totals_empty():
    agg = RecordAggregator()
    assert agg.daily_totals() == {}


# ---------------------------------------------------------------------------
# Добавление одной записи
# ---------------------------------------------------------------------------

def test_single_record_count():
    agg = RecordAggregator()
    agg.add({"date": "2024-03-15", "category": "misc", "value": 42})
    assert agg.summary()["count"] == 1


def test_single_record_total_value():
    agg = RecordAggregator()
    agg.add({"date": "2024-03-15", "category": "misc", "value": 42})
    assert agg.summary()["total_value"] == 42.0


def test_single_record_min_max_value_equal():
    agg = RecordAggregator()
    agg.add({"date": "2024-03-15", "category": "misc", "value": 7.5})
    s = agg.summary()
    assert s["min_value"] == 7.5
    assert s["max_value"] == 7.5


def test_single_record_first_last_date_equal():
    agg = RecordAggregator()
    agg.add({"date": "2024-03-15", "category": "misc", "value": 1})
    s = agg.summary()
    assert s["first_date"] == "2024-03-15"
    assert s["last_date"] == "2024-03-15"


def test_single_record_category_count_is_one():
    agg = RecordAggregator()
    agg.add({"date": "2024-03-15", "category": "food", "value": 10})
    assert agg.summary()["categories"]["food"] == 1


# ---------------------------------------------------------------------------
# summary() со множеством записей
# ---------------------------------------------------------------------------

def test_summary_count(agg_with_records):
    assert agg_with_records.summary()["count"] == 6


def test_summary_total_value(agg_with_records):
    # 120+80+350+60+200+150 = 960
    assert agg_with_records.summary()["total_value"] == pytest.approx(960.0)


def test_summary_min_value(agg_with_records):
    assert agg_with_records.summary()["min_value"] == pytest.approx(60.0)


def test_summary_max_value(agg_with_records):
    assert agg_with_records.summary()["max_value"] == pytest.approx(350.0)


def test_summary_first_date(agg_with_records):
    assert agg_with_records.summary()["first_date"] == "2024-01-05"


def test_summary_last_date(agg_with_records):
    assert agg_with_records.summary()["last_date"] == "2024-01-08"


def test_summary_category_counts(agg_with_records):
    cats = agg_with_records.summary()["categories"]
    assert cats["food"] == 3
    assert cats["travel"] == 2
    assert cats["health"] == 1


def test_summary_has_required_keys(agg_with_records):
    required = {"count", "categories", "total_value", "min_value", "max_value",
                "first_date", "last_date"}
    assert required <= set(agg_with_records.summary().keys())


# ---------------------------------------------------------------------------
# top_categories()
# ---------------------------------------------------------------------------

def test_top_categories_order(agg_with_records):
    top = agg_with_records.top_categories(2)
    # food(3) > travel(2)
    assert top[0] == ("food", 3)
    assert top[1] == ("travel", 2)


def test_top_categories_length_capped(agg_with_records):
    top = agg_with_records.top_categories(1)
    assert len(top) == 1
    assert top[0][0] == "food"


def test_top_categories_n_zero(agg_with_records):
    assert agg_with_records.top_categories(0) == []


def test_top_categories_n_larger_than_categories(agg_with_records):
    # 3 категории, просим 10 — вернёт все 3
    top = agg_with_records.top_categories(10)
    assert len(top) == 3


def test_top_categories_returns_list_of_tuples(agg_with_records):
    top = agg_with_records.top_categories(2)
    assert isinstance(top, list)
    assert all(isinstance(t, tuple) and len(t) == 2 for t in top)


# ---------------------------------------------------------------------------
# daily_totals()
# ---------------------------------------------------------------------------

def test_daily_totals_keys(agg_with_records):
    dt = agg_with_records.daily_totals()
    assert set(dt.keys()) == {"2024-01-05", "2024-01-06", "2024-01-07", "2024-01-08"}


def test_daily_totals_values(agg_with_records):
    dt = agg_with_records.daily_totals()
    assert dt["2024-01-05"] == pytest.approx(200.0)  # 120+80
    assert dt["2024-01-06"] == pytest.approx(350.0)
    assert dt["2024-01-07"] == pytest.approx(260.0)  # 60+200
    assert dt["2024-01-08"] == pytest.approx(150.0)


def test_daily_totals_sum_equals_total(agg_with_records):
    dt = agg_with_records.daily_totals()
    total = agg_with_records.summary()["total_value"]
    assert sum(dt.values()) == pytest.approx(total)


def test_daily_totals_single_day():
    agg = RecordAggregator()
    agg.add({"date": "2024-06-01", "category": "x", "value": 5})
    agg.add({"date": "2024-06-01", "category": "y", "value": 3})
    assert agg.daily_totals() == {"2024-06-01": pytest.approx(8.0)}


# ---------------------------------------------------------------------------
# Валидация при add()
# ---------------------------------------------------------------------------

def test_add_rejects_bad_date_format():
    agg = RecordAggregator()
    with pytest.raises((ValueError, KeyError)):
        agg.add({"date": "05/01/2024", "category": "food", "value": 10})


def test_add_rejects_empty_category():
    agg = RecordAggregator()
    with pytest.raises((ValueError, TypeError)):
        agg.add({"date": "2024-01-01", "category": "", "value": 10})


def test_add_rejects_non_numeric_value():
    agg = RecordAggregator()
    with pytest.raises((ValueError, TypeError)):
        agg.add({"date": "2024-01-01", "category": "food", "value": "много"})


def test_add_rejects_missing_fields():
    agg = RecordAggregator()
    with pytest.raises((KeyError, TypeError, ValueError)):
        agg.add({"date": "2024-01-01", "category": "food"})  # нет value


def test_add_accepts_int_value():
    agg = RecordAggregator()
    agg.add({"date": "2024-01-01", "category": "shop", "value": 100})
    assert agg.summary()["count"] == 1


def test_add_accepts_negative_value():
    agg = RecordAggregator()
    agg.add({"date": "2024-01-01", "category": "refund", "value": -50.0})
    assert agg.summary()["min_value"] == pytest.approx(-50.0)


# ---------------------------------------------------------------------------
# to_json() / from_json() round-trip
# ---------------------------------------------------------------------------

def test_to_json_returns_string(agg_with_records):
    assert isinstance(agg_with_records.to_json(), str)


def test_to_json_is_valid_json(agg_with_records):
    s = agg_with_records.to_json()
    parsed = json.loads(s)  # не должно кидать
    assert isinstance(parsed, (dict, list))


def test_json_roundtrip_summary(agg_with_records):
    restored = RecordAggregator.from_json(agg_with_records.to_json())
    assert restored.summary() == agg_with_records.summary()


def test_json_roundtrip_daily_totals(agg_with_records):
    restored = RecordAggregator.from_json(agg_with_records.to_json())
    original_dt = agg_with_records.daily_totals()
    restored_dt = restored.daily_totals()
    assert set(restored_dt.keys()) == set(original_dt.keys())
    for k in original_dt:
        assert restored_dt[k] == pytest.approx(original_dt[k])


def test_json_roundtrip_empty_aggregator():
    agg = RecordAggregator()
    restored = RecordAggregator.from_json(agg.to_json())
    assert restored.summary() == agg.summary()
    assert restored.daily_totals() == {}


def test_json_roundtrip_can_add_after_restore(agg_with_records):
    restored = RecordAggregator.from_json(agg_with_records.to_json())
    count_before = restored.summary()["count"]
    restored.add({"date": "2024-02-01", "category": "new", "value": 1})
    assert restored.summary()["count"] == count_before + 1
