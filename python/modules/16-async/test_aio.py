# Эти тесты трогать не нужно — это эталон поведения (ФИКСИРОВАННЫЕ примеры и краевые
# случаи). Рандомизированные property-тесты — в test_aio_props.py.
#
# ВАЖНО: здесь НЕ используется pytest-asyncio. Тестовые функции — обычные `def`,
# а корутину внутри запускают через asyncio.run(coro()). Так и работает мост из
# синхронного теста в асинхронный код (см. Идею 2 в README).
#
# Тесты НЕ полагаются на конкретное время (это флейки). Они проверяют КОРРЕКТНОСТЬ:
# значения, порядок результатов, полноту обработки и реальный потолок конкурентности
# (через счётчик одновременно активных вызовов под семафором), а не «быстрее
# последовательного».

import asyncio
import inspect

import pytest

from aio import (
    async_sum,
    delayed,
    gather_results,
    bounded_map,
    queue_pipeline,
)

# Все задержки крошечные (0.001–0.02s) — тесты быстрые, не флейкие.


# --- async_sum --------------------------------------------------------------

def test_async_sum_is_coroutine_function():
    # async def делает функцию корутиной; её вызов возвращает coroutine-объект.
    assert inspect.iscoroutinefunction(async_sum)
    coro = async_sum([1, 2, 3])
    assert inspect.iscoroutine(coro)
    coro.close()  # закрываем, чтобы не было "never awaited"


def test_async_sum_values():
    assert asyncio.run(async_sum([1, 2, 3])) == 6
    assert asyncio.run(async_sum([])) == 0
    assert asyncio.run(async_sum([-5, 5])) == 0
    assert asyncio.run(async_sum(range(101))) == 5050


# --- delayed ----------------------------------------------------------------

def test_delayed_returns_value():
    assert asyncio.run(delayed("ok", 0.001)) == "ok"
    assert asyncio.run(delayed(42, 0.0)) == 42
    assert asyncio.run(delayed(None, 0.002)) is None


# --- gather_results: порядок результатов == порядок аргументов --------------

def test_gather_results_preserves_argument_order_despite_delays():
    async def main():
        # Первый спит дольше, второй готов сразу, третий — посередине.
        # Порядок результатов обязан идти по позициям, а НЕ по времени завершения.
        return await gather_results([
            delayed("a", 0.02),
            delayed("b", 0.0),
            delayed("c", 0.01),
        ])

    assert asyncio.run(main()) == ["a", "b", "c"]


def test_gather_results_empty():
    assert asyncio.run(gather_results([])) == []


def test_gather_results_returns_list():
    async def main():
        return await gather_results([delayed(i, 0.001) for i in range(5)])

    out = asyncio.run(main())
    assert isinstance(out, list)
    assert out == [0, 1, 2, 3, 4]


# --- bounded_map (существенная задача) --------------------------------------

def test_bounded_map_basic_values_and_order():
    async def double(x):
        await asyncio.sleep(0.001)
        return x * 2

    out = asyncio.run(bounded_map(double, [1, 2, 3, 4, 5], limit=2))
    assert out == [2, 4, 6, 8, 10]


def test_bounded_map_preserves_order_with_uneven_delays():
    # Ранние элементы спят ДОЛЬШЕ, поздние — мгновенно. Если бы порядок шёл по
    # завершению, результат бы перемешался. Он обязан идти по входу.
    async def work(pair):
        idx, delay = pair
        await asyncio.sleep(delay)
        return idx

    items = [(0, 0.02), (1, 0.0), (2, 0.015), (3, 0.0), (4, 0.01)]
    out = asyncio.run(bounded_map(work, items, limit=3))
    assert out == [0, 1, 2, 3, 4]


def test_bounded_map_respects_concurrency_limit():
    # Проверяем РЕАЛЬНЫЙ потолок без таймингов: счётчик «сейчас активно» под func.
    # Максимум одновременно активных вызовов не должен превысить limit.
    state = {"active": 0, "peak": 0}

    async def tracked(x):
        state["active"] += 1
        state["peak"] = max(state["peak"], state["active"])
        # короткая пауза, чтобы вызовы реально перекрывались во времени
        await asyncio.sleep(0.005)
        state["active"] -= 1
        return x

    items = list(range(10))
    out = asyncio.run(bounded_map(tracked, items, limit=3))
    assert out == items
    assert state["peak"] <= 3       # потолок соблюдён
    assert state["peak"] >= 2       # и конкурентность реально была (>1 одновременно)


def test_bounded_map_limit_one_is_sequential():
    # limit=1 => в каждый момент ровно один активный вызов (последовательно).
    state = {"active": 0, "peak": 0}

    async def tracked(x):
        state["active"] += 1
        state["peak"] = max(state["peak"], state["active"])
        await asyncio.sleep(0.001)
        state["active"] -= 1
        return x

    out = asyncio.run(bounded_map(tracked, list(range(6)), limit=1))
    assert out == list(range(6))
    assert state["peak"] == 1


def test_bounded_map_empty_items():
    calls = {"n": 0}

    async def f(x):
        calls["n"] += 1
        return x

    assert asyncio.run(bounded_map(f, [], limit=4)) == []
    assert calls["n"] == 0          # func не вызывалась ни разу


def test_bounded_map_invalid_limit_raises():
    async def f(x):
        return x

    for bad in (0, -1, -10):
        with pytest.raises(ValueError):
            asyncio.run(bounded_map(f, [1, 2, 3], limit=bad))


def test_bounded_map_each_item_processed_once():
    seen = []

    async def record(x):
        await asyncio.sleep(0.001)
        seen.append(x)
        return x

    items = list(range(8))
    out = asyncio.run(bounded_map(record, items, limit=4))
    assert out == items
    assert sorted(seen) == items    # каждый элемент обработан ровно один раз


# --- queue_pipeline (producer/consumer через asyncio.Queue) -----------------

def test_queue_pipeline_processes_all():
    assert asyncio.run(queue_pipeline(list(range(20)), n_consumers=4)) == 20
    assert asyncio.run(queue_pipeline([], n_consumers=2)) == 0
    assert asyncio.run(queue_pipeline([1], n_consumers=1)) == 1


def test_queue_pipeline_more_consumers_than_items_terminates():
    # Больше потребителей, чем элементов: должно корректно завершиться (не зависнуть),
    # каждый consumer получит свой стоп-сигнал.
    assert asyncio.run(queue_pipeline([1, 2, 3], n_consumers=10)) == 3


def test_queue_pipeline_invalid_consumers_raises():
    for bad in (0, -1):
        with pytest.raises(ValueError):
            asyncio.run(queue_pipeline([1, 2, 3], n_consumers=bad))
