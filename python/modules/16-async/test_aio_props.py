# Эти тесты трогать не нужно — это тоже эталон поведения, но другого рода.
#
# test_aio.py проверяет ФИКСИРОВАННЫЕ примеры. Здесь — РАНДОМИЗИРОВАННЫЕ property-тесты:
# hypothesis сам подбирает сотни входов (пустые списки, один элемент, длинные списки,
# разные limit) и проверяет не конкретный ответ, а ИНВАРИАНТЫ — законы, которые обязаны
# держаться для ЛЮБОГО входа.
#
# Ключевые инварианты асинхронных функций:
#   • gather_results / bounded_map совпадают со СВОИМ синхронным эквивалентом (map);
#   • результат всегда идёт в ИСХОДНОМ порядке элементов (а не в порядке завершения);
#   • bounded_map НИКОГДА не превышает потолок конкурентности limit (счётчик активных);
#   • queue_pipeline обрабатывает РОВНО столько элементов, сколько подано.
#
# ВАЖНО: @settings(derandomize=True) — прогон детерминированный (один и тот же набор
# примеров каждый раз), чтобы reference-green не мигал. Нет pytest-asyncio: каждый тест —
# обычный def, корутину гоняем через asyncio.run. Задержки крошечные (0–0.01s), и тесты
# НЕ полагаются на конкретное время — только на корректность результата и потолок.

import asyncio

from hypothesis import given, settings
from hypothesis import strategies as st

from aio import gather_results, bounded_map, queue_pipeline

ints = st.integers(min_value=-1000, max_value=1000)
int_lists = st.lists(ints, max_size=30)
# Маленькие неотрицательные задержки — имитация I/O без долгого ожидания.
delays = st.floats(min_value=0.0, max_value=0.01)


# --- gather_results: порядок == порядок аргументов, при ЛЮБЫХ задержках ------

@settings(derandomize=True, max_examples=60, deadline=None)
@given(pairs=st.lists(st.tuples(ints, delays), max_size=12))
def test_gather_results_order_independent_of_delays(pairs):
    # Каждая корутина возвращает свой индекс, но спит на случайное (разное) время.
    # gather обязан вернуть значения СТРОГО в исходном порядке, как бы кто ни спал.
    async def make(idx, delay):
        await asyncio.sleep(delay)
        return idx

    async def main():
        coros = [make(i, d) for i, (i_val, d) in enumerate(pairs)]
        return await gather_results(coros)

    out = asyncio.run(main())
    assert out == list(range(len(pairs)))


# --- bounded_map: эквивалентен синхронному map по значениям и порядку --------

@settings(derandomize=True, max_examples=60, deadline=None)
@given(xs=int_lists, limit=st.integers(min_value=1, max_value=8))
def test_bounded_map_equals_plain_map(xs, limit):
    async def f(x):
        await asyncio.sleep(0.0)
        return x * x + 1

    out = asyncio.run(bounded_map(f, xs, limit=limit))
    assert out == [x * x + 1 for x in xs]   # значения И порядок как у обычного map


@settings(derandomize=True, max_examples=60, deadline=None)
@given(xs=int_lists, limit=st.integers(min_value=1, max_value=8))
def test_bounded_map_length_preserved(xs, limit):
    async def f(x):
        return x

    out = asyncio.run(bounded_map(f, xs, limit=limit))
    assert len(out) == len(xs)


# --- bounded_map: потолок конкурентности НИКОГДА не превышается --------------

@settings(derandomize=True, max_examples=40, deadline=None)
@given(n=st.integers(min_value=0, max_value=20), limit=st.integers(min_value=1, max_value=6))
def test_bounded_map_never_exceeds_limit(n, limit):
    # Счётчик одновременно активных вызовов под func. Его пик обязан быть <= limit
    # для любого n и limit — это и есть гарантия семафора (без всяких таймингов).
    state = {"active": 0, "peak": 0}

    async def tracked(x):
        state["active"] += 1
        state["peak"] = max(state["peak"], state["active"])
        await asyncio.sleep(0.001)      # окно, в котором вызовы могут перекрыться
        state["active"] -= 1
        return x

    items = list(range(n))
    out = asyncio.run(bounded_map(tracked, items, limit=limit))
    assert out == items
    assert state["peak"] <= limit
    assert state["peak"] <= max(1, n)   # не больше, чем вообще есть элементов


# --- bounded_map: исходный порядок при «обратных» задержках ------------------

@settings(derandomize=True, max_examples=40, deadline=None)
@given(xs=st.lists(ints, min_size=1, max_size=12), limit=st.integers(min_value=1, max_value=6))
def test_bounded_map_order_with_reversed_delays(xs, limit):
    # Ранние элементы спят дольше поздних. Порядок результатов всё равно исходный.
    n = len(xs)

    async def f_with_index(pair):
        i, x = pair
        await asyncio.sleep(0.001 * (n - i))   # элемент 0 спит дольше всех
        return x

    indexed = list(enumerate(xs))
    out = asyncio.run(bounded_map(f_with_index, indexed, limit=limit))
    assert out == xs


# --- queue_pipeline: обработано ровно столько, сколько подано ----------------

@settings(derandomize=True, max_examples=60, deadline=None)
@given(xs=int_lists, n_consumers=st.integers(min_value=1, max_value=6))
def test_queue_pipeline_processes_exactly_len(xs, n_consumers):
    # Сколько элементов подали — столько и обработано. И всегда корректно завершается
    # (если бы стоп-сигналы были неверны, тест бы завис на join/gather).
    processed = asyncio.run(queue_pipeline(xs, n_consumers=n_consumers))
    assert processed == len(xs)
