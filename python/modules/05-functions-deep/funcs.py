# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.

from __future__ import annotations

import functools
from typing import Callable


def apply_twice(f: Callable, x):
    """Применить f к x дважды: вернуть f(f(x)).

    Контракт:
      - f — любая функция одного аргумента;
      - сначала вычисляем f(x), затем подаём результат снова в f;
      - возвращаем f(f(x)).

    Пример: apply_twice(lambda n: n + 3, 1) == 7  (1 -> 4 -> 7).
    """
    return f(f(x))


def make_multiplier(n: int) -> Callable[[int], int]:
    """Вернуть функцию, умножающую свой аргумент на n (замыкание).

    Контракт:
      - возвращаем НОВУЮ функцию одного аргумента x, которая считает x * n;
      - n «захватывается» из этого вызова: make_multiplier(3) и make_multiplier(10)
        дают независимые друг от друга функции;
      - саму make_multiplier тут вызывать на результат НЕ нужно — верни функцию.

    Пример: triple = make_multiplier(3); triple(4) == 12.
    """
    def multiplier(x: int) -> int:
        return x * n

    return multiplier


def compose(f: Callable, g: Callable) -> Callable:
    """Композиция двух функций: вернуть функцию x -> f(g(x)).

    Контракт:
      - возвращаем НОВУЮ функцию одного аргумента x;
      - она сначала применяет g к x, затем f к результату: f(g(x));
      - порядок важен: именно f(g(x)), а не g(f(x)).

    Пример: inc = lambda n: n + 1; dbl = lambda n: n * 2;
            compose(inc, dbl)(5) == 11  (5 -> 10 -> 11).
    """
    def composed(x):
        return f(g(x))

    return composed


def sort_by_length(words: list[str]) -> list[str]:
    """Отсортировать слова по длине (по возрастанию).

    Контракт:
      - возвращаем НОВЫЙ список, отсортированный по длине строки (sorted + key=...);
      - исходный список words НЕ изменяем;
      - при равной длине сохраняется исходный порядок (сортировка устойчива).

    Пример: sort_by_length(["bbb", "a", "cc"]) == ["a", "cc", "bbb"].
    """
    return sorted(words, key=len)


def memoize(f: Callable) -> Callable:
    """Декоратор-кэш: запоминать результаты по аргументам.

    Контракт:
      - возвращаем обёртку над f с тем же поведением, что и f;
      - при первом вызове с данными позиционными аргументами — вычисляем f и
        запоминаем результат; при повторном вызове с теми же аргументами —
        возвращаем сохранённое значение, НЕ вызывая f заново;
      - метаданные оригинала (например, __name__) сохрани через functools.wraps;
      - для этого задания аргументы будут только позиционными (*args достаточно).

    Тест проверяет это, считая число РЕАЛЬНЫХ вызовов f.
    """
    cache: dict = {}

    @functools.wraps(f)
    def wrapper(*args):
        if args not in cache:
            cache[args] = f(*args)
        return cache[args]

    return wrapper


# ---------------------------------------------------------------------------
# Новые задания (Идеи 6–8)
# ---------------------------------------------------------------------------

def make_counter(start: int = 0) -> Callable[[], int]:
    """Фабрика счётчиков: каждый вызов возвращённой функции увеличивает счётчик на 1.

    Контракт:
      - возвращаем функцию без аргументов;
      - при каждом вызове она увеличивает внутреннее состояние на 1 и возвращает
        новое значение (т.е. первый вызов → start + 1);
      - два вызова make_counter() создают полностью независимые счётчики —
        один не влияет на другой;
      - изменение внутреннего состояния требует nonlocal.

    Пример:
        c = make_counter()
        c()  # 1
        c()  # 2
        d = make_counter(10)
        d()  # 11  — независим от c
        c()  # 3   — c не изменился
    """
    count = start

    def counter() -> int:
        nonlocal count
        count += 1
        return count

    return counter


def compose_many(*funcs: Callable) -> Callable:
    """Variadic-композиция: применить функции справа налево.

    compose_many(f, g, h)(x)  ==  f(g(h(x)))

    Контракт:
      - принимает произвольное число функций одного аргумента;
      - возвращает новую функцию одного аргумента, которая применяет их
        правоналево (как в математической записи f∘g∘h);
      - compose_many() (без функций) — тождественная функция (x → x);
      - compose_many(f) == f по поведению;
      - связь с двуместным compose: compose_many(f, g) == compose(f, g).

    Пример:
        add1 = lambda x: x + 1
        dbl  = lambda x: x * 2
        neg  = lambda x: -x
        compose_many(add1, dbl, neg)(3)  # neg(3)=-3 → dbl(-3)=-6 → add1(-6)=-5
    """
    def composed(x):
        result = x
        for f in reversed(funcs):
            result = f(result)
        return result

    return composed
