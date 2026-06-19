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

    def __repr__(self) -> str:
        """Строковое представление вида 'a/b' (например, '-1/2', '3/1')."""
        return f"{self.num}/{self.den}"

    @property
    def value(self) -> float:
        """Значение дроби как float (num / den)."""
        return self.num / self.den
