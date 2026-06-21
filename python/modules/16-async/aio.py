# Этот файл заполняешь ТЫ. Реализуй функции так, чтобы тесты модуля 16-async стали зелёными.
#
# Модуль 16 — Асинхронность (asyncio).
# Все функции ниже — корутины (объявлены через `async def`). Внутри используй
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
    raise NotImplementedError("TODO: верни сумму элементов xs — это корутина, используй встроенный sum()")


async def delayed(value: Any, seconds: float) -> Any:
    """Разминка. Подождать seconds (неблокирующе) и вернуть value.

    `await asyncio.sleep(seconds)` отдаёт управление событийному циклу на время
    паузы — другие задачи в это время работают. Это «отдых», а не «зависание».
    Пример: asyncio.run(delayed("ok", 0.01)) == "ok"
    """
    raise NotImplementedError("TODO: подожди seconds с помощью asyncio.sleep, затем верни value")


async def gather_results(coros: Iterable[Awaitable[Any]]) -> list:
    """Разминка. Запустить корутины КОНКУРЕНТНО и собрать их результаты в список.

    Тонкое место gather: даже если корутины завершаются в РАЗНОМ порядке (у кого
    короче sleep — тот раньше), список результатов идёт в ИСХОДНОМ порядке аргументов.
    Пример: asyncio.run(gather_results([delayed(1, 0.02), delayed(2, 0.0)])) == [1, 2]
    """
    raise NotImplementedError("TODO: запусти все coros конкурентно через asyncio.gather и верни список результатов")


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
    raise NotImplementedError(
        "TODO: проверь limit >= 1 (иначе ValueError), создай asyncio.Semaphore(limit), "
        "оберни каждый вызов func в worker-корутину с 'async with sem', "
        "запусти всё через asyncio.gather и верни список результатов в исходном порядке"
    )


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
    raise NotImplementedError(
        "TODO: проверь n_consumers >= 1 (иначе ValueError), создай asyncio.Queue(), "
        "запусти n_consumers consumer-задач через asyncio.create_task, "
        "producer кладёт все items затем n_consumers стоп-сигналов (None), "
        "дождись queue.join() и asyncio.gather(*consumers), верни счётчик обработанных"
    )
