# здесь пишешь ТЫ; реализуй так, чтобы тесты модуля 17-metaprogramming стали зелёными
from __future__ import annotations

from typing import Any


# ===========================================================================
# Разминка A. Ленивый/дефолтный атрибут через __getattr__
# ===========================================================================

class DefaultDict:
    """Словарь-обёртка, где обращение к ОТСУТСТВУЮЩЕМУ атрибуту даёт значение по умолчанию.

    `__getattr__` вызывается Питоном ТОЛЬКО когда обычный поиск атрибута провалился
    (нет в __dict__ инстанса, нет в классе). Поэтому реальные поики (`_store`, `_default`)
    находятся обычным путём и сюда не попадают — рекурсии нет.

    Пример:
        d = DefaultDict(default=0)
        d.x          # 0   — атрибута нет, отдаём default
        d.x = 5
        d.x          # 5   — теперь есть в __dict__, __getattr__ не зовётся
    """

    def __init__(self, default: Any = None) -> None:
        self._default = default

    def __getattr__(self, name: str) -> Any:
        """Вызывается, только если обычный поиск `name` не нашёл ничего.

        Имена с подчёркиванием в начале (служебные) пробрасываем как ошибку — иначе
        легко словить бесконечную рекурсию и спрятать настоящие баги.
        """
        raise NotImplementedError("TODO: реализуй логику __getattr__ — верни default для обычных имён, подними AttributeError для имён с '_'")


# ===========================================================================
# Существенное задание. Валидирующие дескрипторы данных
# ===========================================================================

class Field:
    """Базовый валидирующий data-дескриптор.

    Хранит значение пофайльно — в `instance.__dict__` под приватным ключом, поэтому
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
        raise NotImplementedError("TODO: запомни self.name = name и self.private_name = '_' + name")

    def __get__(self, instance: Any, owner: type | None = None) -> Any:
        """Доступ к атрибуту.

        Если читают через класс (instance is None) — возвращаем сам дескриптор
        (так делает property). Иначе достаём значение из instance.__dict__.
        """
        raise NotImplementedError("TODO: при instance is None верни self; иначе достань значение из instance.__dict__ по private_name, при отсутствии подними AttributeError")

    def __set__(self, instance: Any, value: Any) -> None:
        """Присваивание атрибута: валидируем, потом кладём per-instance."""
        raise NotImplementedError("TODO: вызови self.validate(value), затем сохрани в instance.__dict__[self.private_name]")

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
        raise NotImplementedError("TODO: проверь тип value против self.expected_type; для int явно отсеки bool; подними TypeError при несоответствии")


class Positive(Typed):
    """Числовой дескриптор: только строго положительные int/float (> 0).

    Нарушение типа → TypeError, нарушение знака → ValueError.
    """

    def __init__(self) -> None:
        super().__init__(expected_type=(int, float))  # type: ignore[arg-type]

    def validate(self, value: Any) -> None:
        raise NotImplementedError("TODO: отсеки bool (TypeError), проверь что value — int или float (TypeError), проверь что value > 0 (ValueError)")


class Ranged(Positive):
    """Числовой дескриптор в полуинтервале (0, max] — расширяет Positive верхней границей.

    Значение вне диапазона → ValueError; неверный тип → TypeError (наследуется).
    """

    def __init__(self, maximum: float) -> None:
        super().__init__()
        self.maximum = maximum

    def validate(self, value: Any) -> None:
        raise NotImplementedError("TODO: вызови super().validate(value), затем проверь value <= self.maximum (ValueError при превышении)")


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
        raise NotImplementedError("TODO: определи ключ (name или cls.__name__), проверь дубликат в Plugin.registry (ValueError), запиши cls.name и Plugin.registry[key] = cls; не забудь super().__init_subclass__(**kwargs)")

    @classmethod
    def get(cls, name: str) -> type["Plugin"]:
        """Достать класс-плагин по имени; KeyError, если такого нет."""
        raise NotImplementedError("TODO: верни Plugin.registry[name] — KeyError сам поднимется если нет")


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
    raise NotImplementedError("TODO: собери field_names из cls.__init__.__annotations__ (без 'return'), навесь __repr__ на cls, верни cls")
