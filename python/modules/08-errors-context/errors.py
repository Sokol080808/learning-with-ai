# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.

from __future__ import annotations

from types import TracebackType


def safe_divide(a: float, b: float) -> float | None:
    """Поделить a на b.

    Контракт:
      - если b == 0, вернуть None (а не падать с ZeroDivisionError);
      - иначе вернуть a / b (обычное деление, результат — float).

    Идея модуля: вместо того чтобы ронять программу, мы аккуратно
    сообщаем «не получилось» через None.
    """
    try:
        return a / b
    except ZeroDivisionError:
        return None


def parse_int(s: str) -> int | None:
    """Превратить строку в int.

    Контракт:
      - если s — корректное целое в десятичной записи ("42", "-7", " 10 "
        с пробелами по краям тоже годится — int() их сам обрежет), вернуть это число;
      - если строка не является целым числом ("abc", "3.14", "", "12x"),
        вернуть None.

    Делать это нужно через try/except (стиль EAFP), а НЕ проверять строку
    вручную символ за символом.
    """
    try:
        return int(s)
    except (ValueError, TypeError):
        return None


def get_or(xs: list, i: int, default):
    """Вернуть xs[i], а если индекса нет — default.

    Контракт:
      - если i — допустимый индекс списка xs (в т.ч. отрицательный, как в Python:
        xs[-1] — последний), вернуть xs[i];
      - если индекс вне диапазона, вернуть default (никаких исключений наружу).

    Тип результата зависит от содержимого списка и default, поэтому
    аннотацию возврата мы намеренно не фиксируем.
    """
    try:
        return xs[i]
    except IndexError:
        return default


class Suppress:
    """Контекст-менеджер: подавляет указанные типы исключений внутри блока with.

    Учебный аналог contextlib.suppress.

    Пример:
        with Suppress(ValueError):
            int("не число")   # ValueError проглочен, выполнение идёт дальше

    Контракт:
      - Suppress(*exc_types) принимает один или несколько классов исключений;
      - если внутри with возникло исключение, ЯВЛЯЮЩЕЕСЯ одним из exc_types
        (или его подклассом), оно подавляется — наружу не пробрасывается;
      - исключения других типов пробрасываются как обычно;
      - если исключения не было — ничего особенного не происходит.

    Подсказка по механике: __exit__ должен вернуть True, чтобы «проглотить»
    исключение, и False/None — чтобы пропустить его дальше.
    """

    def __init__(self, *exc_types: type[BaseException]) -> None:
        """Запомнить типы исключений, которые надо подавлять."""
        self.exc_types = exc_types

    def __enter__(self) -> "Suppress":
        """Вход в блок with. Обычно просто возвращает сам менеджер (self)."""
        return self

    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_value: BaseException | None,
        traceback: TracebackType | None,
    ) -> bool:
        """Выход из блока with.

        Сюда Python передаёт тип/значение/трейсбек исключения, если оно было
        (иначе все три — None). Верни True, если исключение надо подавить,
        иначе False.
        """
        if exc_type is not None and issubclass(exc_type, self.exc_types):
            return True
        return False
