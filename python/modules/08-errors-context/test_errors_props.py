# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_errors.py проверяет ФИКСИРОВАННЫЕ примеры (safe_divide(10, 2) -> 5.0 и т.п.).
# Здесь — РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов
# (огромные числа, нули, отрицательные, мусорные строки, юникод) и проверяет не
# конкретный ответ, а ИНВАРИАНТЫ — свойства, которые обязаны выполняться для ЛЮБОГО
# входа. Контекст-менеджер обязан подавлять ровно «свои» исключения и пропускать
# чужие; парсер — соглашаться ровно с тем, что принимает настоящий int(); get_or —
# никогда не падать. Если реализация верна на примерах, но врёт на краю — эти тесты
# покажут минимальный контрпример.

import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from errors import safe_divide, parse_int, get_or, Suppress, tag, MissingKeyError, wrap_lookup

ints = st.integers(min_value=-10**12, max_value=10**12)
finite_floats = st.floats(min_value=-1e9, max_value=1e9, allow_nan=False, allow_infinity=False)
# Делитель держим вдали от нуля: у денормалей (~1e-308) деление переполняет мантиссу
# и арифметика «плывёт» — это свойство float, а не safe_divide. Берём |b| из [1e-3, 1e9].
nonzero_floats = st.floats(min_value=1e-3, max_value=1e9, allow_nan=False, allow_infinity=False).flatmap(
    lambda m: st.sampled_from([m, -m])
)


# --- safe_divide: либо точное частное, либо None строго на нуле -------------

@given(a=finite_floats, b=nonzero_floats)
def test_safe_divide_matches_real_division(a, b):
    # При ненулевом делителе результат совпадает с обычным делением.
    assert safe_divide(a, b) == pytest.approx(a / b, rel=1e-9, abs=1e-9)


@given(a=finite_floats, b=nonzero_floats)
def test_safe_divide_inverse_recovers_numerator(a, b):
    # Частное, умноженное обратно на делитель, восстанавливает числитель.
    q = safe_divide(a, b)
    assert q is not None
    assert q * b == pytest.approx(a, rel=1e-6, abs=1e-6)


@given(a=finite_floats)
def test_safe_divide_zero_divisor_is_always_none(a):
    # Делитель ноль (в любой форме) -> None, без ZeroDivisionError наружу.
    assert safe_divide(a, 0) is None
    assert safe_divide(a, 0.0) is None


@given(a=nonzero_floats, b=nonzero_floats)
def test_safe_divide_never_raises_and_is_float(a, b):
    result = safe_divide(a, b)
    assert isinstance(result, float)


# --- parse_int: согласие с настоящим int() для ЛЮБОЙ строки ----------------

@given(n=ints)
def test_parse_int_roundtrips_integers(n):
    # Строковое представление целого всегда парсится обратно в это целое.
    assert parse_int(str(n)) == n


@given(n=ints, lead=st.text(alphabet=" \t\n", max_size=4), trail=st.text(alphabet=" \t\n", max_size=4))
def test_parse_int_ignores_surrounding_whitespace(n, lead, trail):
    # Пробельные края int() обрезает сам — parse_int обязан вести себя так же.
    assert parse_int(f"{lead}{n}{trail}") == n


@given(s=st.text(max_size=20))
def test_parse_int_agrees_with_builtin_int(s):
    # Главный инвариант: parse_int принимает РОВНО то, что принимает int(s),
    # и возвращает ровно то же число; на всём остальном — None.
    try:
        expected = int(s)
    except ValueError:
        expected = None
    assert parse_int(s) == expected


@given(s=st.text(max_size=20))
def test_parse_int_result_is_int_or_none(s):
    r = parse_int(s)
    assert r is None or isinstance(r, int)


def test_parse_int_extreme_edges():
    assert parse_int("") is None
    assert parse_int("   ") is None
    assert parse_int("3.0") is None
    assert parse_int("+5") == 5
    assert parse_int("-0") == 0


# --- get_or: внутри диапазона == индексация, снаружи == default, без падений -

@given(xs=st.lists(ints, max_size=30), default=st.integers())
def test_get_or_matches_indexing_inside_range(xs, default):
    # Для любого валидного индекса get_or совпадает с обычной индексацией.
    for i in range(-len(xs), len(xs)):
        assert get_or(xs, i, default) == xs[i]


@given(xs=st.lists(ints, max_size=30), i=st.integers(min_value=-10**6, max_value=10**6), default=st.integers())
def test_get_or_never_raises_and_uses_default_out_of_range(xs, default, i):
    # Каков бы ни был индекс, get_or не бросает; вне диапазона отдаёт default.
    result = get_or(xs, i, default)
    if -len(xs) <= i < len(xs):
        assert result == xs[i]
    else:
        assert result is default


@given(default=st.integers())
def test_get_or_empty_list_always_default(default):
    # У пустого списка нет валидных индексов — всегда default.
    for i in (-2, -1, 0, 1, 5):
        assert get_or([], i, default) is default


# --- Suppress: подавляет ровно «свои» типы, пропускает чужие ----------------

# Иерархия для проверки подклассов: LookupError -> {IndexError, KeyError}.
exc_classes = st.sampled_from([ValueError, KeyError, IndexError, TypeError, LookupError, ArithmeticError, ZeroDivisionError])


@given(suppressed=exc_classes, raised=exc_classes)
@settings(max_examples=200, deadline=None)
def test_suppress_swallows_iff_subclass(suppressed, raised):
    # Инвариант == семантика except: исключение проглатывается тогда и только
    # тогда, когда оно подкласс заявленного типа. Иначе пробрасывается наружу.
    if issubclass(raised, suppressed):
        with Suppress(suppressed):
            raise raised("boom")
        # дошли сюда без падения — подавлено, как и ожидалось
    else:
        with pytest.raises(raised):
            with Suppress(suppressed):
                raise raised("boom")


@given(types=st.lists(exc_classes, min_size=1, max_size=4), raised=exc_classes)
@settings(max_examples=200, deadline=None)
def test_suppress_multiple_types_is_union(types, raised):
    # Несколько типов работают как один большой except (..., ...).
    should_suppress = any(issubclass(raised, t) for t in types)
    if should_suppress:
        with Suppress(*types):
            raise raised("boom")
    else:
        with pytest.raises(raised):
            with Suppress(*types):
                raise raised("boom")


@given(payload=st.integers())
def test_suppress_no_exception_runs_body_to_completion(payload):
    # Без исключения тело отрабатывает полностью, побочный эффект сохраняется.
    seen = []
    with Suppress(ValueError):
        seen.append(payload)
    assert seen == [payload]


@given(types=st.lists(exc_classes, min_size=1, max_size=3))
def test_suppress_enter_returns_self(types):
    mgr = Suppress(*types)
    with mgr as entered:
        pass
    assert entered is mgr


# --- tag: teardown всегда выполняется, исключение пробрасывается ----------

tag_names = st.text(alphabet=st.characters(whitelist_categories=("Ll", "Lu"), min_codepoint=65), min_size=1, max_size=16)


@given(name=tag_names)
@settings(derandomize=True)
def test_tag_always_prints_closing_tag_on_exception(name, capsys):
    # Инвариант: закрывающий тег появляется при любом исходе — нормальном и с ошибкой.
    with pytest.raises(ValueError):
        with tag(name):
            raise ValueError("тест")
    out = capsys.readouterr().out
    assert f"</{name}>" in out


@given(name=tag_names)
@settings(derandomize=True)
def test_tag_open_before_close(name, capsys):
    # Инвариант: открывающий тег появляется перед закрывающим.
    with tag(name):
        pass
    out = capsys.readouterr().out
    open_pos = out.index(f"<{name}>")
    close_pos = out.index(f"</{name}>")
    assert open_pos < close_pos


# --- wrap_lookup: поведение на произвольных dict-ах ----------------------

printable_keys = st.text(min_size=1, max_size=20)
printable_values = st.integers() | st.text(max_size=20)


@given(d=st.dictionaries(printable_keys, printable_values, min_size=0, max_size=10), key=printable_keys)
@settings(derandomize=True)
def test_wrap_lookup_returns_value_iff_key_present(d, key):
    # Инвариант: если ключ есть — возвращает значение из словаря;
    # если ключа нет — бросает MissingKeyError с KeyError как __cause__.
    if key in d:
        assert wrap_lookup(d, key) == d[key]
    else:
        with pytest.raises(MissingKeyError) as exc_info:
            wrap_lookup(d, key)
        assert isinstance(exc_info.value.__cause__, KeyError)


@given(d=st.dictionaries(printable_keys, printable_values, min_size=1, max_size=10))
@settings(derandomize=True)
def test_wrap_lookup_never_raises_key_error_directly(d):
    # Голый KeyError никогда не должен просочиться наружу из wrap_lookup.
    for key in list(d.keys()) + ["__absent_key__"]:
        try:
            wrap_lookup(d, key)
        except MissingKeyError:
            pass  # ожидаемо для отсутствующих ключей
        except KeyError:
            pytest.fail("wrap_lookup пробросил KeyError напрямую — должен MissingKeyError")
