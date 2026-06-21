# Тесты модуля 17 «Метапрограммирование» — фиксированные и краевые случаи.
#
# Запуск: ./python/run.sh 17
# Сейчас — КРАСНЫЕ: стаб кидает NotImplementedError. Реализуй классы/функции в meta.py —
# тесты позеленеют.
import pytest

from meta import (
    DefaultDict,
    Field,
    Plugin,
    Positive,
    Ranged,
    Typed,
    auto_repr,
)


# ===========================================================================
# Разминка A. DefaultDict.__getattr__
# ===========================================================================

def test_defaultdict_missing_attr_returns_default():
    d = DefaultDict(default=0)
    assert d.anything == 0


def test_defaultdict_default_none():
    d = DefaultDict()
    assert d.whatever is None


def test_defaultdict_set_then_get_uses_real_value():
    d = DefaultDict(default=0)
    d.x = 5
    assert d.x == 5  # реальное поле затеняет default, __getattr__ не зовётся


def test_defaultdict_different_missing_names_same_default():
    d = DefaultDict(default="?")
    assert d.a == "?"
    assert d.b == "?"


def test_defaultdict_private_name_raises_attributeerror():
    d = DefaultDict(default=0)
    with pytest.raises(AttributeError):
        _ = d._not_there


def test_defaultdict_no_getattr_recursion_on_init():
    # Если бы __getattr__ дёргал self._default до его создания — была бы рекурсия.
    d = DefaultDict(default=42)
    assert d.foo == 42


# ===========================================================================
# Существенное: дескрипторы Field / Typed / Positive / Ranged
# ===========================================================================

def test_field_set_name_records_names():
    class C:
        f = Field()

    desc = C.__dict__["f"]
    assert desc.name == "f"
    assert desc.private_name == "_f"


def test_field_stores_and_reads_value():
    class C:
        f = Field()

    c = C()
    c.f = 123
    assert c.f == 123


def test_field_get_via_class_returns_descriptor():
    class C:
        f = Field()

    assert isinstance(C.f, Field)  # доступ через класс отдаёт сам дескриптор


def test_field_read_before_set_raises():
    class C:
        f = Field()

    c = C()
    with pytest.raises(AttributeError):
        _ = c.f


def test_field_value_stored_in_instance_dict():
    class C:
        f = Field()

    c = C()
    c.f = 7
    assert c.__dict__["_f"] == 7  # per-instance, под приватным ключом


# --- Typed ---

def test_typed_accepts_correct_type():
    class C:
        name = Typed(str)

    c = C()
    c.name = "hello"
    assert c.name == "hello"


def test_typed_rejects_wrong_type():
    class C:
        name = Typed(str)

    c = C()
    with pytest.raises(TypeError):
        c.name = 123


def test_typed_int_rejects_bool():
    class C:
        n = Typed(int)

    c = C()
    with pytest.raises(TypeError):
        c.n = True  # bool — подкласс int, но для Typed(int) это ошибка типа


def test_typed_int_accepts_int():
    class C:
        n = Typed(int)

    c = C()
    c.n = 10
    assert c.n == 10


# --- Positive ---

def test_positive_accepts_positive_number():
    class Account:
        balance = Positive()

    a = Account()
    a.balance = 100
    assert a.balance == 100


def test_positive_accepts_float():
    class Account:
        balance = Positive()

    a = Account()
    a.balance = 0.5
    assert a.balance == 0.5


def test_positive_rejects_zero():
    class Account:
        balance = Positive()

    a = Account()
    with pytest.raises(ValueError):
        a.balance = 0


def test_positive_rejects_negative():
    class Account:
        balance = Positive()

    a = Account()
    with pytest.raises(ValueError):
        a.balance = -1


def test_positive_rejects_non_number():
    class Account:
        balance = Positive()

    a = Account()
    with pytest.raises(TypeError):
        a.balance = "100"


def test_positive_rejects_bool():
    class Account:
        balance = Positive()

    a = Account()
    with pytest.raises(TypeError):
        a.balance = True


def test_positive_independent_per_instance():
    class Account:
        balance = Positive()

    a, b = Account(), Account()
    a.balance = 10
    b.balance = 20
    assert a.balance == 10
    assert b.balance == 20  # значения не делятся между инстансами


def test_positive_two_fields_on_same_class_independent():
    class Account:
        balance = Positive()
        limit = Positive()

    a = Account()
    a.balance = 10
    a.limit = 99
    assert a.balance == 10
    assert a.limit == 99


# --- Ranged ---

def test_ranged_accepts_in_range():
    class Slider:
        value = Ranged(maximum=100)

    s = Slider()
    s.value = 50
    assert s.value == 50


def test_ranged_accepts_boundary_max():
    class Slider:
        value = Ranged(maximum=100)

    s = Slider()
    s.value = 100
    assert s.value == 100


def test_ranged_rejects_above_max():
    class Slider:
        value = Ranged(maximum=100)

    s = Slider()
    with pytest.raises(ValueError):
        s.value = 101


def test_ranged_rejects_zero_and_negative():
    class Slider:
        value = Ranged(maximum=100)

    s = Slider()
    with pytest.raises(ValueError):
        s.value = 0
    with pytest.raises(ValueError):
        s.value = -5


def test_ranged_rejects_wrong_type():
    class Slider:
        value = Ranged(maximum=100)

    s = Slider()
    with pytest.raises(TypeError):
        s.value = "50"


# ===========================================================================
# Вторая задача: реестр плагинов через __init_subclass__
# ===========================================================================

def test_plugin_registers_subclass_by_explicit_name():
    class Local(Plugin):
        registry = {}  # отдельный реестр, чтобы не мешать другим тестам

    class Json(Local, name="json"):
        pass

    # Реестр на базовом Plugin тоже наполняется (общий) — проверим оба контракта:
    assert Plugin.registry["json"] is Json
    assert Plugin.get("json") is Json
    assert Json.name == "json"


def test_plugin_registers_subclass_default_name_from_classname():
    class Yaml(Plugin):
        pass

    assert Plugin.registry["Yaml"] is Yaml
    assert Yaml.name == "Yaml"


def test_plugin_duplicate_name_raises():
    class Csv(Plugin, name="dup-format"):
        pass

    with pytest.raises(ValueError):
        class Csv2(Plugin, name="dup-format"):  # имя уже занято
            pass


def test_plugin_get_unknown_raises_keyerror():
    with pytest.raises(KeyError):
        Plugin.get("definitely-not-registered-xyz")


# ===========================================================================
# Разминка B: декоратор класса auto_repr
# ===========================================================================

def test_auto_repr_basic():
    @auto_repr
    class P:
        def __init__(self, x: int, y: int) -> None:
            self.x = x
            self.y = y

    assert repr(P(1, 2)) == "P(x=1, y=2)"


def test_auto_repr_single_field():
    @auto_repr
    class Box:
        def __init__(self, w: float) -> None:
            self.w = w

    assert repr(Box(3.5)) == "Box(w=3.5)"


def test_auto_repr_returns_same_class():
    @auto_repr
    class C:
        def __init__(self, a: int) -> None:
            self.a = a

    assert C.__name__ == "C"
    assert isinstance(C(1), C)


def test_auto_repr_uses_repr_of_values():
    @auto_repr
    class S:
        def __init__(self, name: str) -> None:
            self.name = name

    assert repr(S("hi")) == "S(name='hi')"  # строки в repr — в кавычках
