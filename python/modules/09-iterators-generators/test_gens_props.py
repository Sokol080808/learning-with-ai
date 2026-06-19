# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_gens.py проверяет ФИКСИРОВАННЫЕ примеры (countdown(3) -> [3, 2, 1] и т.п.).
# Здесь — РАНДОМИЗИРОВАННЫЕ property-тесты: hypothesis сам подбирает сотни входов
# (пустые списки, одиночные элементы, отрицательные n, длинные списки) и проверяет
# не конкретный ответ, а ИНВАРИАНТЫ — законы, которые обязаны держаться для ЛЮБОГО
# входа. Ленивый генератор должен совпадать со своим «жадным» эквивалентом; take(n)
# по длине == min(n, len); chunks обязан склеиваться обратно в исходный список;
# running_total согласован с накопительной суммой. Если реализация верна на
# примерах, но врёт на краю — эти тесты покажут минимальный контрпример.

import inspect
import itertools

from hypothesis import given, settings
from hypothesis import strategies as st

from gens import countdown, take, running_total, chunks

ints = st.integers(min_value=-10**9, max_value=10**9)
int_lists = st.lists(ints, max_size=50)


# --- countdown: совпадает с жадным range, ленив и пуст на n <= 0 ------------

@given(n=st.integers(min_value=-50, max_value=200))
def test_countdown_matches_eager_range(n):
    # Жадный эквивалент: range(n, 0, -1). Ленивый генератор обязан выдать то же.
    assert list(countdown(n)) == list(range(n, 0, -1))


@given(n=st.integers(min_value=-50, max_value=0))
def test_countdown_nonpositive_is_empty(n):
    assert list(countdown(n)) == []


@given(n=st.integers(min_value=1, max_value=200))
def test_countdown_length_and_bounds(n):
    out = list(countdown(n))
    assert len(out) == n
    assert out[0] == n and out[-1] == 1


@given(n=st.integers(min_value=-10, max_value=50))
def test_countdown_is_generator(n):
    assert inspect.isgenerator(countdown(n))


# --- take: длина == min(k, len), это префикс, лень на бесконечном источнике -

@given(xs=int_lists, k=st.integers(min_value=-5, max_value=60))
def test_take_length_is_min_k_len(xs, k):
    out = take(xs, k)
    assert isinstance(out, list)
    assert len(out) == max(0, min(k, len(xs)))


@given(xs=int_lists, k=st.integers(min_value=-5, max_value=60))
def test_take_is_a_prefix(xs, k):
    # Взятое — ровно начало списка, тех же элементов и в том же порядке.
    out = take(xs, k)
    assert out == xs[:max(0, k)]


@given(k=st.integers(min_value=0, max_value=100))
def test_take_from_infinite_count_is_range(k):
    # На бесконечном источнике take ОБЯЗАН остановиться (иначе тест зависнет).
    assert take(itertools.count(0), k) == list(range(k))


@given(xs=int_lists, k=st.integers(min_value=0, max_value=20))
def test_take_does_not_overconsume(xs, k):
    # После take(it, k) в итераторе остаётся ровно хвост xs[k:].
    it = iter(xs)
    head = take(it, k)
    tail = list(it)
    assert head == xs[:k]
    assert tail == xs[k:]
    assert head + tail == xs


def test_take_extreme_edges():
    assert take([], 5) == []
    assert take([7], 0) == []
    assert take([7], 1) == [7]
    assert take(itertools.count(0), 0) == []


# --- running_total: согласован с itertools.accumulate, ленив ----------------

@given(xs=int_lists)
@settings(max_examples=200)
def test_running_total_matches_accumulate(xs):
    # Жадный эквивалент — itertools.accumulate (нарастающая сумма).
    assert list(running_total(xs)) == list(itertools.accumulate(xs))


@given(xs=st.lists(ints, min_size=1, max_size=50))
def test_running_total_last_equals_sum_and_length(xs):
    out = list(running_total(xs))
    assert len(out) == len(xs)
    assert out[-1] == sum(xs)


@given(xs=st.lists(ints, min_size=1, max_size=50))
def test_running_total_step_differences_recover_xs(xs):
    # Разности соседних накопленных сумм восстанавливают исходные элементы.
    out = list(running_total(xs))
    recovered = [out[0]] + [out[i] - out[i - 1] for i in range(1, len(out))]
    assert recovered == xs


def test_running_total_empty_and_generator():
    assert list(running_total([])) == []
    assert inspect.isgenerator(running_total([1, 2, 3]))


# --- chunks: куски склеиваются обратно, размеры ровные кроме последнего -----

@given(xs=st.lists(ints, max_size=50), size=st.integers(min_value=1, max_value=10))
def test_chunks_concatenate_back_to_original(xs, size):
    # Главный закон: склейка всех кусков == исходный список (ничего не теряем
    # и не добавляем, порядок сохранён).
    pieces = list(chunks(xs, size))
    flat = [x for piece in pieces for x in piece]
    assert flat == xs


@given(xs=st.lists(ints, max_size=50), size=st.integers(min_value=1, max_value=10))
def test_chunks_sizes_are_full_except_last(xs, size):
    pieces = list(chunks(xs, size))
    if not xs:
        assert pieces == []
        return
    # Все куски, кроме последнего, ровно по size; последний — от 1 до size.
    for piece in pieces[:-1]:
        assert len(piece) == size
    assert 1 <= len(pieces[-1]) <= size
    # Количество кусков == потолок(len / size).
    expected_count = (len(xs) + size - 1) // size
    assert len(pieces) == expected_count


@given(xs=st.lists(ints, max_size=50), size=st.integers(min_value=1, max_value=10))
def test_chunks_yields_lists_and_is_generator(xs, size):
    assert inspect.isgenerator(chunks(xs, size))
    for piece in chunks(xs, size):
        assert isinstance(piece, list)


def test_chunks_extreme_edges():
    assert list(chunks([], 3)) == []
    assert list(chunks([1], 5)) == [[1]]
    assert list(chunks([1, 2, 3], 1)) == [[1], [2], [3]]
