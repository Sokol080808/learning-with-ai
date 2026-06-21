# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.


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
    result: list[str] = []
    for i in range(1, n + 1):
        if i % 15 == 0:
            result.append("FizzBuzz")
        elif i % 3 == 0:
            result.append("Fizz")
        elif i % 5 == 0:
            result.append("Buzz")
        else:
            result.append(str(i))
    return result


def factorial(n: int) -> int:
    """Вернуть факториал n (n!).

    Контракт: factorial(0) == 1, factorial(1) == 1,
    factorial(n) == 1 * 2 * ... * n для n >= 2. n считается неотрицательным.
    """
    result = 1
    for k in range(2, n + 1):
        result *= k
    return result


def fib(n: int) -> int:
    """Вернуть n-е число Фибоначчи.

    Контракт: fib(0) == 0, fib(1) == 1, далее fib(n) == fib(n-1) + fib(n-2).
    n считается неотрицательным.
    """
    prev, curr = 0, 1
    for _ in range(n):
        prev, curr = curr, prev + curr
    return prev


def count_vowels(s: str) -> int:
    """Сколько в строке английских гласных (a, e, i, o, u), без учёта регистра.

    Контракт: для пустой строки результат 0; регистр не важен ("A" считается).
    Прочие символы (согласные, цифры, пробелы) не считаются.
    """
    vowels = set("aeiou")
    return sum(1 for ch in s.lower() if ch in vowels)


def gcd(a: int, b: int) -> int:
    """Наибольший общий делитель a и b по алгоритму Евклида.

    Контракт: gcd(a, 0) == a, gcd(0, b) == b; для положительных a, b — их НОД.
    """
    while b:
        a, b = b, a % b
    return a


def pairwise_sum(xs: list[int]) -> list[int]:
    """Вернуть список попарных сумм соседних элементов.

    Контракт:
      - pairwise_sum([1, 4, 9, 16]) == [5, 13, 25]  (1+4, 4+9, 9+16)
      - pairwise_sum([42]) == []    (один элемент — нет пар)
      - pairwise_sum([]) == []      (пустой список — нет пар)
    Длина результата равна len(xs) - 1 для непустого xs, иначе 0.
    """
    return [a + b for a, b in zip(xs, xs[1:])]
