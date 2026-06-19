# Модуль 11 — Полезная стандартная библиотека

Большая идея модуля: *«батарейки в комплекте»* (batteries included) — это лозунг Python.
Львиную долю того, что в других языках приходится тащить сторонней библиотекой, здесь уже
лежит в **стандартной библиотеке**: счётчики, словари с дефолтом, работа с JSON, датами,
путями. Junior отличается от новичка не тем, что умеет написать цикл подсчёта вручную, а тем,
что **знает, что нужный инструмент уже есть**, и тянется за ним. Этот модуль — экскурсия по
самым ходовым «батарейкам».

## Идея 1. `collections.Counter` и `defaultdict`

Классическая задача: посчитать, сколько раз встречается каждый элемент. Можно вручную:

```python
counts = {}
for word in words:
    if word in counts:
        counts[word] += 1
    else:
        counts[word] = 1
```

Работает, но многословно. `Counter` делает это за один вызов и даёт бонусы:

```python
from collections import Counter

c = Counter(["a", "b", "a", "c", "a"])
# Counter({'a': 3, 'b': 1, 'c': 1})
c["a"]            # 3
c["я-не-тут"]     # 0 — отсутствующий ключ даёт 0, а не KeyError
c.most_common(1)  # [('a', 3)] — список (элемент, частота), от самого частого
c.most_common()   # все пары по убыванию частоты
```

ПОЧЕМУ это важно: `most_common(n)` отдаёт **n самых частых** уже отсортированными — не надо
самому возиться с сортировкой словаря по значениям. А `Counter` — это подкласс `dict`, так что
ведёт себя как обычный словарь, просто умнее.

Родственник — `defaultdict`. Проблема, которую он решает: когда значение словаря это *список*
(или другой контейнер), под каждый новый ключ его приходится заводить вручную:

```python
groups = {}
for w in words:
    key = w[0]
    if key not in groups:      # эту проверку легко забыть → KeyError
        groups[key] = []
    groups[key].append(w)
```

`defaultdict` берёт фабрику значения по умолчанию и сам создаёт пустой контейнер при первом
обращении к новому ключу:

```python
from collections import defaultdict

groups = defaultdict(list)     # фабрика — list, т.е. при новом ключе подставится []
for w in words:
    groups[w[0]].append(w)     # ключа ещё нет? defaultdict сам сделает [] и сразу .append
```

ПОЧЕМУ это важно: `defaultdict(list)` убирает шаблонную проверку «есть ли ключ». Передаём
**саму функцию** `list` (без скобок!) — это фабрика, которую вызовут, когда понадобится
дефолт. `defaultdict(int)` дал бы 0 по умолчанию (как раз ручной аналог `Counter`).

> Подвох: `defaultdict` *создаёт* ключ при простом чтении отсутствующего ключа — обращение
> `d[x]` уже вставит дефолт. Иногда это сюрприз. Для «просто посмотреть, не создавая» есть
> обычный `dict.get(x)`.

## Идея 2. `json`: `dumps` / `loads`

JSON — текстовый формат обмена данными, на котором говорит почти весь веб. Модуль `json`
переводит **объекты Python ↔ строку**:

```python
import json

data = {"name": "Ada", "age": 36, "langs": ["Python", "C++"]}
s = json.dumps(data)            # объект -> СТРОКА:  '{"name": "Ada", ...}'
back = json.loads(s)            # СТРОКА -> объект:  снова dict, равный data
back == data                    # True
```

Запомни направление по словам: **dump = «сбросить» наружу** (в строку/файл), **load =
«загрузить» внутрь** (разобрать строку обратно в объект). Буква `s` на конце (`dumps`/`loads`)
значит *string* — работа со строкой; без `s` (`dump`/`load`) — работа с файловым объектом.

ПОЧЕМУ это важно: JSON знает только базовые типы — `dict`, `list`, `str`, `int`/`float`,
`bool`, `None`. Они отображаются на объект/массив/строку/число/булево/`null`. Кортеж при
`dumps` превратится в массив и при `loads` вернётся уже **списком** — JSON не различает
`tuple` и `list`. Произвольный объект (например, своего класса) `json` сериализовать не умеет
и кинет `TypeError`. Ключи в JSON-объекте всегда строки.

> Удобно для глаз: `json.dumps(data, indent=2)` напечатает с отступами, а
> `ensure_ascii=False` оставит кириллицу как есть, а не в виде `\uXXXX`.

## Идея 3. `datetime`: разбор и разница дат

Даты нельзя считать «руками» (разное число дней в месяцах, високосные годы) — для этого есть
`datetime`. Нам сейчас хватит типа `date` (без времени):

```python
from datetime import date

d = date.fromisoformat("2024-03-01")   # разобрать строку ISO "YYYY-MM-DD" -> date
d.year, d.month, d.day                 # 2024, 3, 1
```

Главный трюк: даты можно **вычитать**. Результат — объект `timedelta`, у которого есть `.days`:

```python
a = date.fromisoformat("2024-01-01")
b = date.fromisoformat("2024-01-31")
delta = b - a                          # timedelta
delta.days                             # 30
```

ПОЧЕМУ это важно: `b - a` даёт *направленную* разницу. Если `a` позже `b`, `.days` будет
**отрицательным** (`a - b` для тех же дат даст `-30`). И `datetime` сам правильно учитывает
високосный год: между `2020-02-28` и `2020-03-01` будет **2** дня (2020-й високосный, есть
29 февраля), а между `2021-02-28` и `2021-03-01` — только **1**. Самому такое не посчитать без
ошибок — в этом вся ценность «батарейки».

> `fromisoformat` строгий: строку вроде `"01/03/2024"` он не примет и кинет `ValueError`.
> ISO-формат `YYYY-MM-DD` — единый язык дат, в котором сортировка строк совпадает с сортировкой
> по времени. Поэтому его и любят.

## Идея 4. `pathlib` и «батарейки в комплекте»

Раньше пути к файлам склеивали строками (`dir + "/" + name`) — и спотыкались о слэши,
разные на Windows и Linux. `pathlib` даёт объект `Path`, который знает про файловую систему:

```python
from pathlib import Path

p = Path("data") / "logs" / "app.txt"   # оператор / собирает путь по-человечески
p.name        # 'app.txt'
p.suffix      # '.txt'
p.exists()    # есть ли такой файл/папка
p.read_text(encoding="utf-8")            # прочитать весь файл одной строкой
```

Заданий по `pathlib` в этом модуле нет — просто знай, что он есть: когда дойдёшь до работы с
файлами (модуль 12), `Path` будет твоим инструментом по умолчанию вместо ручной склейки строк.

Это и есть смысл фразы **«батарейки в комплекте»**: `Counter`, `defaultdict`, `json`,
`datetime`, `pathlib`, а ещё `itertools`, `re`, `collections.deque` и десятки других модулей
идут вместе с Python. Прежде чем писать что-то «с нуля», задай себе вопрос junior-разработчика:
*а нет ли этого уже в стандартной библиотеке?* Очень часто — есть.

---

## Задания

Файл `stdlibtour.py`. Реализуй функции так, чтобы тесты позеленели:

1. `most_common_word(text: str) -> str` — вернуть самое частое **слово** в тексте.
   Слова разделены пробелами (`str.split()`), регистр и пунктуацию не нормализуем — берём как
   есть. Используй `collections.Counter`. На пустом/пробельном тексте верни пустую строку `""`.

2. `group_by_first_letter(words: list[str]) -> dict[str, list[str]]` — сгруппировать слова по
   их **первой букве**. Ключ — первая буква, значение — список слов с этой буквой **в исходном
   порядке**. Используй `collections.defaultdict`. Пустые строки в списке игнорируй. Вернуть
   можно обычный `dict`.

3. `to_json_str(obj) -> str` — сериализовать объект Python в JSON-строку через `json.dumps`.

4. `from_json_str(s: str)` — разобрать JSON-строку обратно в объект Python через `json.loads`.
   (Пара 3 и 4 — взаимно обратны: `from_json_str(to_json_str(x)) == x` для JSON-совместимых `x`.)

5. `days_between(a: str, b: str) -> int` — число дней между двумя датами в ISO-формате
   `"YYYY-MM-DD"`. Разбери обе через `datetime.date.fromisoformat`, верни `(b - a).days`.
   Знак сохраняется: если `a` позже `b`, результат отрицательный.

```bash
./python/run.sh 11
```

### Подсказки

<details><summary>Задание 1 — most_common_word</summary>
Разбей текст на слова через <code>text.split()</code> — без аргументов он сам схлопывает
лишние пробелы и не даёт пустых слов. Скорми список в <code>Counter(...)</code>. Метод
<code>most_common(1)</code> вернёт список вида <code>[(слово, частота)]</code> — тебе нужен
сам элемент <code>[0][0]</code>. Не забудь про пустой текст: если слов нет, верни <code>""</code>.
</details>
<details><summary>Задание 2 — group_by_first_letter</summary>
Заведи <code>defaultdict(list)</code>. Иди по словам; для каждого непустого слова возьми его
первую букву <code>w[0]</code> как ключ и сделай <code>groups[key].append(w)</code> — проверять
наличие ключа не нужно, в этом весь смысл <code>defaultdict</code>. Пустые строки пропусти
(<code>if not w: continue</code>). Порядок внутри списка получится исходным сам собой, ведь ты
добавляешь по ходу. Вернуть можно <code>dict(groups)</code> или сам <code>groups</code>.
</details>
<details><summary>Задания 3 и 4 — json туда и обратно</summary>
Это две строчки-обёртки. <code>to_json_str</code> — это <code>return json.dumps(obj)</code>.
<code>from_json_str</code> — это <code>return json.loads(s)</code>. Главное — не перепутать
направление: <code>dumps</code> делает строку, <code>loads</code> читает строку.
</details>
<details><summary>Задание 5 — days_between</summary>
<code>date.fromisoformat(a)</code> и <code>date.fromisoformat(b)</code> дадут два объекта
<code>date</code>. Их разность <code>b - a</code> — это <code>timedelta</code>; у него есть
поле <code>.days</code>. Просто верни его. Ничего не модулируй и не бери по модулю — знак нужен.
</details>

Напишешь функцию — покажи код, и я скажу, в верном ли направлении ты идёшь (и заодно подскажу,
где «батарейка» из стандартной библиотеки сэкономила бы тебе ещё пару строк).

---

## Задание (существенное): RecordAggregator — типизированный агрегатор записей

### Мотивация

Пять заданий выше — это отдельные «батарейки»: `Counter`, `defaultdict`, `json`, `datetime` —
каждая сама по себе. Но на практике эти инструменты работают *вместе*: ты получаешь поток
событий (логи, транзакции, клики), хочешь посчитать сводку, найти топ-категории, сравнить дни
— и в конце сохранить или передать результат через JSON. Задача этого задания — построить один
небольшой, но *нетривиальный* класс, который объединяет все четыре «батарейки» в единый
инструмент.

### Спецификация API

Реализуй класс `RecordAggregator` в файле `stdlibtour.py`.

**Запись** — это `dict` с тремя обязательными полями:

| Поле       | Тип           | Контракт                                              |
|------------|---------------|-------------------------------------------------------|
| `"date"`   | `str`         | ISO-формат `"YYYY-MM-DD"`, проверять через `fromisoformat` |
| `"category"` | `str`       | непустая строка                                       |
| `"value"`  | `int` / `float` | конечное число (не NaN, не Inf)                     |

**Методы:**

```python
agg = RecordAggregator()

agg.add(record: dict) -> None
    # Добавить запись. Валидировать все три поля; при нарушении — ValueError/TypeError.

agg.summary() -> dict
    # Ключи: count, categories, total_value, min_value, max_value, first_date, last_date.
    # На пустом агрегаторе: count=0, total_value=0.0, min_value/max_value/first_date/last_date=None,
    # categories={}.

agg.top_categories(n: int = 3) -> list[tuple[str, int]]
    # Топ-N категорий по убыванию частоты. Counter.most_common(n).
    # При n=0 или нет записей — [].

agg.daily_totals() -> dict[str, float]
    # {ISO-дата: сумма value}. Использовать defaultdict(float).

agg.to_json() -> str
    # Сериализовать состояние через json.dumps. Достаточно для round-trip.

RecordAggregator.from_json(s: str) -> RecordAggregator
    # Класс-метод. Восстановить через json.loads. После from_json(to_json(agg)):
    # summary() и daily_totals() совпадают с оригиналом.
```

**Краевые случаи:**

- Пустой агрегатор — все «нет данных» поля `None` или `{}` или `0.0`.
- `top_categories(n)` при `n` большем числа категорий — вернуть все категории.
- Отрицательный `value` — разрешён (возвраты, кредиты); `min_value` может быть отрицательным.
- Одна и та же дата в нескольких записях — суммы в `daily_totals()` накапливаются.
- `add()` должна проверять поля **перед** сохранением: невалидная запись не изменяет состояние.

### Подсказки

<details><summary>Шаг 1 — внутреннее состояние __init__</summary>

Думай о том, что потребуется для каждого метода:

- `summary()` нужны: счётчик записей, список всех `value`, список всех `date`, счётчик категорий.
- `daily_totals()` нужен: `defaultdict(float)` с накопленными суммами по датам.
- `to_json()` / `from_json()` нужно сохранить всё это в JSON-совместимые типы.

Заведи `self._records: list[dict]` — это проще всего: при `add()` просто добавляй в список,
а в остальных методах пересчитывай из него. Это не самый эффективный вариант (O(n) на каждый
вызов), но достаточный для учебной задачи.
</details>

<details><summary>Шаг 2 — валидация в add()</summary>

Три проверки по порядку:

1. **date**: `date.fromisoformat(record["date"])` — сам кинет `ValueError` на неправильном формате.
2. **category**: `if not record["category"]: raise ValueError(...)` — пустая строка фальшива.
3. **value**: `if not isinstance(record["value"], (int, float)): raise TypeError(...)`.
   Но осторожно: `bool` — подкласс `int` в Python (`isinstance(True, int)` — `True`).
   Реши, пропускать ли `bool` или тоже отбрасывать — смотри тесты, они подскажут.
   Для проверки конечности: `import math; math.isfinite(v)`.
</details>

<details><summary>Шаг 3 — summary()</summary>

Если `self._records` пуст — верни словарь с нулями/None прямо сразу.

Иначе:
- `count = len(self._records)`
- `categories = dict(Counter(r["category"] for r in self._records))`
- `values = [r["value"] for r in self._records]`; `total_value = float(sum(values))`; `min/max` — встроенные функции.
- `dates = [r["date"] for r in self._records]`; `first_date = min(dates)`, `last_date = max(dates)` — ISO-строки сортируются лексикографически правильно!
</details>

<details><summary>Шаг 4 — daily_totals()</summary>

```python
from collections import defaultdict
totals = defaultdict(float)
for r in self._records:
    totals[r["date"]] += r["value"]
return dict(totals)
```

Всё. `defaultdict(float)` создаёт `0.0` для нового ключа, поэтому `+=` работает с первого раза.
</details>

<details><summary>Шаг 5 — to_json() / from_json()</summary>

Простейший вариант — сохранять сам список `self._records`:

```python
def to_json(self) -> str:
    return json.dumps(self._records)

@classmethod
def from_json(cls, s: str) -> "RecordAggregator":
    agg = cls()
    for r in json.loads(s):
        agg.add(r)
    return agg
```

`json.dumps` умеет `int`, `float`, `str`, `list`, `dict` — как раз то, что внутри каждой записи.
После `json.loads` / повторного `add()` все инварианты сохраняются автоматически.
</details>
