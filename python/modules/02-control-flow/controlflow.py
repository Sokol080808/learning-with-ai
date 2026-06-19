# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 02 стали зелёными.
#
# Подсказки и теория — в README.md этого модуля. Готовых решений тут нет: каждая функция
# пока кидает NotImplementedError. Убери raise и напиши тело сам.


def fizzbuzz(n: int) -> list[str]:
    """Вернуть список строк для чисел 1..n.

    Контракт:
      - число делится на 15 -> "FizzBuzz"
      - число делится на 3  -> "Fizz"
      - число делится на 5  -> "Buzz"
      - иначе               -> само число строкой, например "7"
    Длина результата равна n. Для n <= 0 результат — пустой список.
    Пример: fizzbuzz(5) == ["1", "2", "Fizz", "4", "Buzz"].
    """
    raise NotImplementedError("TODO: реализуй fizzbuzz по контракту из docstring")


def factorial(n: int) -> int:
    """Вернуть факториал n (n!).

    Контракт: factorial(0) == 1, factorial(1) == 1,
    factorial(n) == 1 * 2 * ... * n для n >= 2. n считается неотрицательным.
    """
    raise NotImplementedError("TODO: реализуй factorial по контракту из docstring")


def fib(n: int) -> int:
    """Вернуть n-е число Фибоначчи.

    Контракт: fib(0) == 0, fib(1) == 1, далее fib(n) == fib(n-1) + fib(n-2).
    n считается неотрицательным.
    """
    raise NotImplementedError("TODO: реализуй fib по контракту из docstring")


def count_vowels(s: str) -> int:
    """Сколько в строке английских гласных (a, e, i, o, u), без учёта регистра.

    Контракт: для пустой строки результат 0; регистр не важен ("A" считается).
    Прочие символы (согласные, цифры, пробелы) не считаются.
    """
    raise NotImplementedError("TODO: реализуй count_vowels по контракту из docstring")


def gcd(a: int, b: int) -> int:
    """Наибольший общий делитель a и b по алгоритму Евклида.

    Контракт: gcd(a, 0) == a, gcd(0, b) == b; для положительных a, b — их НОД.
    """
    raise NotImplementedError("TODO: реализуй gcd по контракту из docstring")
