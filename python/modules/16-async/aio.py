# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
#
# Модуль 16 — Асинхронность (asyncio).
# Все функции ниже — корутины (объявлены через `async def`). Внутри они используют
# await, asyncio.gather, asyncio.create_task, asyncio.Queue, asyncio.Semaphore.
# Запускать их из синхронного кода (и из тестов) нужно через asyncio.run(coro()).

import asyncio
from typing import Any, Awaitable, Callable, Iterable


async def async_sum(xs: Iterable[int]) -> int:
    """Разминка. Корутина, возвращающая сумму элементов xs.

    Тело тривиально-синхронное (никакого await не требуется) — смысл упражнения
    в том, чтобы почувствовать: `async def` делает функцию КОРУТИНОЙ. Её вызов
    async_sum([1, 2, 3]) ничего не считает — возвращает объект-корутину, который
    оживает только под await / asyncio.run.
    Пример: asyncio.run(async_sum([1, 2, 3])) == 6
    """
    return sum(xs)


async def delayed(value: Any, seconds: float) -> Any:
    """Разминка. Подождать seconds (неблокирующе) и вернуть value.

    `await asyncio.sleep(seconds)` отдаёт управление событийному циклу на время
    паузы — другие задачи в это время работают. Это «отдых», а не «зависание».
    Пример: asyncio.run(delayed("ok", 0.01)) == "ok"
    """
    await asyncio.sleep(seconds)
    return value


async def gather_results(coros: Iterable[Awaitable[Any]]) -> list:
    """Разминка. Запустить корутины КОНКУРЕНТНО и собрать их результаты в список.

    Тонкое место gather: даже если корутины завершаются в РАЗНОМ порядке (у кого
    короче sleep — тот раньше), список результатов идёт в ИСХОДНОМ порядке аргументов.
    Пример: asyncio.run(gather_results([delayed(1, 0.02), delayed(2, 0.0)])) == [1, 2]
    """
    return list(await asyncio.gather(*coros))


# --- Существенная задача -----------------------------------------------------


async def bounded_map(
    func: Callable[[Any], Awaitable[Any]],
    items: Iterable[Any],
    limit: int,
) -> list:
    """Конкурентно применить корутину-функцию func к каждому элементу items,
    но так, чтобы ОДНОВРЕМЕННО выполнялось не более `limit` вызовов.

    Это «map с потолком конкурентности»: классический способ не перегрузить ресурс
    (сервер, диск, лимит соединений). Ограничитель — asyncio.Semaphore(limit):
    каждый вызов func захватывает «слот» перед стартом и отпускает после.

    Контракт:
      • Результаты возвращаются в ИСХОДНОМ порядке items (i-й результат — для i-го
        элемента), независимо от того, кто из вызовов завершился раньше.
      • В каждый момент времени активно (между захватом и освобождением слота) не
        более `limit` вызовов func.
      • limit >= 1. При limit < 1 — ValueError.
      • Пустой items — пустой список (func не вызывается ни разу).

    func — это КОРУТИНА-функция: func(x) возвращает awaitable, который надо await'ить.
    Пример:
        async def double(x): return x * 2
        asyncio.run(bounded_map(double, [1, 2, 3], limit=2)) == [2, 4, 6]
    """
    if limit < 1:
        raise ValueError(f"limit должен быть >= 1, получено {limit!r}")

    sem = asyncio.Semaphore(limit)

    async def worker(x: Any) -> Any:
        async with sem:               # захватываем слот; ждём, если все заняты
            return await func(x)

    # Оборачиваем каждый элемент в задачу через gather — он сохраняет порядок,
    # а семафор удерживает число одновременно работающих воркеров <= limit.
    return list(await asyncio.gather(*(worker(x) for x in items)))


# --- Бонус: producer/consumer через очередь (используется в существенной теме) --


async def queue_pipeline(items: Iterable[Any], n_consumers: int) -> int:
    """Producer/consumer через asyncio.Queue: один producer кладёт все items в
    очередь, `n_consumers` потребителей разбирают их конкурентно. Возвращает,
    сколько элементов суммарно обработано.

    Демонстрирует корректное ЗАВЕРШЕНИЕ: producer после раскладки делает по одному
    «стоп-сигналу» (None) на каждого consumer, и каждый consumer, увидев None,
    выходит из цикла. Без сигналов о конце consumer'ы зависли бы на queue.get().

    Контракт:
      • n_consumers >= 1, иначе ValueError.
      • Возвращает len(list(items)) — каждый элемент обработан ровно один раз.
    """
    if n_consumers < 1:
        raise ValueError(f"n_consumers должен быть >= 1, получено {n_consumers!r}")

    queue: asyncio.Queue = asyncio.Queue()
    processed = 0

    async def consumer() -> None:
        nonlocal processed
        while True:
            item = await queue.get()
            if item is None:          # стоп-сигнал — этот consumer завершается
                queue.task_done()
                break
            processed += 1
            queue.task_done()

    # Стартуем потребителей как задачи — они начнут ждать на queue.get().
    consumers = [asyncio.create_task(consumer()) for _ in range(n_consumers)]

    # Producer: раскладываем элементы, затем по стоп-сигналу на каждого consumer.
    for item in items:
        await queue.put(item)
    for _ in range(n_consumers):
        await queue.put(None)

    await queue.join()                # ждём, пока вся очередь будет обработана
    await asyncio.gather(*consumers)  # дожидаемся завершения всех потребителей
    return processed
