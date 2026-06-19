# ВНИМАНИЕ: здесь пишешь ТЫ. Реализуй функции так, чтобы тесты модуля 06 стали зелёными.
#
# Модуль 06 — ООП: классы, dunder-методы, свойства.
# Тебе нужно реализовать ОДИН класс — Fraction (рациональная дробь).
# Готовых решений тут нет: только сигнатуры и контракт в docstring'ах.
# Подсказки (ступенчатые) — в README.md этого модуля.

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
        raise NotImplementedError("TODO: нормализуй дробь в __init__ (см. идею 4 и подсказки 1–2)")

    def add(self, other: "Fraction") -> "Fraction":
        """Сумма self и other как НОВАЯ нормализованная Fraction.

        a/b + c/d = (a*d + c*b) / (b*d). Не изменяй self и other.
        """
        raise NotImplementedError("TODO: сложи две дроби и верни новый Fraction (подсказка 3)")

    def mul(self, other: "Fraction") -> "Fraction":
        """Произведение self и other как НОВАЯ нормализованная Fraction.

        a/b * c/d = (a*c) / (b*d). Не изменяй self и other.
        """
        raise NotImplementedError("TODO: перемножь две дроби и верни новый Fraction (подсказка 3)")

    def __eq__(self, other: object) -> bool:
        """Равенство по содержимому.

        Для не-Fraction вернуть NotImplemented (не False и не исключение).
        """
        raise NotImplementedError("TODO: сравни по содержимому, с чужим типом верни NotImplemented (подсказка 4)")

    def __repr__(self) -> str:
        """Строковое представление вида 'a/b' (например, '-1/2', '3/1')."""
        raise NotImplementedError("TODO: верни f-строку вида 'num/den' (подсказка 5)")

    @property
    def value(self) -> float:
        """Значение дроби как float (num / den)."""
        raise NotImplementedError("TODO: верни num/den как float (подсказка 5)")
