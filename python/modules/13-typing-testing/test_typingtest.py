# Эти тесты трогать не нужно — это эталон поведения.
# Модуль 13 — Аннотации типов и тестирование.
#
# Заметь, как устроены тесты: arrange-act-assert (подготовь-выполни-проверь),
# pytest.raises для ожидаемых ошибок и @pytest.mark.parametrize для пачки случаев.

import pytest

from typingtest import apply_all, clamp, merge_counts, safe_get


# --- Задание 1: clamp (зажать в диапазон) ---

@pytest.mark.parametrize("x, lo, hi, expected", [
    (3, 0, 5, 3),       # внутри диапазона — без изменений
    (-1, 0, 5, 0),      # ниже нижней границы — прижали к lo
    (10, 0, 5, 5),      # выше верхней границы — прижали к hi
    (0, 0, 5, 0),       # ровно на нижней границе
    (5, 0, 5, 5),       # ровно на верхней границе
    (7, 7, 7, 7),       # вырожденный диапазон из одной точки (lo == hi)
    (-3, -5, -1, -3),   # отрицательный диапазон, внутри
    (-10, -5, -1, -5),  # отрицательный диапазон, ниже
    (2, -5, -1, -1),    # отрицательный диапазон, выше
])
def test_clamp_basic(x, lo, hi, expected):
    # arrange — входные данные пришли из parametrize
    # act
    result = clamp(x, lo, hi)
    # assert
    assert result == expected


def test_clamp_returns_x_itself_when_inside():
    # arrange
    x, lo, hi = 4, 0, 10
    # act
    result = clamp(x, lo, hi)
    # assert — внутри диапазона возвращается ровно то же число
    assert result == 4


def test_clamp_bad_range_raises_value_error():
    # lo > hi — диапазон пуст, зажимать некуда: ждём ValueError
    with pytest.raises(ValueError):
        clamp(3, 5, 0)


def test_clamp_equal_bounds_is_allowed():
    # lo == hi — это НЕ ошибка: диапазон из одной точки, любой x схлопывается в неё
    assert clamp(100, 7, 7) == 7
    assert clamp(-100, 7, 7) == 7


# --- Задание 2: safe_get (значение или None) ---

def test_safe_get_existing_key():
    # arrange
    d = {"a": 1, "b": 2}
    # act / assert
    assert safe_get(d, "a") == 1
    assert safe_get(d, "b") == 2


def test_safe_get_missing_key_returns_none():
    d = {"a": 1}
    assert safe_get(d, "zzz") is None


def test_safe_get_does_not_raise_on_missing():
    # ключа нет, но KeyError быть НЕ должно — только None
    d = {}
    result = safe_get(d, "anything")
    assert result is None


def test_safe_get_value_zero_is_returned_not_none():
    # значение 0 — это валидный ответ, его нельзя путать с "ключа нет"
    d = {"hits": 0}
    assert safe_get(d, "hits") == 0
    assert safe_get(d, "hits") is not None


def test_safe_get_does_not_mutate_dict():
    # просто чтение отсутствующего ключа не должно добавлять его в словарь
    d = {"a": 1}
    safe_get(d, "missing")
    assert d == {"a": 1}


# --- Задание 3: merge_counts (сумма значений по ключам) ---

def test_merge_counts_overlapping_keys():
    a = {"a": 1, "b": 2}
    b = {"b": 3, "c": 4}
    assert merge_counts(a, b) == {"a": 1, "b": 5, "c": 4}


def test_merge_counts_disjoint_keys():
    a = {"x": 1}
    b = {"y": 2}
    assert merge_counts(a, b) == {"x": 1, "y": 2}


def test_merge_counts_empty_second():
    a = {"a": 1, "b": 2}
    assert merge_counts(a, {}) == {"a": 1, "b": 2}


def test_merge_counts_empty_first():
    b = {"a": 1, "b": 2}
    assert merge_counts({}, b) == {"a": 1, "b": 2}


def test_merge_counts_both_empty():
    assert merge_counts({}, {}) == {}


def test_merge_counts_all_keys_overlap():
    a = {"x": 10, "y": 20}
    b = {"x": 1, "y": 2}
    assert merge_counts(a, b) == {"x": 11, "y": 22}


def test_merge_counts_does_not_mutate_inputs():
    # исходные словари должны остаться нетронутыми — возвращается НОВЫЙ словарь
    a = {"a": 1, "b": 2}
    b = {"b": 3, "c": 4}
    merge_counts(a, b)
    assert a == {"a": 1, "b": 2}
    assert b == {"b": 3, "c": 4}


def test_merge_counts_result_is_new_object():
    # результат не должен быть тем же объектом, что любой из входов
    a = {"a": 1}
    b = {"a": 2}
    result = merge_counts(a, b)
    assert result is not a
    assert result is not b
    assert result == {"a": 3}


# --- Задание 4: apply_all (применить список Callable к числу) ---

# ─── Фикстуры ─────────────────────────────────────────────────────────────────

@pytest.fixture
def sample_funcs() -> list:
    """Небольшой набор функций int → int для тестов apply_all.

    Демонстрирует pytest.fixture: подготовка данных вынесена в одно место,
    каждый тест получает свежую копию (фикстура вызывается заново).
    """
    return [
        lambda x: x + 1,    # прибавить 1
        lambda x: x * 2,    # умножить на 2
        abs,                 # абсолютное значение (встроенная функция)
        lambda x: x ** 2,   # возвести в квадрат
    ]


# ─── Корректность ─────────────────────────────────────────────────────────────

def test_apply_all_basic_positive(sample_funcs: list) -> None:
    # arrange: x=3, функции из фикстуры
    # act
    result = apply_all(sample_funcs, 3)
    # assert: [3+1, 3*2, abs(3), 3**2]
    assert result == [4, 6, 3, 9]


def test_apply_all_basic_negative(sample_funcs: list) -> None:
    # x=-3 — проверяем abs и квадрат на отрицательном числе
    result = apply_all(sample_funcs, -3)
    assert result == [-2, -6, 3, 9]


def test_apply_all_zero(sample_funcs: list) -> None:
    result = apply_all(sample_funcs, 0)
    assert result == [1, 0, 0, 0]


def test_apply_all_single_func() -> None:
    # один элемент в списке — результат тоже список из одного элемента
    result = apply_all([lambda x: x * 10], 5)
    assert result == [50]


def test_apply_all_empty_funcs() -> None:
    # пустой список функций → пустой результат
    result = apply_all([], 42)
    assert result == []


def test_apply_all_result_length_matches_funcs(sample_funcs: list) -> None:
    # длина результата всегда равна числу функций
    result = apply_all(sample_funcs, 7)
    assert len(result) == len(sample_funcs)


def test_apply_all_preserves_order() -> None:
    # порядок результатов совпадает с порядком функций
    funcs = [lambda x: x + 100, lambda x: x - 100, lambda x: x * 0]
    result = apply_all(funcs, 5)
    assert result == [105, -95, 0]


def test_apply_all_does_not_mutate_funcs(sample_funcs: list) -> None:
    # вызов apply_all не должен менять список функций
    original_len = len(sample_funcs)
    apply_all(sample_funcs, 10)
    assert len(sample_funcs) == original_len


def test_apply_all_identity_func() -> None:
    # тождественная функция: результат совпадает с x для каждого элемента
    funcs = [lambda x: x, lambda x: x, lambda x: x]
    assert apply_all(funcs, 7) == [7, 7, 7]
