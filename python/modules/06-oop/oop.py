# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.

from __future__ import annotations

from math import gcd  # пригодится для нормализации (сокращения дроби)


class Fraction:
    """Рациональная дробь num/den, хранящаяся в нормализованной форме.

    Инварианты (должны выполняться сразу после создания и всегда):
      - den > 0 (знаменатель строго положителен, знак — в числителе);
      - gcd(|num|, den) == 1 (дробь сокращена);
      - ноль представлен как 0/1.

    Атрибуты:
      num: int — числитель (может быть отрицательным или нулём);
      den: int — знаменатель (всегда положителен).
    """

    def __init__(self, num: int, den: int) -> None:
        """Создать дробь num/den и НОРМАЛИЗОВАТЬ её.

        Контракт:
          - если den == 0 — поднять ValueError;
          - перенести знак в числитель (знаменатель сделать положительным);
          - сократить num и den на их наибольший общий делитель (НОД);
          - сохранить результат в self.num и self.den.
        """
        if den == 0:
            raise ValueError("знаменатель не может быть нулём")
        # знак всегда переносим в числитель
        if den < 0:
            num, den = -num, -den
        # сокращаем на НОД; для num == 0 берём gcd(0, den) == den, что нормализует ноль в 0/1
        g = gcd(abs(num), den)
        self.num = num // g
        self.den = den // g

    def add(self, other: "Fraction") -> "Fraction":
        """Сумма self и other как НОВАЯ нормализованная Fraction.

        a/b + c/d = (a*d + c*b) / (b*d). Не изменяй self и other.
        """
        new_num = self.num * other.den + other.num * self.den
        new_den = self.den * other.den
        return Fraction(new_num, new_den)

    def mul(self, other: "Fraction") -> "Fraction":
        """Произведение self и other как НОВАЯ нормализованная Fraction.

        a/b * c/d = (a*c) / (b*d). Не изменяй self и other.
        """
        return Fraction(self.num * other.num, self.den * other.den)

    def __eq__(self, other: object) -> bool:
        """Равенство по содержимому.

        Для не-Fraction вернуть NotImplemented (не False и не исключение).
        """
        if not isinstance(other, Fraction):
            return NotImplemented
        return self.num == other.num and self.den == other.den

    def __hash__(self) -> int:
        """Хеш, согласованный с __eq__.

        Равные дроби (одинаковые num и den после нормализации) обязаны иметь
        одинаковый хеш. Делегируем кортежу — он умеет хешироваться сам.
        Определение __eq__ без __hash__ делает класс нехешируемым (Python
        ставит __hash__ = None); явно определяем, чтобы Fraction можно было
        класть в set и использовать как ключ dict.
        """
        return hash((self.num, self.den))

    def __add__(self, other: object) -> "Fraction":
        """Сложение через оператор +.

        a/b + c/d = (a*d + c*b) / (b*d). Для чужого типа — NotImplemented,
        чтобы Python мог попробовать __radd__ правого операнда.
        """
        if not isinstance(other, Fraction):
            return NotImplemented
        new_num = self.num * other.den + other.num * self.den
        new_den = self.den * other.den
        return Fraction(new_num, new_den)

    def __mul__(self, other: object) -> "Fraction":
        """Умножение через оператор *.

        a/b * c/d = (a*c) / (b*d). Для чужого типа — NotImplemented.
        """
        if not isinstance(other, Fraction):
            return NotImplemented
        return Fraction(self.num * other.num, self.den * other.den)

    def __repr__(self) -> str:
        """Технический вид объекта для разработчика (REPL, pytest, отладчик).

        Формат 'Fraction(num, den)' — по нему сразу ясен тип и данные.
        """
        return f"Fraction({self.num}, {self.den})"

    def __str__(self) -> str:
        """Человекочитаемое представление: просто 'num/den'.

        __str__ используется print() и f-строками. Если __str__ не определён,
        Python как запасной вариант использует __repr__.
        """
        return f"{self.num}/{self.den}"

    @classmethod
    def from_int(cls, n: int) -> "Fraction":
        """Альтернативный конструктор: целое число n → дробь n/1.

        Пример: Fraction.from_int(3) == Fraction(3, 1).
        Используем cls вместо Fraction, чтобы подклассы могли его
        корректно наследовать.
        """
        return cls(n, 1)

    @property
    def value(self) -> float:
        """Значение дроби как float (num / den)."""
        return self.num / self.den
