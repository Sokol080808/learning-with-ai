# ЭТАЛОННОЕ РЕШЕНИЕ (ветка reference). На ветке main здесь лежит стаб с NotImplementedError —
# его заполняет ученик. Этот файл существует только чтобы доказать, что задачи решаемы и что
# тесты (включая рандомизированные) зелёные на правильном коде. В main он НЕ попадает.
from __future__ import annotations

from typing import Any


# ===========================================================================
# Разминка A. Ленивый/дефолтный атрибут через __getattr__
# ===========================================================================

class DefaultDict:
    """Словарь-обёртка, где обращение к ОТСУТСТВУЮЩЕМУ атрибуту даёт значение по умолчанию.

    `__getattr__` вызывается Питоном ТОЛЬКО когда обычный поиск атрибута провалился
    (нет в __dict__ инстанса, нет в классе). Поэтому реальные поички (`_store`, `_default`)
    находятся обычным путём и сюда не попадают — рекурсии нет.

    Пример:
        d = DefaultDict(default=0)
        d.x          # 0   — атрибута нет, отдаём default
        d.x = 5
        d.x          # 5   — теперь есть в __dict__, __getattr__ не зовётся
    """

    def __init__(self, default: Any = None) -> None:
        # Прямое присваивание в __dict__, чтобы не зависеть от возможного __setattr__.
        self._default = default

    def __getattr__(self, name: str) -> Any:
        """Вызывается, только если обычный поиск `name` не нашёл ничего.

        Имена с подчёркиванием в начале (служебные) пробрасываем как ошибку — иначе
        легко словить бесконечную рекурсию и спрятать настоящие баги.
        """
        if name.startswith("_"):
            raise AttributeError(name)
        return self._default


# ===========================================================================
# Существенное задание. Валидирующие дескрипторы данных
# ===========================================================================
#
# Дескриптор — это объект, который, будучи атрибутом КЛАССА, перехватывает доступ к
# одноимённому атрибуту ИНСТАНСА через __get__/__set__. Если есть и __get__, и __set__
# (или __delete__), это «data descriptor» — он имеет приоритет над __dict__ инстанса.
# property — это как раз data descriptor.
#
# __set_name__(owner, name) Питон зовёт автоматически в момент создания класса-владельца
# и сообщает дескриптору, под каким именем его положили. Так дескриптор узнаёт своё имя
# без ручного дублирования: Positive() вместо Positive("price").


class Field:
    """Базовый валидирующий data-дескриптор.

    Хранит значение ПОФАЙЛЬНО — в `instance.__dict__` под приватным ключом, поэтому
    разные инстансы не делят состояние (классическая ловушка «состояние в самом
    дескрипторе»). Подклассы переопределяют `validate`, чтобы наложить ограничения.

    Поток:
        class Account:
            balance = Positive()      # __set_name__ → self.name = "balance"
        a = Account(); a.balance = 10 # __set__ → validate(10) → instance.__dict__["_balance"] = 10
        a.balance                     # __get__ → instance.__dict__["_balance"]
    """

    def __set_name__(self, owner: type, name: str) -> None:
        """Питон зовёт это при создании класса; запоминаем публичное и приватное имя."""
        self.name = name
        self.private_name = f"_{name}"

    def __get__(self, instance: Any, owner: type | None = None) -> Any:
        """Доступ к атрибуту.

        Если читают через класс (instance is None) — возвращаем сам дескриптор
        (так делает property). Иначе достаём значение из instance.__dict__.
        """
        if instance is None:
            return self
        if self.private_name not in instance.__dict__:
            raise AttributeError(self.name)
        return instance.__dict__[self.private_name]

    def __set__(self, instance: Any, value: Any) -> None:
        """Присваивание атрибута: валидируем, потом кладём per-instance."""
        self.validate(value)
        instance.__dict__[self.private_name] = value

    def validate(self, value: Any) -> None:
        """Проверка значения. Базовый дескриптор не ограничивает ничего."""
        return None


class Typed(Field):
    """Дескриптор, требующий значение конкретного типа.

    Нарушение типа → TypeError. `bool` — подкласс `int`, поэтому при expected=int
    булевы значения мы НЕ пропускаем (частая каноническая ловушка).
    """

    def __init__(self, expected_type: type) -> None:
        self.expected_type = expected_type

    def validate(self, value: Any) -> None:
        # bool — подкласс int; явно его отсекаем, когда ждём именно int.
        if self.expected_type is int and isinstance(value, bool):
            raise TypeError(f"{self.name}: expected int, got bool")
        if not isinstance(value, self.expected_type):
            raise TypeError(
                f"{self.name}: expected {self.expected_type.__name__}, "
                f"got {type(value).__name__}"
            )


class Positive(Typed):
    """Числовой дескриптор: только строго положительные int/float (> 0).

    Нарушение типа → TypeError, нарушение знака → ValueError.
    """

    def __init__(self) -> None:
        super().__init__(expected_type=(int, float))  # type: ignore[arg-type]

    def validate(self, value: Any) -> None:
        # bool пролез бы как int → отсекаем явно.
        if isinstance(value, bool):
            raise TypeError(f"{self.name}: expected a number, got bool")
        if not isinstance(value, (int, float)):
            raise TypeError(
                f"{self.name}: expected int or float, got {type(value).__name__}"
            )
        if value <= 0:
            raise ValueError(f"{self.name}: must be > 0, got {value!r}")


class Ranged(Positive):
    """Числовой дескриптор в полуинтервале (0, max] — расширяет Positive верхней границей.

    Значение вне диапазона → ValueError; неверный тип → TypeError (наследуется).
    """

    def __init__(self, maximum: float) -> None:
        super().__init__()
        self.maximum = maximum

    def validate(self, value: Any) -> None:
        super().validate(value)  # тип + положительность
        if value > self.maximum:
            raise ValueError(
                f"{self.name}: must be <= {self.maximum}, got {value!r}"
            )


# ===========================================================================
# Вторая задача. Авто-реестр подклассов через __init_subclass__
# ===========================================================================

class Plugin:
    """База с авто-реестром потомков.

    `__init_subclass__` Питон зовёт автоматически КАЖДЫЙ раз, когда определяют подкласс.
    Это позволяет регистрировать плагины без метаклассов и без ручного списка. Имя в
    реестре берём из атрибута `name` подкласса (или из __name__, если не задан).

    Пример:
        class Json(Plugin, name="json"): ...
        Plugin.registry["json"]   # <class Json>
    """

    registry: dict[str, type["Plugin"]] = {}

    def __init_subclass__(cls, /, name: str | None = None, **kwargs: Any) -> None:
        """Хук создания подкласса: валидируем имя и регистрируем класс.

        Параметр `name` приходит из заголовка класса: `class X(Plugin, name="x")`.
        Дубликат имени → ValueError (защита от случайного перетирания плагина).
        """
        super().__init_subclass__(**kwargs)
        key = name if name is not None else cls.__name__
        if key in Plugin.registry:
            raise ValueError(f"plugin name already registered: {key!r}")
        cls.name = key
        Plugin.registry[key] = cls

    @classmethod
    def get(cls, name: str) -> type["Plugin"]:
        """Достать класс-плагин по имени; KeyError, если такого нет."""
        return cls.registry[name]


# ===========================================================================
# Декоратор класса. Авто-repr по полям из __init__-аннотаций
# ===========================================================================

def auto_repr(cls: type) -> type:
    """Декоратор КЛАССА: добавляет __repr__ по полям из аннотаций __init__.

    Декоратор класса — функция `type -> type`. Здесь мы навешиваем __repr__, который
    печатает значения атрибутов, перечисленных в `cls.__init__.__annotations__`
    (без 'return'). Это «дешёвая» альтернатива метаклассу для разовой доработки класса.

    Пример:
        @auto_repr
        class P:
            def __init__(self, x: int, y: int) -> None:
                self.x = x; self.y = y
        repr(P(1, 2))   # 'P(x=1, y=2)'
    """
    field_names = [
        n for n in getattr(cls.__init__, "__annotations__", {}) if n != "return"
    ]

    def __repr__(self: Any) -> str:
        parts = ", ".join(f"{n}={getattr(self, n)!r}" for n in field_names)
        return f"{cls.__name__}({parts})"

    cls.__repr__ = __repr__  # type: ignore[assignment]
    return cls
