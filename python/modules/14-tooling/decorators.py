"""Модуль 14 — существенное задание: декораторы-инструменты.

Три практических декоратора, которые часто встречаются в реальных проектах:
    retry    — повторные попытки при исключениях
    memoize  — кэш результатов по аргументам
    timed    — замер времени выполнения
"""

from __future__ import annotations

import functools
import time
from typing import Callable


# ------------------------------------------------------------------ retry ---

def retry(times: int) -> Callable:
    """Декоратор-фабрика: повторяет вызов функции до ``times`` раз.

    Если функция завершилась успешно — возвращает результат немедленно.
    Если все ``times`` попыток бросили исключение — пробрасывает исключение
    последней попытки.  ``times >= 1``.

    ``functools.wraps`` гарантирует, что ``__name__``, ``__doc__`` и
    ``__wrapped__`` оригинальной функции сохраняются на обёртке.
    """
    def decorator(func: Callable) -> Callable:
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            last_exc: BaseException | None = None
            for _ in range(times):
                try:
                    return func(*args, **kwargs)
                except Exception as exc:
                    last_exc = exc
            raise last_exc  # type: ignore[misc]

        return wrapper

    return decorator


# ----------------------------------------------------------------- memoize --

def memoize(func: Callable) -> Callable:
    """Декоратор: кэширует результаты функции по ключу ``(args, frozenset(kwargs.items()))``.

    Повторный вызов с теми же аргументами возвращает значение из кэша,
    не вызывая функцию заново.  Кэш доступен как ``func.cache`` (обычный
    ``dict``); очистка — ``func.cache.clear()``.

    ``functools.wraps`` сохраняет метаданные оригинала, включая ``__wrapped__``.
    """
    cache: dict = {}

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        key = (args, frozenset(kwargs.items()))
        if key not in cache:
            cache[key] = func(*args, **kwargs)
        return cache[key]

    wrapper.cache = cache  # type: ignore[attr-defined]
    return wrapper


# ------------------------------------------------------------------- timed --

def timed(func: Callable) -> Callable:
    """Декоратор: после каждого вызова сохраняет время выполнения в секундах
    в атрибуте ``func.last_elapsed`` (``float``).

    До первого вызова ``last_elapsed is None``.  Если функция бросила
    исключение, время всё равно фиксируется, а исключение пробрасывается дальше.

    ``functools.wraps`` сохраняет метаданные оригинала.
    """
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start = time.perf_counter()
        try:
            return func(*args, **kwargs)
        finally:
            wrapper.last_elapsed = time.perf_counter() - start

    wrapper.last_elapsed = None  # type: ignore[attr-defined]
    return wrapper
