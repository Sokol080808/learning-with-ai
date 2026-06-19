# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.


def to_fahrenheit(c: float) -> float:
    return c * 9 / 5 + 32


def is_even(n: int) -> bool:
    return n % 2 == 0


def min_of_three(a: int, b: int, c: int) -> int:
    return min(a, b, c)


def average3(a: int, b: int, c: int) -> float:
    return (a + b + c) / 3


def greet(name: str) -> str:
    return f"Привет, {name}!"
