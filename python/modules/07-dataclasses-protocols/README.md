# Модуль 07 — Наследование, dataclasses, утиная типизация

До этого ты писал отдельные функции. Теперь — про то, как **строить из объектов модели
предметной области**: фигуры, точки, сущности. Большая идея модуля: *в Python
полиморфизм держится не на иерархии классов, а на наличии нужных методов*. Объекту
неважно, кто его предок, — важно, **что он умеет**. Это и есть «утиная типизация»,
и она меняет то, как ты проектируешь код.

## Идея 1. Наследование и `super()` — и когда оно оправдано

Наследование говорит «класс B **является** частным случаем A» и переиспользует его код.

```python
class Animal:
    def __init__(self, name: str) -> None:
        self.name = name

    def describe(self) -> str:
        return f"{self.name} — животное"

class Dog(Animal):
    def __init__(self, name: str, breed: str) -> None:
        super().__init__(name)   # вызвали __init__ родителя — иначе self.name не появится
        self.breed = breed
```

`super().__init__(...)` — это «выполни конструктор родителя». **Почему так, а не
`Animal.__init__(self, name)`?** Потому что `super()` сам находит правильного предка по
цепочке наследования (MRO) — это переживёт переименование базового класса и корректно
работает при множественном наследовании.

> Подвох: наследование часто переоценивают. Оно жёстко связывает классы. Правило: бери
> наследование, когда честно работает фраза «B **is a** A» (Dog is an Animal). Если же
> «B **has a** A» (Car has an Engine) — это **композиция**, держи объект полем, а не
> наследуйся. Лишняя иерархия — источник хрупкого кода.

## Идея 2. `@dataclass` — автогенерация `__init__`/`__repr__`/`__eq__`

Класс, который просто хранит данные, скучно писать руками: конструктор, читаемый
`repr`, сравнение по полям… `@dataclass` генерирует всё это за тебя.

```python
from dataclasses import dataclass

@dataclass
class Point:
    x: float
    y: float
```

Эти 4 строки эквивалентны десяткам строк ручного кода. Бесплатно ты получаешь:

```python
p = Point(1.0, 2.0)        # __init__ — присвоил x и y
print(p)                   # Point(x=1.0, y=2.0)   — читаемый __repr__
Point(1.0, 2.0) == Point(1.0, 2.0)   # True — __eq__ сравнивает по полям, а не по адресу
```

**Почему `==` важно понять.** Обычный объект сравнивается *по идентичности* (тот же он в
памяти или нет), поэтому два «одинаковых» объекта оказались бы не равны. `@dataclass`
делает `__eq__`, который сравнивает **значения полей** — обычно именно это и нужно для
моделей данных. Аннотации типов (`x: float`) тут обязательны: dataclass смотрит именно на
них, чтобы понять, какие поля у класса есть.

В dataclass можно добавлять обычные методы:

```python
@dataclass
class Point:
    x: float
    y: float

    def distance_to(self, other: "Point") -> float:
        return ((self.x - other.x) ** 2 + (self.y - other.y) ** 2) ** 0.5
```

## Идея 3. Полиморфизм: общий интерфейс, разные реализации

Полиморфизм — это когда **один и тот же вызов** ведёт себя по-разному в зависимости от
типа объекта. Заведём базовый класс-«контракт» и наследников, которые его выполняют:

```python
class Shape:
    def area(self) -> float:
        raise NotImplementedError   # базовый класс не знает площадь — это абстракция

class Circle(Shape):
    def __init__(self, r: float) -> None:
        self.r = r
    def area(self) -> float:
        return 3.141592653589793 * self.r ** 2

class Rectangle(Shape):
    def __init__(self, w: float, h: float) -> None:
        self.w = w
        self.h = h
    def area(self) -> float:
        return self.w * self.h
```

Теперь функция может работать с **любой** фигурой, не зная её конкретный тип:

```python
def total_area(shapes):
    return sum(s.area() for s in shapes)

total_area([Circle(1), Rectangle(2, 3)])   # 3.1415... + 6 — каждому s.area() свой
```

**Почему `raise NotImplementedError` в базовом классе — это нормально, а не недоделка.**
`Shape` не существует «сам по себе»: нельзя посчитать площадь «фигуры вообще». Базовый
класс лишь объявляет: *«у меня будет метод `area`, наследник обязан его дать»*. Если
кто-то по ошибке вызовет `Shape().area()`, исключение честно скажет, что метод не
переопределён. (В реальном коде для этого часто берут `abc.ABC` с `@abstractmethod` —
но `NotImplementedError` ту же идею выражает проще, и её достаточно понять сейчас.)

## Идея 4. Утиная типизация: «если крякает как утка»

Главная мысль модуля. Питону, чтобы вызвать `s.area()`, **не нужно** проверять, что `s`
— наследник `Shape`. Ему достаточно, чтобы у `s` **был метод `area`**. Это и есть
утиная типизация:

> «Если нечто ходит как утка и крякает как утка — это утка».

Не «какого ты типа», а «что ты умеешь». Проверять умения можно двумя инструментами:

```python
hasattr(obj, "area")          # есть ли у объекта атрибут/метод с таким именем
isinstance(obj, (int, float)) # является ли obj экземпляром одного из типов
```

Классический пример — «можно ли по объекту итерироваться». Списки, строки, словари,
множества, генераторы — все итерируемы, но они **не** общий класс-предок. Объединяет их
одно: наличие метода `__iter__`. Проверка «по умению»:

```python
def is_iterable(obj) -> bool:
    return hasattr(obj, "__iter__")
```

Так `[1, 2]`, `"abc"`, `{1: 2}` дадут `True`, а `42` или `None` — `False`. Заметь: число
не итерируемо, хотя и «значение». Утка определяется тем, что крякает, а не тем, что она
вообще существует.

> Подвох: `hasattr(obj, "__iter__")` — простая и надёжная проверка итерируемости. Бывает
> и более общий способ через `iter(obj)` в `try/except TypeError` (он ловит ещё и старый
> протокол через `__getitem__`), но для наших целей наличие `__iter__` — то, что нужно.

## Идея 5. Параметры `@dataclass`: `frozen`, `order`, `eq`

По умолчанию `@dataclass` генерирует только `__init__`, `__repr__` и `__eq__`. Три
параметра-флага меняют поведение:

```python
from dataclasses import dataclass

@dataclass(frozen=True)
class Point3D:
    x: float
    y: float
    z: float
```

**`frozen=True` — иммутабельность и хешируемость.**
После создания объект нельзя изменить: любая попытка присвоить `p.x = 1` бросает
`FrozenInstanceError`. Бонус: `frozen=True` автоматически добавляет `__hash__`, поэтому
такой объект можно класть в `set` и использовать как ключ `dict`. Мутабельный dataclass
(`frozen=False`, по умолчанию) `__hash__` не имеет (или он `None` при наличии `__eq__`),
и это правильно — изменяемый объект в `set` даст непредсказуемое поведение.

```python
p = Point3D(1.0, 2.0, 3.0)
# p.x = 9.9   # → FrozenInstanceError: cannot assign to field 'x'
{p, Point3D(1.0, 2.0, 3.0)}   # → {Point3D(x=1.0, y=2.0, z=3.0)} — работает, элемент один
```

**`order=True` — генерирует операторы сравнения.**
Добавляет `__lt__`, `__le__`, `__gt__`, `__ge__` в том же порядке, что поля объявлены.
Без него `p1 < p2` бросает `TypeError`. С ним объекты можно сортировать через `sorted()`.

```python
@dataclass(frozen=True, order=True)
class Version:
    major: int
    minor: int
    patch: int

versions = [Version(1, 2, 0), Version(0, 9, 5), Version(1, 1, 3)]
sorted(versions)  # → [Version(0,9,5), Version(1,1,3), Version(1,2,0)]
```

**`eq=False`** — отключает генерацию `__eq__`, и тогда сравнение снова идёт по
идентичности. Нужно крайне редко: например, когда ты сам пишешь `__eq__` с другой
логикой.

> Подвох: нельзя одновременно `eq=False, order=True` — сравнения без равенства не
> имеют смысла, Python сразу бросит `ValueError`.

## Идея 6. Мутабельные дефолты — ловушка №1

Самая частая ошибка с dataclass (и со стандартными аргументами функций вообще):

```python
from dataclasses import dataclass

@dataclass
class Bag:
    items: list = []   # ← ОШИБКА: Python бросит ValueError прямо здесь
```

Python запрещает мутабельный объект в качестве дефолта поля — и правильно делает. Если
бы он допустил `items: list = []`, **все** экземпляры `Bag` делили бы *один и тот же*
список. Добавишь элемент в `bag1.items` — он появится и в `bag2.items`. Это та же
«классическая» ловушка изменяемых дефолтных аргументов функций.

**Решение: `field(default_factory=...)`**

```python
from dataclasses import dataclass, field

@dataclass
class Bag:
    items: list = field(default_factory=list)   # каждый экземпляр получает свой []
```

`default_factory` — это *callable* (вызываемый объект), который dataclass зовёт при
каждом создании нового экземпляра. `list` (сам класс) вызывается без аргументов →
возвращает новый пустой список.

```python
b1 = Bag()
b2 = Bag()
b1.items.append(42)
assert b2.items == []   # ← b2 не затронут — у него свой список
```

Для любого мутабельного дефолта (список, словарь, множество, пользовательский класс) —
используй `field(default_factory=...)`. Для иммутабельных (`int`, `str`, `tuple`,
`frozen dataclass`) простой дефолт безопасен.

> Почему именно `list`, а не `lambda: []`? Оба работают. `list` чуть короче и более
> «питонично»; `lambda: []` нужен, когда хочется создать список с начальным содержимым —
> например `lambda: [0, 0, 0]`.

## Идея 7. `__post_init__`: валидация и производные поля

Автогенерированный `__init__` просто присваивает значения полям. Иногда нужно что-то
большее: проверить инвариант, вычислить производное поле, нормализовать данные. Для
этого dataclass вызывает `__post_init__` **сразу после** обычной инициализации полей,
если такой метод определён:

```python
from dataclasses import dataclass
import math

@dataclass
class Circle:
    r: float

    def __post_init__(self) -> None:
        if self.r < 0:
            raise ValueError(f"Радиус не может быть отрицательным: {self.r}")
```

```python
Circle(r=3.0)    # ОК
Circle(r=-1.0)   # → ValueError: Радиус не может быть отрицательным: -1.0
```

**Производные поля** — те, которые вычисляются из других. Их нельзя передавать в
конструктор (иначе нарушится инвариант), поэтому используют `field(init=False)`:

```python
from dataclasses import dataclass, field

@dataclass
class Ring:
    outer: float
    inner: float
    area: float = field(init=False)   # не попадает в __init__

    def __post_init__(self) -> None:
        if self.inner > self.outer:
            raise ValueError("inner должен быть <= outer")
        self.area = math.pi * (self.outer**2 - self.inner**2)
```

```python
r = Ring(outer=5.0, inner=3.0)
r.area   # → 50.265... — вычислено при создании, не надо звать метод
```

> Подвох с `frozen=True`: внутри `__post_init__` нельзя делать `self.area = ...`
> напрямую — поле уже «заморожено» сразу после `__init__`. Обходят через
> `object.__setattr__(self, "area", ...)`. Это редкость, но знать полезно.

## Идея 8. ABC (номинальная типизация) vs Protocol (структурная): когда что

В Python есть два способа выразить «этот класс должен уметь X»:

**ABC (`abc.ABC` + `@abstractmethod`) — номинальная типизация:**
Наследник явно объявляет `class Foo(MyABC)` и обязан реализовать все абстрактные методы.
Python проверяет это в момент создания объекта (`Foo()` бросит `TypeError`, если метод
не переопределён). Хорошо видна иерархия: `isinstance(foo, MyABC)` всегда работает без
специальных флагов.

```python
from abc import ABC, abstractmethod

class Drawable(ABC):
    @abstractmethod
    def area(self) -> float: ...

    @abstractmethod
    def label(self) -> str: ...

class Disk(Drawable):     # явное «is-a»
    def area(self): ...
    def label(self): ...
```

Попытка `Disk()` без реализации `area` → немедленный `TypeError: Can't instantiate
abstract class Disk without concrete implementation for abstract method 'area'`.

**Protocol (`typing.Protocol`) — структурная типизация:**
Наследования не нужно. Любой класс с нужными методами *автоматически* совместим.
`@runtime_checkable` добавляет `isinstance`-проверку в рантайме (но она проверяет только
**наличие** атрибутов, не их сигнатуры).

```python
from typing import Protocol, runtime_checkable

@runtime_checkable
class Drawable(Protocol):
    def area(self) -> float: ...
    def label(self) -> str: ...

class Disk:             # ← нет (Drawable)! но методы есть
    def area(self): ...
    def label(self): ...

isinstance(Disk(), Drawable)   # → True — структурная проверка сработала
```

**Когда что выбирать:**

| Ситуация | ABC | Protocol |
|---|---|---|
| Своя иерархия, все классы под твоим контролем | ✓ | ✓ |
| Классы из чужих библиотек / без общего предка | — | ✓ |
| Нужна явная ошибка при пропущенном методе | ✓ | — |
| Нужна совместимость «по форме» без изменения кода | — | ✓ |
| Статический анализ без `isinstance` | ✓ | ✓ |

> Правило большого пальца: **Protocol** — если работаешь с чужим кодом или хочешь
> гибкости; **ABC** — если строишь собственную иерархию и хочешь защиту от «я забыл
> реализовать метод». В реальных проектах часто ABC внутри библиотеки, Protocol на
> граница между компонентами.

## Идея 9. Родня строителей классов данных: сравнительная таблица

Python даёт несколько инструментов для «класса, который просто хранит данные».
Путаница между ними — частый источник вопросов.

| Инструмент | Иммут. | Хеш | Тип-безопасен | Методы | Дефолты | Когда |
|---|---|---|---|---|---|---|
| `tuple` | да | да | нет | нет | — | быстрые анонимные данные |
| `collections.namedtuple` | да | да | частично | нет | через `_field_defaults` | лёгкая замена tuple с именами |
| `typing.NamedTuple` | да | да | да (аннотации) | можно | да | то же, но с аннотациями и методами |
| `typing.TypedDict` | нет* | нет | да (mypy) | нет | да | словарь с фиксированными ключами (JSON-схема) |
| `@dataclass` | опц. (`frozen`) | опц. | да | да | `field(...)` | полноценные объекты-данные |

\* TypedDict — это обычный `dict` в рантайме, иммутабельности нет.

**Ориентир по выбору:**
- Нужна простая пара/тройка значений без методов → `tuple` или `namedtuple`.
- Работаешь с JSON / TypedDict → `TypedDict`.
- Объект данных с методами, валидацией, наследованием → `@dataclass`.
- Иммутабельный объект-значение (координата, денежная сумма, версия) → `@dataclass(frozen=True)`.
- Нужен хеш (использовать в `set`/`dict`) → `frozen dataclass` или `NamedTuple`.

---

## Задания

Файл `models.py`. Реализуй так, чтобы тесты модуля 07 стали зелёными.

1. `@dataclass` **`Point`** с полями `x: float`, `y: float` и методом
   `distance_to(self, other: Point) -> float` — евклидово расстояние до другой точки.

2. Базовый класс **`Shape`** с методом `area(self) -> float`, который в базовом классе
   делает `raise NotImplementedError` (это нормально — абстракция). Наследники:
   - **`Circle`** — конструктор `Circle(r: float)`, поле `self.r`, `area()` = π·r².
   - **`Rectangle`** — конструктор `Rectangle(w: float, h: float)`, поля `self.w`,
     `self.h`, `area()` = w·h.

   Для π используй `math.pi`.

3. `total_area(shapes: list[Shape]) -> float` — сумма площадей всех фигур (полиморфизм:
   просто зови `area()` у каждой, не разбирая тип). Для пустого списка — `0.0`.

4. `is_iterable(obj: object) -> bool` — «утиная» проверка: можно ли по объекту
   итерироваться. Списки/строки/словари/множества/генераторы → `True`; числа, `None`,
   функции → `False`.

```bash
./python/run.sh 07
```

### Подсказки

<details><summary>Задание 1 — distance_to</summary>
Расстояние между точками — теорема Пифагора: корень из суммы квадратов разностей
координат. Возведение в степень — `**`; корень — это `** 0.5`. Аннотацию типа на
`other` внутри самого класса пиши строкой: `other: "Point"` (класс ещё «не достроен» в
момент чтения тела).
</details>

<details><summary>Задание 2 — Shape и наследники</summary>
В `Shape.area` тело — ровно `raise NotImplementedError`. В `Circle.__init__` сохрани
`self.r = r`, в `area` верни `math.pi * self.r ** 2`. В `Rectangle` — два поля и
произведение. Наследников объявляй как `class Circle(Shape):`. Можешь, но не обязан,
звать `super().__init__()` — у `Shape` своего `__init__` нет.
</details>

<details><summary>Задание 3 — total_area без проверки типов</summary>
В этом весь смысл полиморфизма: тебе всё равно, круг это или прямоугольник. Пройдись по
`shapes` и сложи `s.area()` каждого. Одна строка с `sum(...)` и генераторным выражением
справляется. Что вернуть для пустого списка? `sum` пустого — уже `0`; верни как `float`.
</details>

<details><summary>Задание 4 — is_iterable «по-утиному»</summary>
Не перечисляй типы списком («если list, или str, или…») — это против духа модуля.
Спроси у объекта про **умение**: есть ли у него метод `__iter__`. Один `hasattr(...)`
возвращает ровно `bool`.
</details>

Реализуешь — покажи код, скажу, в верном ли направлении. Особенно интересно про задание
4: попробуй мысленно прогнать его на `range(5)`, на генераторе и на `42` — и объясни мне,
почему ответы именно такие.

---

## Задание (существенное): Protocol + dataclass-фигуры + агрегатор холста

Пока что ты работал с наследованием: `Circle(Shape)`, `Rectangle(Shape)`. В реальном
коде классы часто приходят из разных мест и не могут «просто унаследоваться» от общего
предка — они уже готовы, или живут в чужой библиотеке. Питон решает это через **Protocol**
из модуля `typing`: ты описываешь *что умеет* нужный объект (его интерфейс), и любой класс
с такими методами **автоматически** считается совместимым — без явного `class Foo(MyProtocol)`.

Это и есть **структурная типизация** — статическая родственница утиной типизации.

### Что нужно реализовать

Всё пишется в `models.py`. Скелет уже добавлен — тебе нужно заполнить тела методов.

#### 1. Протокол `Drawable`

```python
from typing import Protocol, runtime_checkable

@runtime_checkable
class Drawable(Protocol):
    def area(self) -> float: ...
    def label(self) -> str: ...
```

`@runtime_checkable` позволяет проверять протокол через `isinstance` во время работы
программы (по умолчанию Protocol — только для статических анализаторов).

Контракт:
- `area() -> float` — площадь объекта; обязана быть `>= 0`.
- `label() -> str` — короткая читаемая метка вида `"ИмяТипа(параметры)"`.

#### 2. Dataclass `Disk`

```python
@dataclass
class Disk:
    r: float          # радиус >= 0

    def area(self) -> float: ...    # math.pi * r²
    def label(self) -> str: ...     # "Disk(r=<r>)"  — например "Disk(r=3.0)"
```

`Disk` **не наследует** `Drawable` — и всё равно удовлетворяет протоколу, потому что у
него есть нужные методы.

#### 3. Dataclass `Rect`

```python
@dataclass
class Rect:
    w: float          # ширина >= 0
    h: float          # высота >= 0

    def area(self) -> float: ...    # w * h
    def label(self) -> str: ...     # "Rect(<w>x<h>)"  — например "Rect(4.0x5.0)"
```

#### 4. Dataclass `Ring`

```python
@dataclass
class Ring:
    outer: float      # внешний радиус
    inner: float      # внутренний радиус (дырка); 0 <= inner <= outer

    def area(self) -> float: ...    # math.pi * (outer² - inner²)
    def label(self) -> str: ...     # "Ring(outer=<outer>, inner=<inner>)"
```

Подвох: когда `inner == 0`, кольцо вырождается в диск. Когда `inner == outer`, площадь
равна нулю.

#### 5. Функция `describe_canvas`

```python
def describe_canvas(shapes: list[Drawable]) -> list[str]:
    ...
```

Принимает список *любых* объектов, удовлетворяющих `Drawable` (не только `Disk`/`Rect`/`Ring` —
любой класс с методами `area()` и `label()` тоже подойдёт).

Возвращает список строк вида:
```
"<label>: area=<площадь>"
```
где `<площадь>` — `round(shape.area(), 4)`. Список **отсортирован по убыванию площади**
(самая большая фигура — первой). Для пустого списка — `[]`.

Пример:
```python
describe_canvas([Rect(w=2.0, h=3.0), Disk(r=1.0)])
# → ['Rect(2.0x3.0): area=6.0', 'Disk(r=1.0): area=3.1416']
```

### Подсказки

<details>
<summary>Что такое Protocol и зачем @runtime_checkable</summary>

`Protocol` — это способ описать интерфейс без базового класса. Если ты напишешь
`class Disk:` (без `(Drawable)`), mypy и pyright всё равно будут рады принять `Disk`
туда, где ожидается `Drawable`, — потому что у него есть методы `area` и `label`.
`@runtime_checkable` добавляет возможность `isinstance(obj, Drawable)` — без него
такая проверка упадёт с ошибкой. Нужно помнить: runtime-проверка смотрит только на
**наличие** атрибутов, не на их сигнатуры.
</details>

<details>
<summary>Как реализовать area() и label() у трёх фигур</summary>

- `Disk.area`: `math.pi * self.r ** 2`
- `Disk.label`: `f"Disk(r={self.r})"`
- `Rect.area`: `self.w * self.h`
- `Rect.label`: `f"Rect({self.w}x{self.h})"`
- `Ring.area`: `math.pi * (self.outer**2 - self.inner**2)` — та же формула Пифагора:
  площадь большого диска минус площадь дырки.
- `Ring.label`: `f"Ring(outer={self.outer}, inner={self.inner})"`
</details>

<details>
<summary>Как построить describe_canvas: сортировка и форматирование</summary>

Тебе нужно: (1) отсортировать по убыванию `area()`, (2) составить строку для каждой фигуры.
Подсказка по сортировке: `sorted(shapes, key=..., reverse=True)`. Какой ключ передать?
Тот, по которому хочешь упорядочить. Подсказка по строке: `f"{shape.label()}: area={round(shape.area(), 4)}"`.
Собери это в list comprehension.
</details>

<details>
<summary>Почему это «структурная», а не «утиная» типизация?</summary>

Утиная типизация — это когда ты вызываешь метод в рантайме и надеешься, что он есть
(никакой явной проверки). Структурная типизация — когда статический анализатор (mypy,
pyright) проверяет совместимость *по форме* интерфейса, не требуя общего предка. Protocol
выражает именно это: «мне не важно, кто ты по иерархии — важно, что ты умеешь». Python
проверит это статически (через аннотации) и, с `@runtime_checkable`, ещё и в рантайме.
</details>

---

## Задание (существенное 2): `Money` — frozen + order + `__post_init__`

Здесь ты соберёшь в одном классе все три новые идеи: иммутабельность через `frozen=True`,
сортируемость через `order=True`, валидацию через `__post_init__`, и защиту от общего
мутабельного дефолта через `field(default_factory=...)`.

### Что нужно реализовать

Всё пишется в `models.py`.

#### Dataclass `Money`

```python
@dataclass(frozen=True, order=True)
class Money:
    amount: Decimal       # сумма; не может быть отрицательной
    currency: str         # код валюты, 3 заглавные буквы (напр. "USD", "EUR", "RUB")
    tags: tuple[str, ...] # метки (теги); иммутабельный кортеж (default: пустой кортеж)
```

Требования:
- `amount` — тип `decimal.Decimal` (точная арифметика без float-погрешностей).
- `currency` — ровно 3 символа, все заглавные латинские буквы; при нарушении — `ValueError`.
- `amount` должна быть `>= 0`; при нарушении — `ValueError`.
- `tags` — кортеж строк, дефолт `()` (пустой кортеж); используй `field(default=...)`.
  Почему кортеж, а не список? Потому что `frozen=True` требует, чтобы поля
  тоже были хешируемыми — список хешировать нельзя, кортеж — можно.
- `frozen=True` → объект неизменяем, хешируем: можно класть в `set` и `dict`.
- `order=True` → объекты сравниваются и сортируются (по `(amount, currency, tags)`).

#### Метод `convert(rate: Decimal, target_currency: str) -> Money`

```python
def convert(self, rate: Decimal, target_currency: str) -> Money:
    ...
```

Возвращает **новый** `Money` с `amount = self.amount * rate` (округли до 2 знаков через
`quantize`) и `currency = target_currency`. `tags` не переносятся (дефолт `()`).
Метод не изменяет оригинал — он `frozen`.

### Подсказки

<details>
<summary>Как объявить frozen+order dataclass и откуда Decimal</summary>

```python
from dataclasses import dataclass, field
from decimal import Decimal

@dataclass(frozen=True, order=True)
class Money:
    amount: Decimal
    currency: str
    tags: tuple[str, ...] = field(default=())
```

`field(default=())` безопасен, потому что кортеж иммутабелен — нет опасности общего
объекта. `field(default_factory=tuple)` тоже сработает, но `default=()` чище.
</details>

<details>
<summary>Как написать __post_init__ для frozen dataclass</summary>

Внутри `__post_init__` можно только **читать** `self.amount` и `self.currency` и бросать
`ValueError`. Присваивать поля нельзя — объект уже заморожен. Валидируй так:

```python
def __post_init__(self) -> None:
    if self.amount < Decimal("0"):
        raise ValueError(f"amount не может быть отрицательным: {self.amount}")
    if len(self.currency) != 3 or not self.currency.isalpha() or not self.currency.isupper():
        raise ValueError(f"currency должен быть 3 заглавными буквами: {self.currency!r}")
```
</details>

<details>
<summary>Как реализовать convert и округлить Decimal</summary>

```python
from decimal import Decimal, ROUND_HALF_UP

def convert(self, rate: Decimal, target_currency: str) -> "Money":
    new_amount = (self.amount * rate).quantize(Decimal("0.01"), rounding=ROUND_HALF_UP)
    return Money(amount=new_amount, currency=target_currency)
```

`quantize(Decimal("0.01"))` оставляет ровно 2 знака после запятой. `ROUND_HALF_UP` —
стандартное «банковское» округление (0.5 → вверх).
</details>

<details>
<summary>Почему frozen + order + хешируемость важны вместе</summary>

`frozen=True` даёт `__hash__`, что позволяет `Money(Decimal("10"), "USD") in {some_set}`.
`order=True` даёт `__lt__` и друзей, что позволяет `sorted([money1, money2, money3])`.
Без `frozen` Python устанавливает `__hash__ = None` при наличии `__eq__` (чтобы не
допустить изменяемый объект в множестве). С `order=True` без `frozen=True` тоже работает,
но тогда объект нельзя хешировать.
</details>
