# Property-тесты модуля 17 «Метапрограммирование».
#
# hypothesis генерит И валидные, И невалидные значения — проверяем инварианты дескрипторов
# и реестра, а не конкретные числа. Явный @settings(derandomize=True): один и тот же набор
# примеров на каждом прогоне, тест не «мигает».
import math

import pytest
from hypothesis import assume, given, settings
from hypothesis import strategies as st

from meta import (
    DefaultDict,
    Plugin,
    Positive,
    Ranged,
    Typed,
)

# ---------------------------------------------------------------------------
# Стратегии
# ---------------------------------------------------------------------------
finite_floats = st.floats(allow_nan=False, allow_infinity=False, width=64)
positive_numbers = st.one_of(
    st.integers(min_value=1, max_value=10**9),
    st.floats(min_value=1e-6, max_value=1e9, allow_nan=False, allow_infinity=False),
)
non_positive_numbers = st.one_of(
    st.integers(max_value=0),
    st.floats(max_value=0.0, allow_nan=False, allow_infinity=False),
)
non_numbers = st.one_of(
    st.text(),
    st.none(),
    st.lists(st.integers()),
    st.booleans(),  # bool считаем «не числом» для числовых дескрипторов
)


def _make_positive_holder():
    class Holder:
        value = Positive()

    return Holder()


# ===========================================================================
# Positive: принимает валидные, отвергает невалидные
# ===========================================================================

@given(x=positive_numbers)
@settings(derandomize=True)
def test_positive_accepts_all_positive(x):
    h = _make_positive_holder()
    h.value = x
    assert h.value == x  # принято и сохранено без изменений


@given(x=non_positive_numbers)
@settings(derandomize=True)
def test_positive_rejects_non_positive(x):
    assume(not isinstance(x, bool))
    h = _make_positive_holder()
    with pytest.raises(ValueError):
        h.value = x


@given(x=non_numbers)
@settings(derandomize=True)
def test_positive_rejects_non_numbers_with_typeerror(x):
    h = _make_positive_holder()
    with pytest.raises(TypeError):
        h.value = x


@given(x=positive_numbers)
@settings(derandomize=True)
def test_positive_rejected_value_not_stored(x):
    # После отказа состояние не должно меняться: кладём валидное, потом пытаемся плохое.
    h = _make_positive_holder()
    h.value = x
    with pytest.raises((ValueError, TypeError)):
        h.value = -abs(x) - 1  # гарантированно <= 0
    assert h.value == x  # старое значение уцелело


# ===========================================================================
# Per-instance независимость хранения
# ===========================================================================

@given(a=positive_numbers, b=positive_numbers)
@settings(derandomize=True)
def test_positive_per_instance_independent(a, b):
    class Holder:
        value = Positive()

    h1, h2 = Holder(), Holder()
    h1.value = a
    h2.value = b
    assert h1.value == a
    assert h2.value == b  # изменение h2 не задело h1


@given(a=positive_numbers, b=positive_numbers)
@settings(derandomize=True)
def test_positive_two_fields_independent(a, b):
    class Holder:
        x = Positive()
        y = Positive()

    h = Holder()
    h.x = a
    h.y = b
    assert h.x == a
    assert h.y == b  # два дескриптора на одном инстансе не путают значения


# ===========================================================================
# Ranged: диапазон (0, maximum]
# ===========================================================================

@st.composite
def in_range_pairs(draw):
    maximum = draw(st.floats(min_value=1e-3, max_value=1e9, allow_nan=False, allow_infinity=False))
    value = draw(st.floats(min_value=1e-9, max_value=maximum, allow_nan=False, allow_infinity=False))
    return value, maximum


@st.composite
def above_range_pairs(draw):
    maximum = draw(st.floats(min_value=1e-3, max_value=1e6, allow_nan=False, allow_infinity=False))
    value = draw(st.floats(min_value=maximum, max_value=1e12, allow_nan=False, allow_infinity=False))
    assume(value > maximum)
    return value, maximum


@given(pair=in_range_pairs())
@settings(derandomize=True)
def test_ranged_accepts_in_range(pair):
    value, maximum = pair

    class Holder:
        v = Ranged(maximum=maximum)

    h = Holder()
    h.v = value
    assert h.v == value


@given(pair=above_range_pairs())
@settings(derandomize=True)
def test_ranged_rejects_above_max(pair):
    value, maximum = pair

    class Holder:
        v = Ranged(maximum=maximum)

    h = Holder()
    with pytest.raises(ValueError):
        h.v = value


@given(x=non_positive_numbers)
@settings(derandomize=True)
def test_ranged_rejects_non_positive(x):
    assume(not isinstance(x, bool))

    class Holder:
        v = Ranged(maximum=1e9)

    h = Holder()
    with pytest.raises(ValueError):
        h.v = x


# ===========================================================================
# Typed: тип, и bool-ловушка для int
# ===========================================================================

@given(s=st.text())
@settings(derandomize=True)
def test_typed_str_accepts_strings(s):
    class Holder:
        v = Typed(str)

    h = Holder()
    h.v = s
    assert h.v == s


@given(n=st.integers())
@settings(derandomize=True)
def test_typed_int_rejects_nothing_but_accepts_ints(n):
    class Holder:
        v = Typed(int)

    h = Holder()
    h.v = n
    assert h.v == n


@given(b=st.booleans())
@settings(derandomize=True)
def test_typed_int_always_rejects_bool(b):
    class Holder:
        v = Typed(int)

    h = Holder()
    with pytest.raises(TypeError):
        h.v = b  # bool недопустим для Typed(int) при любом значении


# ===========================================================================
# Реестр Plugin: содержит ровно объявленные подклассы
# ===========================================================================

valid_names = st.text(
    alphabet=st.characters(min_codepoint=97, max_codepoint=122), min_size=1, max_size=12
)


# Уникальный префикс имён, чтобы сгенерированные плагины не сталкивались с реальными
# (Json/Yaml/... из фиксированных тестов). hypothesis гоняет тело сотни раз с одним
# Plugin.registry, поэтому КАЖДЫЙ пример сам убирает свои записи в finally —
# иначе имена накапливались бы и коллизировали между примерами.
def _fresh(name: str, salt: int) -> str:
    return f"prop_{salt}_{name}"


@given(names=st.lists(valid_names, min_size=0, max_size=8, unique=True))
@settings(derandomize=True)
def test_registry_contains_exactly_declared_subclasses(names):
    before = set(Plugin.registry)
    declared = {}
    try:
        for i, nm in enumerate(names):
            key = _fresh(nm, i)
            sub = type(key.capitalize() + "Plugin", (Plugin,), {}, name=key)
            declared[key] = sub

        # Реестр пополнился РОВНО объявленными подклассами — ни больше, ни меньше.
        assert set(Plugin.registry) == before | set(declared)
        for key, cls in declared.items():
            assert Plugin.registry[key] is cls
            assert cls.name == key
    finally:
        for key in declared:
            Plugin.registry.pop(key, None)


@given(name=valid_names)
@settings(derandomize=True)
def test_registry_duplicate_name_raises(name):
    key = _fresh(name, 0)
    try:
        type(key.capitalize() + "A", (Plugin,), {}, name=key)
        with pytest.raises(ValueError):
            type(key.capitalize() + "B", (Plugin,), {}, name=key)
    finally:
        Plugin.registry.pop(key, None)


# ===========================================================================
# DefaultDict: дефолт для отсутствующих, реальное значение после set
# ===========================================================================

attr_names = st.text(
    alphabet=st.characters(min_codepoint=97, max_codepoint=122), min_size=1, max_size=10
)


@given(default=st.integers(), name=attr_names)
@settings(derandomize=True)
def test_defaultdict_missing_returns_default(default, name):
    d = DefaultDict(default=default)
    assert getattr(d, name) == default


@given(default=st.integers(), name=attr_names, value=st.integers())
@settings(derandomize=True)
def test_defaultdict_set_overrides_default(default, name, value):
    assume(value != default)
    d = DefaultDict(default=default)
    setattr(d, name, value)
    assert getattr(d, name) == value
