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


def parse_number(s: str) -> "int | float | None":
    """Безопасно распарсить строку в число без исключений.

    Возвращает:
    - int,   если строка представляет целое число (например, "42", "-7", " 10 ")
    - float, если строка представляет дробное число ("3.14", "-0.5", "1e3")
    - None,  если строка — мусор, пустая или не является числом
    """
    try:
        return int(s.strip())
    except (ValueError, AttributeError):
        pass
    try:
        return float(s.strip())
    except (ValueError, AttributeError):
        return None
