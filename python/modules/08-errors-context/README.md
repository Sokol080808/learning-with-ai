# Модуль 08 — Ошибки, исключения и контекст-менеджеры

До сих пор мы тихо предполагали, что данные «хорошие»: строка точно число, индекс точно
в диапазоне, делитель точно не ноль. В реальной программе это не так почти никогда.
**Большая идея модуля:** ошибки — это не «случайно сломалось», а ожидаемая часть потока
выполнения, которой надо управлять явно. Python даёт для этого два инструмента —
*исключения* (как реагировать, когда что-то пошло не так) и *контекст-менеджеры*
(как гарантированно прибрать за собой). Если ты знаешь C++ — второе это аналог RAII.

## Идея 1. try / except / else / finally; raise

Исключение — это объект-сигнал «дальше так нельзя». Когда оно «вылетает», обычное
выполнение прерывается и Python ищет, кто его поймает.

```python
try:
    x = int(s)          # рискованная операция
except ValueError:
    x = 0               # сюда попадём, если int() не смог разобрать строку
else:
    print("разобрали", x)  # сюда — ТОЛЬКО если в try не было исключения
finally:
    print("это выполнится в любом случае")  # уборка: и при ошибке, и без
```

ПОЧЕМУ четыре блока, а не один большой `try`:
- `except` ловит **конкретный** тип. Лови узко (`ValueError`), а не всё подряд
  (`except Exception` или, ещё хуже, голый `except:`) — иначе спрячешь баги, о которых
  должен был узнать.
- `else` выполняется, только если `try` прошёл без ошибки. Это позволяет держать в `try`
  лишь рискованную строку, а «продолжение при успехе» вынести отдельно — меньше шанс
  случайно поймать исключение не из той операции.
- `finally` выполняется **всегда** — даже если внутри был `return` или новое исключение.
  Туда кладут освобождение ресурсов (закрыть файл, вернуть соединение).

Сам сигнал поднимают через `raise`:

```python
def withdraw(balance, amount):
    if amount > balance:
        raise ValueError("недостаточно средств")  # явно говорим: так нельзя
    return balance - amount
```

## Идея 2. Иерархия исключений; свои исключения

Все исключения — это классы, выстроенные в дерево наследования. На вершине
`BaseException`, чуть ниже `Exception` (от него наследуется почти всё, что ты ловишь),
а дальше — семейства: `ValueError`, `LookupError` → `IndexError`, `KeyError` и т.д.

```python
try:
    {"a": 1}["b"]
except LookupError:        # KeyError — подкласс LookupError, поэтому ловится
    print("нет такого ключа")
```

ПОЧЕМУ это важно: `except Base` ловит **и все подклассы** `Base`. Поэтому порядок и
выбор класса в `except` — это инструмент: лови базовый, если хочешь накрыть семейство;
лови конкретный, если важен именно он.

Свои исключения заводят, наследуясь от `Exception` — чтобы у ошибок твоей программы было
осмысленное имя, и их можно было ловить отдельно от чужих:

```python
class InsufficientFunds(Exception):
    """Денег на счёте не хватает."""

raise InsufficientFunds("баланс 100, просят 150")
```

Вызывающий код теперь может написать `except InsufficientFunds:` и точно знать, что
поймал именно «свою» бизнес-ошибку, а не, скажем, опечатку в коде, всплывшую как
`TypeError`.

## Идея 3. EAFP vs LBYL

Два стиля защиты от ошибок:

- **LBYL** — *Look Before You Leap*, «семь раз отмерь»: сначала проверяем условия.
- **EAFP** — *Easier to Ask Forgiveness than Permission*, «проще попросить прощения,
  чем разрешения»: просто делаем, а если не вышло — ловим исключение.

```python
# LBYL — проверяем заранее
if key in d:
    value = d[key]
else:
    value = default

# EAFP — пробуем и ловим (питоничный стиль)
try:
    value = d[key]
except KeyError:
    value = default
```

ПОЧЕМУ в Python чаще предпочитают EAFP:
- меньше «гонки»: между `if key in d` и `d[key]` словарь теоретически мог измениться;
  при EAFP операция атомарна — либо взяли, либо поймали.
- одна проверка вместо двойной работы (`in`, потом снова доступ).
- читается как «делаем нормальный путь, а отклонение — отдельно».

Но EAFP не догма: если проверка дешёвая и очевидная (`if b == 0`), LBYL понятнее.
В заданиях ты применишь оба: `safe_divide` — естественно LBYL, `parse_int` — EAFP.

## Идея 4. Контекст-менеджеры: with, `__enter__` / `__exit__`

Представь файл: его надо открыть, поработать и **обязательно** закрыть — даже если
посередине вылетело исключение. Писать это руками через `try/finally` каждый раз
утомительно. `with` делает это за тебя:

```python
with open("data.txt") as f:
    process(f)
# здесь f уже закрыт — гарантированно, даже если process бросил исключение
```

Под капотом `with X as y` — это объект, у которого есть два метода:

```python
class Timer:
    def __enter__(self):
        self.start = time.time()
        return self                 # то, что попадёт в `as`
    def __exit__(self, exc_type, exc_value, traceback):
        print("заняло", time.time() - self.start)
        return False                # False/None — исключение (если было) пробрасываем
```

ПОЧЕМУ это аналог RAII из C++: «захват ресурса — это инициализация, освобождение — это
выход из области видимости». `__enter__` = захват, `__exit__` = гарантированный
освобождающий код, который Python вызовет на выходе из блока при любом исходе.

Ключевая деталь `__exit__`: если внутри `with` возникло исключение, Python передаёт его
тип/значение/трейсбек в аргументы (`exc_type` и т.д.). **Возвращаемое значение решает
судьбу исключения:** вернёшь `True` — оно подавлено (как будто не было); вернёшь
`False`/`None` — оно полетит дальше наружу. Именно на этом и построено задание `Suppress`.

## Идея 5. Связывание исключений: `raise … from err`

Иногда ловишь низкоуровневую ошибку (`KeyError`, `ValueError`) и хочешь пробросить вместо
неё смысловую доменную ошибку — например, `MissingUserError`. Но хочется и сохранить
оригинал для отладки. Для этого Python даёт `raise NewError(…) from original`:

```python
class MissingUserError(Exception):
    """Пользователь не найден в базе."""

def get_user(db: dict, uid: int):
    try:
        return db[uid]
    except KeyError as err:
        raise MissingUserError(f"нет пользователя {uid}") from err
```

При этом у нового исключения заполняется атрибут `__cause__` — это **явная** причина.
Python печатает «The above exception was the direct cause of the following exception».

Когда исключение возникает **внутри блока except** без явного `from`, Python всё равно
цепляет исходное в `__context__` — неявная причина. Разница:

| Атрибут | Откуда | Сообщение Python |
|---|---|---|
| `__cause__` | `raise X from Y` | «direct cause» |
| `__context__` | исключение внутри `except` без `from` | «during handling» |

ПОЧЕМУ оборачивать важно: вызывающий код работает с понятием «нет такого ключа в словаре»
неудобно — ему непонятно, *что* за словарь, *что* за ключ и *какой* из сотни словарей.
Доменная ошибка (`MissingUserError`, `ConfigKeyMissing`) сразу даёт контекст, а `__cause__`
сохраняет трейсбек оригинала — ничего не теряем.

Чтобы полностью «оборвать цепочку» и скрыть внутреннюю ошибку: `raise X from None`.

## Идея 6. Генераторный контекст-менеджер: `@contextlib.contextmanager`

Писать класс с `__enter__`/`__exit__` — подробно. Для большинства случаев достаточно
декоратора `@contextmanager` из модуля `contextlib` и обычного генератора:

```python
from contextlib import contextmanager

@contextmanager
def managed_resource():
    resource = acquire()     # __enter__: подготовка
    try:
        yield resource       # тело блока with выполняется здесь
    finally:
        release(resource)    # __exit__: уборка — выполнится ВСЕГДА
```

Схема работы:
1. Python выполняет генератор до `yield` — это `__enter__`. То, что вы `yield`ите, идёт
   в `as`-переменную.
2. Тело `with` выполняется.
3. Python возобновляет генератор после `yield` — это `__exit__`. Если внутри `with` было
   исключение, оно **брошено в точку yield** (генератор «получает» его).
4. `try: yield … finally:` гарантирует уборку при любом исходе.

ПОЧЕМУ `try/finally` вокруг `yield`, а не просто «код после yield»: если внутри `with`
вылетело исключение, оно прилетает в точку `yield`, и без `try/finally` код после yield
просто не выполнится — ресурс не освободится. `finally` же всегда выполняется.

ПОДВОХ: внутри `@contextmanager` должен быть ровно один `yield`. Два `yield` — ошибка.

## Идея 7. Exception Groups и `except*` (Python 3.11+)

Python 3.11 добавил `ExceptionGroup` — объект, несущий **несколько** исключений сразу.
Это нужно для задач параллелизма (asyncio TaskGroup, concurrent.futures): несколько
корутин могут упасть одновременно, и мы хотим получить все ошибки, а не только первую.

```python
# Создание группы
eg = ExceptionGroup("сетевые ошибки", [
    ConnectionError("хост А"),
    TimeoutError("хост Б"),
])
raise eg

# Поимка — ключевое слово except* (с asterisk)
try:
    raise ExceptionGroup("проблемы", [ValueError(1), TypeError(2)])
except* ValueError as eg:
    print("ValueError:", eg.exceptions)   # ловим только ValueError-ы
except* TypeError as eg:
    print("TypeError:", eg.exceptions)    # ловим только TypeError-ы
```

`except*` (в отличие от `except`) обрабатывает **каждый** подходящий тип отдельно;
исключения другого типа при этом **продолжают распространяться**. Это нельзя смешивать
с обычным `except` в одном `try`.

ПОЧЕМУ это важно именно для async: `asyncio.TaskGroup` автоматически собирает ошибки
упавших задач в `ExceptionGroup`, поэтому обработка через `except*` стала идиомой.

## Идея 8. `contextlib.ExitStack` — динамическое число ресурсов

Когда количество ресурсов известно **заранее** — пишем `with A() as a, B() as b:`.
Но если ресурсов динамически много (список файлов, список соединений), нужен
`ExitStack`:

```python
from contextlib import ExitStack

file_names = ["a.txt", "b.txt", "c.txt"]
with ExitStack() as stack:
    files = [stack.enter_context(open(f)) for f in file_names]
    # все файлы открыты; при любом выходе из блока — все закроются
    for f in files:
        process(f)
```

`ExitStack` регистрирует CM-ы и вызывает их `__exit__` в обратном порядке при выходе
из своего блока `with`. Можно также зарегистрировать произвольную функцию уборки:
`stack.callback(my_cleanup_fn)`.

КОГДА использовать: открытие переменного числа файлов, транзакции в БД, временные
каталоги в цикле — всё, что нельзя выразить статическим `with A, B, C`.

## Идея 9. Анти-паттерны обработки ошибок

**Голый `except:`** (без типа) ловит буквально всё, включая `KeyboardInterrupt`,
`SystemExit`, `GeneratorExit` — те, что вы никогда не хотели перехватить:

```python
# ПЛОХО: скрываем Ctrl+C, sys.exit(), баги в коде
try:
    risky()
except:
    pass

# ХОРОШО: ловим только ожидаемое
try:
    risky()
except ValueError as err:
    log(err)
```

**`except: pass` («проглатывание»)** — наихудший паттерн: ошибка случилась, но вы
о ней ничего не знаете. Программа продолжает работать в сломанном состоянии. Если уж
подавляете — хотя бы логируйте: `except Exception as err: log(err)`.

**Ловля `BaseException`** — почти то же, что голый `except:`. `BaseException` — корень
дерева, от него наследуются `KeyboardInterrupt`, `SystemExit`. Ловите их явно и только
когда точно знаете зачем.

Правило большого пальца: **ловите конкретно**, **не молчите**, **используйте `from`
при оборачивании** — и трейсбек всегда будет говорить правду.

## Задания

Открой `errors.py` и реализуй:

1. `safe_divide(a: float, b: float) -> float | None` — поделить `a` на `b`; вернуть
   `None` при делении на ноль (а не падать). Естественный кандидат на LBYL.
2. `parse_int(s: str) -> int | None` — превратить строку в `int`; вернуть `None`, если
   строка не целое число. Делай через `try/except` (EAFP), а не ручным разбором символов.
3. `get_or(xs: list, i: int, default)` — вернуть `xs[i]`; если индекс вне диапазона,
   вернуть `default`. Отрицательные индексы (`-1` и т.п.) — валидны, как в обычном Python.
4. Класс-контекст-менеджер `Suppress(*exc_types)` — учебный аналог
   `contextlib.suppress`. Подавляет указанные типы исключений внутри блока `with`
   (и их подклассы), а исключения других типов пробрасывает наружу. Реализуй
   `__init__`, `__enter__`, `__exit__`.
5. Генераторный контекст-менеджер `tag(name: str)` через `@contextlib.contextmanager`.
   Перед `yield` печатает `<name>`, после (в `finally`) — `</name>`. При исключении
   внутри блока teardown всё равно должен выполниться (закрывающий тег должен появиться),
   а само исключение — пробросить наружу. Это демонстрирует паттерн «try: yield … finally:».
6. Исключение `MissingKeyError(Exception)` и функция `wrap_lookup(d: dict, key)`.
   `wrap_lookup` достаёт `d[key]`; если ключ не найден, оборачивает `KeyError` в
   `MissingKeyError(f"key {key!r} not found")` через `raise … from err`. Вызывающий
   код должен уметь поймать `MissingKeyError` и при этом иметь доступ к оригинальному
   `KeyError` через `exc.__cause__`.

Запуск тестов:

```bash
./python/run.sh 08
```

### Подсказки

<details><summary>safe_divide — LBYL или EAFP?</summary>
Проверка `b == 0` дешёвая и очевидная — это классический LBYL. Можно и через
<code>try/except ZeroDivisionError</code>, тест примет оба варианта. Просто верни
<code>None</code>, когда делитель ноль, иначе обычное <code>a / b</code>.
</details>

<details><summary>parse_int — почему именно try/except</summary>
Строк, которые «похожи на число», слишком много вариантов («-7», « 10 », «3.14»,
«12x»), чтобы перебирать их руками. Доверь разбор самому <code>int(s)</code>: если он
смог — возвращаешь результат; если бросил <code>ValueError</code> — ловишь и возвращаешь
<code>None</code>. Это и есть EAFP.
</details>

<details><summary>get_or — как поймать «индекс вне диапазона»</summary>
Доступ <code>xs[i]</code> при плохом индексе бросает <code>IndexError</code>. Оберни
его в <code>try/except IndexError</code> и в обработчике верни <code>default</code>.
Не проверяй длину вручную — отрицательные индексы тогда легко обработать неверно;
пусть сам список решит, валиден индекс или нет.
</details>

<details><summary>Suppress — что хранить и что вернуть</summary>
В <code>__init__</code> сохрани переданные <code>exc_types</code> (это кортеж — как раз
в таком виде их понимает <code>isinstance</code>). <code>__enter__</code> вернёт
<code>self</code>. Вся соль — в <code>__exit__</code>.
</details>

<details><summary>Suppress.__exit__ — логика подавления</summary>
Если исключения не было, <code>exc_type</code> равен <code>None</code> — верни
<code>False</code> (нечего подавлять). Если исключение есть, проверь, относится ли оно
к нужным типам: <code>exc_type is not None and issubclass(exc_type, self.exc_types)</code>.
Если да — верни <code>True</code> (подавить), иначе <code>False</code> (пробросить).
Помни: <code>issubclass</code> по базовому классу накрывает и подклассы — поэтому
<code>Suppress(LookupError)</code> ловит и <code>IndexError</code>.
</details>

<details><summary>tag — структура генераторного CM</summary>
Импортируй <code>contextmanager</code> из <code>contextlib</code>. Декоратором оберни
обычную функцию-генератор. Структура:
<pre>
@contextmanager
def tag(name):
    print(f"&lt;{name}&gt;")
    try:
        yield            # здесь выполняется тело блока with
    finally:
        print(f"&lt;/{name}&gt;")  # выполнится при любом исходе, даже при исключении
</pre>
Ключевой момент: сам блок <code>try/finally</code> именно вокруг <code>yield</code>.
Без него исключение внутри <code>with</code> «прилетает» в точку <code>yield</code>
и код после него не выполнится.
</details>

<details><summary>wrap_lookup — как связать исключения через from</summary>
Поймай <code>KeyError</code> в блоке <code>except</code> и подними новое исключение
с явным указанием причины:
<pre>
except KeyError as err:
    raise MissingKeyError(f"key {key!r} not found") from err
</pre>
После этого у объекта <code>MissingKeyError</code> будет атрибут <code>__cause__</code>,
указывающий на оригинальный <code>KeyError</code>. Проверить в тесте:
<code>assert exc_info.value.__cause__ is not None</code> и
<code>assert isinstance(exc_info.value.__cause__, KeyError)</code>.
</details>

Реализовал — прогони тесты и приходи: покажи код, скажу, в верном ли направлении (и
заодно обсудим, где здесь честнее EAFP, а где LBYL).
