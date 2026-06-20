# Модуль 17 — Concepts и ranges (C++20)

Большая идея: C++20 даёт два инструмента, которые делают обобщённый код **честным и
читаемым**. *Concepts* описывают, *каким* должен быть тип, прямо в сигнатуре — и
ошибки становятся понятными. *Ranges* и *views* позволяют собирать обработку данных
в ленивый конвейер из маленьких шагов, не плодя промежуточных коллекций.

В модуле 07 ты уже встречал готовые концепты (`std::integral`). Здесь ты научишься
**писать свои** и строить конвейеры из `std::views`.

## Идея 1. Свой концепт через requires-выражение

Концепт — это именованный предикат над типом: компилируемое «да/нет».

```cpp
#include <concepts>

template <class T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;   // (a + b) валидно и даёт что-то, конвертируемое в T
};

static_assert(Addable<int>);          // да
static_assert(!Addable<std::vector<int>>);  // нет: у вектора нет operator+
```

Внутри `requires(T a, T b) { ... }` ты перечисляешь выражения, которые **обязаны
компилироваться**. Запись `{ выражение } -> Концепт;` добавляет требование к *типу*
результата. Концепт не выполняет код — он лишь проверяет, что выражение валидно.

**Почему это лучше:** требование к типу видно в одном месте и читается как
документация, а не прячется в глубине шаблона.

Подвох: `requires(T a, T b)` объявляет *воображаемые* `a`, `b` — они не создаются
по-настоящему. Поэтому концепт работает даже для типов без конструктора по умолчанию.

## Идея 2. Концепт-предикат на наличие операций (begin/end)

Концепт умеет проверять не только арифметику, но и «есть ли у типа нужные методы».

```cpp
#include <iterator>

template <class C>
concept Container = requires(C c) {
    std::begin(c);          // есть begin
    std::end(c);            // есть end
    *std::begin(c);         // по begin можно разыменоваться
};

static_assert(Container<std::vector<int>>);
static_assert(!Container<int>);
```

`std::begin`/`std::end` работают и со встроенными массивами, и со всеми STL-контейнерами,
поэтому опираться лучше на них, а не на методы `.begin()`.

Подвох: если написать просто `c.begin()`, концепт не пройдёт для сырого массива `int[5]`
— у него нет метода, но `std::begin` для него есть.

## Идея 3. Ограничение шаблона концептом vs SFINAE

Концепт можно поставить прямо как «тип параметра» или через `requires`:

```cpp
template <Addable T>          // короткая форма
T twice(T x) { return x + x; }

template <class T>
    requires Addable<T>       // эквивалент через requires-клаузу
T twice2(T x) { return x + x; }
```

Раньше то же делали через **SFINAE** — `std::enable_if`:

```cpp
template <class T,
          class = std::enable_if_t<std::is_arithmetic_v<T>>>
T twice_old(T x) { return x + x; }   // работает, но нечитаемо, и ошибка страшная
```

Разница: SFINAE «молча выкидывает» перегрузку, а сообщение об ошибке у неудачного вызова
— простыня на полэкрана. Концепт же скажет коротко: *«ограничение Addable<T> не
выполнено»*. Концепты — это SFINAE, ставшая человекочитаемой.

**Почему это важно:** при обучении и в большой кодовой базе понятная ошибка экономит часы.

## Идея 4. Ranges и views: ленивость и композиция

`view` — это *лёгкий* объект-обёртка над диапазоном, который **ничего не вычисляет, пока
ты по нему не пройдёшься**. Шаги соединяются через `operator|`:

```cpp
#include <ranges>
#include <vector>

std::vector<int> v{1, 2, 3, 4, 5, 6};
auto pipeline = v
    | std::views::filter([](int x) { return x % 2 == 0; })  // 2,4,6
    | std::views::transform([](int x) { return x * 2; })     // 4,8,12
    | std::views::take(2);                                    // 4,8
// здесь ещё НИЧЕГО не посчитано — pipeline ленив
for (int x : pipeline) { /* вот теперь шаги выполняются по одному элементу */ }
```

Чтобы получить обычный `std::vector`, материализуй диапазон вручную:

```cpp
std::vector<int> out(pipeline.begin(), pipeline.end());
```

**Почему это удобно:** нет промежуточных векторов «после filter», «после transform» — данные
текут по одному элементу. `take(k)` к тому же останавливает работу, как только набрал `k`.

Подвох: view **не владеет** данными. Если исходный вектор умрёт, view станет «висячим».
И не забывай материализовать результат, если функция должна вернуть именно `std::vector`.

## Идея 5. Проекции

Многие ranges-алгоритмы принимают *проекцию* — функцию, которая говорит, «по какому
полю смотреть»:

```cpp
struct Person { std::string name; int age; };
std::vector<Person> people = /* ... */;
std::ranges::sort(people, {}, &Person::age);   // сортировать по age, не трогая name
```

Третий аргумент `&Person::age` — проекция: сравнение идёт по `p.age`, а двигаются целые
`Person`. Это избавляет от лямбды `[](auto& a, auto& b){ return a.age < b.age; }`.

## Идея 6. Sentinel и «разнотипные» пары begin/end

В классическом STL `begin()` и `end()` возвращают **один и тот же тип**. C++20 ranges
ослабляет это требование: конец диапазона может быть *sentinel* — объектом специального
типа, который уметь только сравниваться с итератором. Контракт:

```cpp
template <class S, class I>
concept std::sentinel_for<S, I> =
    std::semiregular<S> &&
    std::input_or_output_iterator<I> &&
    /* можно написать it != s */ requires(I i, S s) { { i != s } -> std::convertible_to<bool>; };
```

**Зачем это нужно.** Рассмотрим `std::ranges::iota_view` (бесконечная последовательность
целых) или нуль-терминированную C-строку:

```cpp
// iota_view<int> — конец никогда не наступит, end() — специальный sentinel-объект
auto naturals = std::views::iota(0);   // begin() -> iota_iterator, end() -> unreachable_sentinel_t

// Нуль-терминированная строка: sentinel — сравнение с '\0'
struct NullSentinel {};
struct CharPtr {
    const char* p;
    bool operator!=(NullSentinel) const { return *p != '\0'; }
    char operator*()              const { return *p; }
    CharPtr& operator++()               { ++p; return *this; }
};
// begin=CharPtr{"hello"}, end=NullSentinel{} — разные типы, но range-for работает
```

**Механика в range-for.** Компилятор разворачивает:
```cpp
for (auto&& e : rng) { ... }
// в:
auto it  = std::ranges::begin(rng);
auto snt = std::ranges::end(rng);
for (; it != snt; ++it) { auto&& e = *it; ... }
```
`it != snt` — это гетерогенное сравнение. Оба типа не обязаны совпадать; достаточно,
чтобы `I::operator!=(S)` (или свободная `operator!=`) было определено.

**Почему это важно:** разнотипный sentinel позволяет создавать диапазоны без вычисления
длины заранее — «длина» выясняется по ходу итерации. Это открывает бесконечные
последовательности (`iota`, генераторы) и «ленивые» терминаторы (парсинг файла до EOF).

Подвох: обычный `std::iterator_traits` расчитан на одинаковые типы. Поэтому `std::ranges`-
алгоритмы написаны с двумя параметрами `I, S`, а не одним `It`. Если вы передаёте
sentinel в старый `std::sort` — он не скомпилируется.

## Идея 7. Категории диапазонов: иерархия мощностей

Каждый диапазон обладает *минимальным* гарантированным набором операций. Иерархия:

```
input_range ⊂ forward_range ⊂ bidirectional_range ⊂ random_access_range ⊂ contiguous_range
```

| Категория         | Что умеет                         | Пример                         |
|-------------------|-----------------------------------|--------------------------------|
| `input_range`     | однопроходный `++it`, `*it`       | `std::istream_view`            |
| `forward_range`   | многопроходный, копируемый `it`   | `std::forward_list`            |
| `bidirectional`   | ещё `--it`                        | `std::list`, `std::set`        |
| `random_access`   | ещё `it + n`, `it[n]`, `it1 - it2`| `std::deque`, `std::vector`   |
| `contiguous`      | ещё гарантированный непрерывный блок памяти | `std::array`, `std::vector` |

**Почему `std::list` — bidirectional, а не random_access.**  
`std::list` хранит узлы **рассыпанными по памяти**, связанными указателями. Чтобы добраться
до `it + 5`, нужно пройти по пяти указателям: O(n). Именно поэтому `operator+` и `operator[]`
у такого итератора нет — они были бы молчаливым O(n), что противоречит принципу *«цена
соответствует синтаксису»*. Двунаправленность же (`--it`) — O(1): у каждого узла есть
указатель назад.

Следствие: `std::sort` требует `random_access_range` (чтобы быстро менять местами любые
элементы). Вот почему `std::list` не работает с `std::sort` — нужен отдельный `list::sort()`.

**Практическая проверка категории диапазона:**

```cpp
#include <ranges>
static_assert(std::ranges::random_access_range<std::vector<int>>);  // OK
static_assert(std::ranges::bidirectional_range<std::list<int>>);     // OK
static_assert(!std::ranges::random_access_range<std::list<int>>);   // list — не random-access
```

Готовые стандартные концепты `std::ranges::range`, `std::ranges::input_range`,
`std::ranges::random_access_range` и т. д. **следует предпочитать самописным**:
они уже учли все угловые случаи (sentinel, итерируемость, `std::ranges::begin`).

## Идея 8. Subsumption: частичный порядок концептов при перегрузке

Концепты образуют **частичный порядок** — компилятор умеет автоматически выбирать
«наиболее специализированную» перегрузку. Это называется *subsumption* («поглощение»):

```cpp
template <std::ranges::range R>
void process(R&& r) { /* общий алгоритм для любого диапазона */ }

template <std::ranges::random_access_range R>
void process(R&& r) { /* оптимизация для random-access */ }
```

При вызове `process(std::vector<int>{})` компилятор знает: `random_access_range` *включает*
`range` как требование (то есть любой `random_access_range` автоматически является `range`).
Поэтому вторая перегрузка **более специализирована** — она и победит без всякой
неоднозначности.

**Механика поглощения.** Концепт `B` поглощает концепт `A`, если каждое ограничение `A`
содержится в `B` в виде *атомарного* подвыражения (нормальная форма). Стандартные
`std::ranges::*` концепты построены именно так — каждый следующий уровень иерархии
включает требования предыдущего через `&&`.

**Подвох — только атомарные requires.** Subsumption работает только для концептов,
определённых через другие именованные концепты. Два концепта с одинаковым телом `requires`
— но разными именами — **не** поглощают друг друга:

```cpp
template <class T> concept A = requires(T x) { x + x; };
template <class T> concept B = requires(T x) { x + x; };
// A и B — НЕ subsume друг друга, даже если тела идентичны!
```

Правило: используйте стандартные концепты как строительные блоки и включайте их через
`&&`, а не копируйте их тела.

## Идея 9. std::ranges-алгоритмы с проекциями: сравнение со старыми алгоритмами

`std::ranges`-версии алгоритмов отличаются от `std::`-версий в трёх важных аспектах:

1. **Принимают диапазон целиком**, а не пару итераторов.
2. **Принимают проекцию** как отдельный параметр (третий/четвёртый).
3. **Работают с sentinel**-концом диапазона.

```cpp
struct Person { std::string name; int age; };
std::vector<Person> people{ {"Зина", 30}, {"Алёша", 22}, {"Маша", 25} };

// --- старый способ (std::) ---
std::sort(people.begin(), people.end(),
          [](const Person& a, const Person& b) { return a.age < b.age; });

auto it1 = std::min_element(people.begin(), people.end(),
          [](const Person& a, const Person& b) { return a.age < b.age; });

// --- новый способ (std::ranges::) ---
std::ranges::sort(people, {}, &Person::age);          // {} = дефолтный компаратор less{}

auto it2 = std::ranges::min_element(people, {}, &Person::age);
if (it2 != people.end()) std::cout << it2->name;     // "Алёша"
```

**Как работает проекция внутри.** Алгоритм не сравнивает элементы напрямую, а сначала
применяет к каждому проекцию, и уже результаты сравнивает:

```
сравнивает proj(a) с proj(b)
вместо a с b
```

Проекция может быть: указателем на поле (`&Person::age`), указателем на метод
(`&Person::toString`), лямбдой (`[](const Person& p){ return p.age * 2; }`).

**Подвох:** `{}` на месте компаратора означает `std::ranges::less{}` (или `std::less{}`),
а не «нет компаратора». Если вы хотите сортировать по убыванию — передайте
`std::ranges::greater{}`:

```cpp
std::ranges::sort(people, std::ranges::greater{}, &Person::age);
```

`std::ranges::find_if` принимает проекцию четвёртым аргументом:

```cpp
auto young = std::ranges::find_if(people,
    [](int a) { return a < 25; },   // предикат — по результату проекции
    &Person::age);                  // проекция
```

## Идея 10. Обзор range-адаптеров: шире filter/transform/take

`<ranges>` содержит богатый набор адаптеров. Вот практически полезные:

```cpp
#include <ranges>
#include <vector>
#include <iostream>

std::vector v{1, 2, 3, 4, 5};

// iota — бесконечная (или ограниченная) последовательность целых
for (int x : std::views::iota(1, 6))       std::cout << x;  // 1 2 3 4 5

// reverse — обратный порядок (требует bidirectional_range)
for (int x : v | std::views::reverse)      std::cout << x;  // 5 4 3 2 1

// drop — пропустить первые N элементов
for (int x : v | std::views::drop(2))      std::cout << x;  // 3 4 5

// enumerate (C++23 — в теории, упомянуть) — пары (индекс, значение)
// for (auto [i, x] : v | std::views::enumerate) ...

// zip (C++23) — объединить два диапазона поэлементно
// for (auto [a, b] : std::views::zip(v1, v2)) ...

// Цепочка из нескольких адаптеров:
auto result = std::views::iota(1)                // 1, 2, 3, 4, ...
    | std::views::filter([](int x){ return x%2==0; })  // 2, 4, 6, ...
    | std::views::transform([](int x){ return x*x; })  // 4, 16, 36, ...
    | std::views::take(4);                             // 4, 16, 36, 64
```

`enumerate` и `zip` появляются в стандарте C++23. В C++20 их можно заменить
`std::ranges::views::zip` через Range-v3 или реализовать вручную.

**Ключевое свойство всех адаптеров:** они *ленивы* — вычисляют значение только при
переходе к очередному элементу. Цепочка из пяти адаптеров не создаёт пяти промежуточных
векторов — каждый элемент проходит всю цепочку за один шаг.

---

## Задания (пиши прямо в `include/concepts_ranges.hpp`, всё в namespace `m17`)

1. Почини два концепта:
   - `Addable<T>` — истинен, если для двух значений `T` валидно выражение `a + b`, и его
     результат конвертируется в `T`.
   - `Container<C>` — истинен, если у `C` есть `std::begin`/`std::end` и по `begin` можно
     разыменоваться (`*begin`).
2. `sum_container(c)` — уже ограничена `Container` + `Addable`-элементами. Реализуй тело:
   пройди по контейнеру и накопи сумму в `Value{}`. Для пустого верни `Value{}`.
3. `even_times_two_take(xs, k)` — собери ленивый конвейер `std::views`: оставить чётные
   ИСХОДНЫЕ числа, удвоить, взять первые `k`, материализовать в `std::vector<int>`.
4. Почини концепт `Sortable<R>` (диапазон со случайным доступом, элементы сравнимы через
   `operator<`) и реализуй `sorted_copy(r)` — отсортированную по возрастанию копию-вектор.
   Несортируемый тип (например `std::list` или элемент без `<`) должен давать ошибку
   **компиляции**.
5. `take_n(range, n)` — свой адаптер: верни `std::vector` первых `n` элементов, проходя
   **вручную** по итераторам `begin`/`end` (без `std::views::take`). Меньше `n` элементов —
   верни сколько есть.
6. **Проекции: sort и min_element по полю.** Дан `struct Person { std::string name; int age; }`.
   Реализуй две функции:
   - `sort_by_age(people)` — принимает `std::vector<Person>` по значению, сортирует его
     **на месте** по полю `age` через `std::ranges::sort` с проекцией `&Person::age`,
     возвращает отсортированный вектор.
   - `youngest_name(people)` — принимает `const std::vector<Person>&`, возвращает `std::string`
     с именем самого молодого (минимальный `age`) через `std::ranges::min_element` с проекцией.
     Для пустого вектора верни пустую строку `""`.

   Самописный концепт здесь не нужен — используй стандартные алгоритмы напрямую.

```bash
./cpp/run.sh 17
```

### Подсказки

<details><summary>Задание 1 — форма requires-выражения</summary>
`template <class T> concept Addable = requires(T a, T b) { ... };`. Внутри фигурных
скобок перечисли требуемые выражения. Для проверки типа результата используй форму
`{ a + b } -> std::convertible_to<T>;`. Для `Container` опирайся на `std::begin(c)` и
`std::end(c)`, а не на методы `.begin()`.
</details>

<details><summary>Задание 2 — аккумулятор правильного типа</summary>
Тип элемента уже выведен за тебя в параметр `Value`. Заведи `Value total{};` и в
range-for прибавляй к нему каждый элемент. Не пиши `int total = 0;` — `Value` может быть
`double` или твоим `Money`.
</details>

<details><summary>Задание 3 — три шага и материализация</summary>
Подключи `<ranges>`. Составь `xs | std::views::filter(...) | std::views::transform(...)
| std::views::take(k)`. Предикат фильтра смотрит на исходное число (`x % 2 == 0`),
трансформация уже на отфильтрованных. В конце построй вектор из `pipeline.begin()`,
`pipeline.end()`. Подумай, почему важен именно такой порядок шагов.
</details>

<details><summary>Задание 4 — что значит «random-access» в концепте</summary>
Тебе нужны два требования. Первое — итератор диапазона должен быть random-access: это
проверяется концептом `std::random_access_iterator<...>` над типом
`decltype(std::begin(std::declval<R&>()))` (или через `std::ranges::random_access_range<R>`).
Второе — элементы сравнимы: `std::totally_ordered<Value>` либо требование `{ a < b }` в
requires. В `sorted_copy` скопируй элементы в вектор и вызови `std::sort`. Копия — значит
исходник не меняем.
</details>

<details><summary>Задание 5 — ручной обход без take</summary>
Возьми `auto it = std::begin(range); auto last = std::end(range);` и крути цикл, пока
`it != last` И набрано меньше `n`. На каждом шаге `push_back(*it)` и `++it`. Так ты не
выйдешь за `end()`, даже если элементов меньше `n`.
</details>

<details><summary>Задание 6 — sort с проекцией и min_element с проекцией</summary>
`std::ranges::sort` принимает три аргумента: диапазон, компаратор и проекцию. Для сортировки
по возрастанию компаратор можно передать как `{}` (дефолтный `less`), а проекцию —
указателем на поле: `std::ranges::sort(v, {}, &Person::age)`.

`std::ranges::min_element` аналогично: `auto it = std::ranges::min_element(people, {}, &Person::age)`.
Не забудь проверить, что итератор не равен `people.end()`, прежде чем брать `it->name`.

Для пустого вектора в `youngest_name` сразу верни `""` — проверь `people.empty()` в начале.
</details>

После задания 4 попробуй вызвать `sorted_copy` от `std::list<int>` и прочитай, что скажет
компилятор про невыполненное ограничение `Sortable`. Покажешь сообщение — разберём, чем
оно лучше старой SFINAE-простыни.
