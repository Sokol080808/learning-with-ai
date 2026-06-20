# Модуль 16 — Продвинутые шаблоны: variadic, SFINAE, трейты, CRTP

Большая идея: шаблоны умеют гораздо больше, чем «одна функция для любого типа». Они
умеют принимать **сколько угодно** аргументов, **выбирать** разные реализации по
свойствам типа на этапе компиляции и даже **встраивать** общее поведение в классы без
наследования с виртуальными функциями. Здесь мы соберём этот инструментарий: пакеты
параметров, fold-выражения, SFINAE, собственные трейты, tag dispatch и CRTP. Всё —
header-only: пишешь прямо в `include/task16.hpp`.

## Идея 1. Variadic templates и parameter packs

Шаблон может принимать **переменное число** типов — это пакет параметров `Ts...`:

```cpp
template <class... Ts>          // Ts — пакет типов
void show(Ts... args) {         // args — пакет значений
    // sizeof...(Ts) — сколько типов в пакете
}
```
Многоточие `...` имеет два смысла: при объявлении — «пакет», при использовании —
«распакуй меня». Пакет нельзя просто так перебрать в цикле — его надо *развернуть*:
либо fold-выражением (Идея 2), либо рекурсией (Идея 3).

**Подвох:** `sizeof...(Ts)` — это про количество элементов пакета, а не размер типа.
Не путай с обычным `sizeof`.

## Идея 2. Fold-выражения (C++17)

Самый короткий способ «свернуть» пакет одной операцией:

```cpp
template <class... Ts>
auto add_all(Ts... xs) {
    return (xs + ...);          // x0 + x1 + ... + xN   (унарная правая свёртка)
}
```
Формы: `(xs + ...)` и `(... + xs)` — правая и левая свёртки; `(xs + ... + 0)` —
бинарная свёртка с начальным значением (спасает от пустого пакета). Работает с любым
бинарным оператором, включая `,` — это позволяет «сделать что-то с каждым элементом»:

```cpp
template <class... Ts>
void print_each(const Ts&... xs) {
    ((std::cout << xs << ' '), ...);   // запятая-свёртка: вызвать для каждого
}
```

**Подвох:** у *унарной* свёртки пустой пакет разрешён лишь для `&&`, `||` и `,`. Для
`+` по пустому пакету будет ошибка — если нужен «ноль элементов», используй бинарную
форму с начальным значением.

## Идея 3. Рекурсивная распаковка пакета

До C++17 (а иногда и сейчас, когда логика сложнее свёртки) пакет разбирают рекурсией:
«голова + хвост».

```cpp
int sum_rec() { return 0; }                       // база: пустой пакет
template <class T, class... Rest>
int sum_rec(T head, Rest... rest) {               // голова + остаток
    return head + sum_rec(rest...);               // рекурсивно по хвосту
}
```
Каждый вызов «съедает» один аргумент, пока не дойдёт до базы. Это классический паттерн;
fold-выражение — его краткая запись для простых случаев.

**Подвох:** забудешь базовый случай (перегрузку без аргументов) — рекурсия не
завершится и компилятор выдаст ошибку про отсутствие подходящей перегрузки.

## Идея 4. SFINAE и `std::enable_if`

SFINAE = *Substitution Failure Is Not An Error*. Если при подстановке типа в шаблон
получается некорректная сигнатура — это **не** ошибка компиляции, а сигнал «эта
перегрузка не подходит, ищи другую». На этом строится выбор реализации по свойствам
типа:

```cpp
#include <type_traits>

template <class T,
          std::enable_if_t<std::is_integral_v<T>, int> = 0>
std::string kind(T) { return "integer"; }

template <class T,
          std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
std::string kind(T) { return "floating"; }
```
`enable_if_t<условие, тип>` существует (даёт `тип`) только если `условие` истинно.
Когда условие ложно — `::type` нет, подстановка проваливается, и эта перегрузка тихо
выпадает из набора кандидатов. Остаётся ровно одна подходящая.

**Подвох:** обе перегрузки `kind` имеют одинаковую сигнатуру по «видимым» параметрам.
Различие прячется в дефолтном шаблонном параметре. Если бы условия пересекались (тип
подходил под оба `enable_if`), получили бы неоднозначность — следи, чтобы ветки были
взаимоисключающими.

## Идея 5. Свои type traits

Трейт — это шаблон, который вычисляет факт о типе на этапе компиляции через **частичную
специализацию**. Базовый случай задаёт значение по умолчанию, специализация ловит
интересный паттерн типа:

```cpp
template <class T> struct is_pointer      { static constexpr bool value = false; };
template <class T> struct is_pointer<T*>  { static constexpr bool value = true;  };
// T* — частичная специализация: «если T имеет форму что-то-со-звёздочкой»
```
Так же снимают `const`:
```cpp
template <class T> struct remove_const          { using type = T; };
template <class T> struct remove_const<const T> { using type = T; };
```
Удобные алиасы `_t` и `_v` — это просто обёртки: `remove_const_t<T> =
typename remove_const<T>::type`, `is_pointer_v<T> = is_pointer<T>::value`.

**Подвох:** `remove_const` снимает const только с **верхнего уровня**. У `const char*`
константен указатель на `char`, а не сам указатель, поэтому здесь ничего не снимется —
тип останется `const char*`. Это правильно, и тест на это смотрит.

## Идея 6. Tag dispatch

Иногда выбор реализации удобнее делать не через `enable_if`, а через **тип-тег** —
пустую структуру, которую передают лишним аргументом. Перегрузки различаются типом
тега, и компилятор выбирает нужную:

```cpp
struct slow_tag {};
struct fast_tag {};

template <class It> long count(It b, It e, slow_tag) { /* в цикле */ }
template <class It> long count(It b, It e, fast_tag) { return e - b; }
```
В стандартной библиотеке теги уже готовы: у каждого итератора есть «категория» —
`std::random_access_iterator_tag`, `std::forward_iterator_tag` и т.д. Получить её можно
так:
```cpp
typename std::iterator_traits<It>::iterator_category{}
```
`std::vector` даёт random-access итератор (можно `e - b`), `std::list` — только
двунаправленный (приходится шагать в цикле). Tag dispatch выбирает быстрый путь там,
где он доступен.

**Подвох:** тег нужно создать как **объект** (с `{}`), а не передать как тип — это
обычный аргумент функции, по которому идёт разрешение перегрузки.

## Идея 7. CRTP — миксин без виртуальных функций

CRTP (*Curiously Recurring Template Pattern*) — класс наследуется от шаблона,
параметризованного **самим собой**:

```cpp
template <class Derived>
struct Printable {
    void print() const {
        const Derived& d = static_cast<const Derived&>(*this);  // знаем реальный тип
        d.write();  // вызываем метод производного класса — статически, без virtual
    }
};
struct Doc : Printable<Doc> {
    void write() const { /* ... */ }
};
```
База через `static_cast` «знает» свой производный тип и может звать его методы. Так
встраивают общее поведение, выводимое из пары базовых операций. Классика — `Comparable`:
производный класс даёт `<` и `==`, а миксин достраивает `!=`, `>`, `<=`, `>=`.

**Подвох:** `static_cast` к `Derived` корректен, только если объект *действительно*
`Derived`. CRTP это гарантирует (наследник передаёт себя параметром), но если ошибёшься
в параметре — `struct A : Printable<B>` — получишь невалидный каст и UB.

## Идея 8а. if constexpr — современная замена SFINAE-ветвлениям

`enable_if` задуман как *фильтр перегрузок* (SFINAE), но его часто применяют, чтобы
просто выбрать одну из двух ветвей логики внутри одной функции. C++17 даёт
элегантную замену — `if constexpr`:

```cpp
// Старый стиль: две отдельные перегрузки через enable_if
template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
std::string describe_v1(T) { return "integer"; }
template <class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
std::string describe_v1(T) { return "floating"; }

// Новый стиль: одна функция с if constexpr
template <class T>
std::string describe_v2(T) {
    if constexpr (std::is_integral_v<T>) {
        return "integer";
    } else if constexpr (std::is_floating_point_v<T>) {
        return "floating";
    } else {
        return "other";
    }
}
```

**Ключевая механика:** тело `if constexpr` компилируется *только* если условие
истинно при подстановке типа. Остальные ветки синтаксически проверяются, но не
инстанцируются — поэтому в `else`-ветке можно писать выражения, недопустимые для
других типов (например, `x.size()` когда T — не контейнер).

**Когда что использовать:**

| Ситуация | Лучший инструмент |
|---|---|
| Нужно **отключить** перегрузку целиком (например, для метапрограммирования через SFINAE-интроспекцию) | `enable_if` / `requires` |
| Нужно выбрать **ветку логики** внутри одной функции | `if constexpr` |
| Код на C++20 и условие выражается концептом | `if constexpr` с `requires` или `concept` |

**Подвох:** `if constexpr` не делает из одной перегрузки несколько — функция всё
равно одна. Если сигнатуры должны отличаться (тип возвращаемого значения,
параметры), `enable_if` по-прежнему нужен.

## Идея 8б. C++20 Concepts — читаемая замена SFINAE

SFINAE работает, но ошибки от него — «стена текста». Concepts (C++20) именуют
ограничения и дают человекочитаемые диагностики.

**Механика `requires`-выражения:**
```cpp
// Именованный концепт: тип T должен поддерживать operator+ и иметь конструктор из 0.
template <class T>
concept Addable = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;    // выражение должно быть, тип совместим
};
```

**Два способа ограничить шаблон:**
```cpp
// 1. Requires-clause в объявлении
template <class T>
    requires Addable<T>
T double_val(T x) { return x + x; }

// 2. Сокращённая форма (abbreviated template)
Addable auto double_val(Addable auto x) { return x + x; }
```

**Сравнение с enable_if:**
```cpp
// enable_if — зашумлённая сигнатура:
template <class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
T increment(T x) { return x + 1; }

// concept — чисто и явно:
template <std::integral T>   // std::integral — встроенный концепт
T increment(T x) { return x + 1; }
```

При нарушении концепта компилятор печатает:
```
note: 'double' does not satisfy 'integral'
```
— вместо многостраничного SFINAE-следа.

**Написать собственный концепт с нуля:**
```cpp
// Проверяем, что у типа есть метод .size() -> size_t
template <class T>
concept HasSize = requires(const T& x) {
    { x.size() } -> std::same_as<std::size_t>;
};
```

**Подвох:** концепты — compile-time предикаты, а не рантайм. Нарушение концепта
при вызове — ошибка компиляции, а не исключение.

## Идея 8в. void_t и Detection Idiom — интроспекция методов/типов

Иногда нужно проверить: «есть ли у типа T метод `.size()`?», «есть ли у него
вложенный тип `value_type`?» — без concepts. Для этого существует `std::void_t`
(C++17) и паттерн *detection idiom*.

**Механика `void_t`:** это шаблонный алиас `template<class...> using void_t = void`.
Его сила в том, что он «поглощает» любые типы — но если при подстановке
возникает ошибка, SFINAE откатывает специализацию.

```cpp
// Базовый трейт: T не имеет .size() по умолчанию.
template <class T, class = void>
struct has_size_method : std::false_type {};

// Специализация: работает только когда выражение t.size() корректно для T.
template <class T>
struct has_size_method<T,
    std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

template <class T>
inline constexpr bool has_size_method_v = has_size_method<T>::value;
```

Тогда:
```cpp
static_assert( has_size_method_v<std::vector<int>>);  // OK: есть .size()
static_assert(!has_size_method_v<int>);               // OK: нет .size()
```

**Аналогично — проверить наличие вложенного типа `value_type`:**
```cpp
template <class T, class = void>
struct is_iterable : std::false_type {};

template <class T>
struct is_iterable<T,
    std::void_t<decltype(std::begin(std::declval<T>())),
                decltype(std::end(std::declval<T>()))>>
    : std::true_type {};
```

**std::declval<T>()** — хелпер, который «создаёт» T прямо в непроверяемом
контексте, не требуя конструктора: нужен для написания выражений внутри `decltype`.

**Подвох:** `void_t` чувствителен к порядку аргументов: дефолтный параметр
`class = void` в основном шаблоне должен совпадать с тем, что подставляет
специализация — иначе компилятор не рассмотрит специализацию как более частный
случай.

## Идея 8г. CRTP: компромиссы против виртуального полиморфизма

CRTP и `virtual` решают похожие задачи — но по-разному. Понимать различие важно,
чтобы выбирать правильно.

| | CRTP | `virtual` |
|---|---|---|
| **Диспетчеризация** | статическая (compile-time) | динамическая (runtime) |
| **vtable-оверхед** | нет | +1 указатель на объект + indirect call |
| **Heterogeneous containers** | невозможны — `Base*` хранит только Base | `std::vector<Shape*>` — легко |
| **Code bloat** | каждая специализация инстанцирует свой набор функций | одна реализация в Base |
| **Цена inline-инлайнинга** | метод базы видит конкретный тип → может инлайниться | виртуальный вызов devirtualize-нуть труднее |

**CRTP уместен**, когда:
- Производительность критична и набор типов известен на этапе компиляции.
- Хочешь «вмешать» несколько поведений в один класс через несколько CRTP-миксинов
  (например, `Comparable<D>` + `Printable<D>`).
- Тебе не нужна гетерогенная коллекция (не хочется хранить разные Derived в одном
  контейнере через общий Base*).

**`virtual` уместен**, когда:
- Типы определяются в runtime (plugin-система, полиморфный граф объектов).
- Нужен гетерогенный контейнер (`std::vector<Shape*>`).
- Набор производных классов не известен заранее (открытая иерархия).

**Конкретный code-bloat пример:** если `Logger<FileLogger>` и `Logger<NetworkLogger>`
оба инстанцируют шаблонные методы базы — получаем две копии бинарного кода вместо
одной виртуальной функции. При десятках специализаций это реально раздувает бинарь.

**Ограничение CRTP на гетерогенные контейнеры:**
```cpp
// НЕ работает — Base<Derived> — разные типы для Dog и Cat:
// std::vector<Animal*> zoo;    // Animal чего? Animal<Dog>? Animal<Cat>?

// Выход: дополнительная нешаблонная база (CRTP + virtual):
struct AnimalBase { virtual void speak() = 0; };
template <class D>
struct Animal : AnimalBase { /* CRTP-миксины */ };
```

## Идея 8д. Fold-выражения: &&, || и вывод в поток

В Идее 2 рассмотрена свёртка по `+`. Рассмотрим ещё три важных случая.

**Логические свёртки** работают с пустым пакетом корректно:

```cpp
// Истина, если ВСЕ аргументы удовлетворяют предикату
template <class Pred, class... Ts>
bool all_of_fold(Pred p, Ts... xs) {
    return (p(xs) && ...);   // пустой пакет → true (нейтральный элемент &&)
}

// Истина, если ХОТЯ БЫ ОДИН удовлетворяет
template <class Pred, class... Ts>
bool any_of_fold(Pred p, Ts... xs) {
    return (p(xs) || ...);   // пустой пакет → false (нейтральный элемент ||)
}
```

Пустой пакет для `&&` возвращает `true` (как в математике: «все из 0 — верно»,
vacuous truth), для `||` — `false`.

**Вывод в поток** через свёртку по `<<`:
```cpp
template <class... Ts>
void print_tuple(const Ts&... xs) {
    ((std::cout << xs), ...);   // запятая-свёртка — печатает каждый без разделителя
}

// Или с разделителем (бинарная форма):
template <class... Ts>
void print_csv(const Ts&... xs) {
    bool first = true;
    (( std::cout << (first ? "" : ", ") << xs, first = false ), ...);
}
```

**Свёртка по <<** через бинарную форму:
```cpp
// Наполнить ostream — идиоматично в логгерах:
template <class... Ts>
std::string concat_str(Ts... xs) {
    std::ostringstream oss;
    (oss << ... << xs);    // левая бинарная свёртка: ((oss << x0) << x1) << ...
    return oss.str();
}
```

**Подвох: приоритет операторов.** `(p(xs) && ...)` — правая свёртка, каждый элемент
вычисляется слева направо с коротким замыканием. Если предикат имеет побочные
эффекты, порядок важен. `&&`/`||` гарантируют порядок вычисления в fold; `,`
тоже гарантирует (слева направо), а вот `+` — нет.

## Идея 9. Почему ошибки шаблонов длинные — и как их читать

Шаблонные ошибки пугают объёмом, потому что компилятор печатает всю цепочку
инстанцирования и полные имена типов (`std::__cxx11::basic_string<char, ...>`). Тактика:

- читай **снизу вверх** или ищи первую строку со словами `required from here` —
  это место в *твоём* коде, откуда всё началось;
- ключевая фраза обычно одна: `no matching function`, `no type named 'type'`
  (привет, SFINAE/enable_if), `incomplete type`, `ambiguous`;
- мысленно подставь конкретный тип вместо `T` — часто сразу видно, какая операция для
  него не определена;
- в C++20 многие из этих простыней заменяются короткими сообщениями concepts (модуль
  07) — но SFINAE из старого кода ты ещё встретишь, так что читать их надо уметь.

---

## Задания (пиши прямо в `include/task16.hpp`)

1. **Variadic + fold.** `template<class... Ts> auto sum(Ts... args)` — сумма всех
   аргументов через fold-выражение. И `to_string_all(args...)` — склейка
   `std::to_string` каждого аргумента через `", "` (для `to_string_all(7, 8)` →
   `"7, 8"`).
2. **Свои трейты.** Реализуй через частичные специализации: `my_remove_const<T>`
   (+ алиас `my_remove_const_t`), `my_is_pointer<T>` (+ `my_is_pointer_v`),
   `my_is_same<A,B>` (+ `my_is_same_v`). Стандартными `<type_traits>`-аналогами
   пользоваться нельзя — в этом весь смысл.
3. **enable_if-перегрузка.** Допиши две версии `describe(T)`: для целочисленных типов
   возвращает `"integer"`, для типов с плавающей точкой — `"floating"`. Различай их
   через `std::enable_if`.
4. **CRTP `Comparable<Derived>`.** Реализуй в миксине `operator!=`, `>`, `<=`, `>=`,
   опираясь только на `operator<` и `operator==` производного класса (тест объявляет
   класс `Money`).
5. **Tag dispatch.** Реализуй `distance_dispatch(begin, end)` — число шагов от `begin`
   до `end`. Для итераторов случайного доступа — быстрый путь `end - begin` (и
   `++adv::fast_path_calls()`), для остальных — цикл со счётчиком шагов. Выбор —
   через тег категории итератора.
6. **all_of fold.** Реализуй `all_of_pred(Pred p, Ts... xs) -> bool` — возвращает
   `true`, если предикат `p` истинен для **всех** аргументов (или пакет пуст).
   Используй fold-выражение по `&&`. Опционально: перепиши `describe()` изнутри через
   `if constexpr`, оставив внешний интерфейс прежним.

```bash
./cpp/run.sh 16
```

### Подсказки

<details><summary>Задание 1 — sum и to_string_all</summary>
Для <code>sum</code> хватит унарной правой свёртки: <code>(args + ...)</code>. Для
<code>to_string_all</code> начни со строки из первого преобразования, а остальные
приклеивай: пригодится запятая-свёртка <code>(( ... ), ...)</code>, чтобы для каждого
аргумента дописать <code>", " + std::to_string(x)</code>. Подумай, как не поставить
лишнюю запятую в начале (подсказка: первый элемент — особый, или используй флаг
«это первый»).
</details>

<details><summary>Задание 1 — куда деть запятую</summary>
Один приём: объяви <code>std::string out;</code> и <code>bool first = true;</code>,
затем запятая-свёрткой для каждого аргумента сделай: если не первый — добавь
<code>", "</code>, потом добавь <code>std::to_string(arg)</code>, и сними флаг.
Тело лямбды/выражения один и тот же для всех элементов — это и разворачивает свёртка.
</details>

<details><summary>Задание 2 — частичные специализации</summary>
Поменяй значение в первичном шаблоне на правильное «по умолчанию» (для
<code>remove_const</code> это <code>using type = T;</code>, для предикатов —
<code>false</code> / разные типы). Затем <b>добавь</b> рядом специализацию, ловящую
нужную форму: <code>template&lt;class T&gt; struct my_is_pointer&lt;T*&gt; { ... };</code>,
<code>my_remove_const&lt;const T&gt;</code>, <code>my_is_same&lt;T, T&gt;</code>.
</details>

<details><summary>Задание 3 — две взаимоисключающие ветки</summary>
В заготовке обе перегрузки уже объявлены с разными <code>enable_if</code>-условиями
(<code>is_integral</code> и <code>is_floating_point</code>) — тебе нужно лишь вписать
правильные <code>return</code>. Если попробуешь <code>describe(3.14)</code> и обе ветки
вдруг подойдут — проверь, что условия и впрямь взаимоисключающие.
</details>

<details><summary>Задание 4 — выразить через базовые операции</summary>
Всё сводится к <code>&lt;</code> и <code>==</code> производного класса:
<code>a != b</code> это <code>!(a == b)</code>; <code>a &gt; b</code> это
<code>b &lt; a</code>; <code>a &lt;= b</code> это <code>!(b &lt; a)</code>;
<code>a &gt;= b</code> это <code>!(a &lt; b)</code>. Операторы объявлены
<code>friend</code> и принимают <code>const Derived&amp;</code> — внутри просто
используй <code>&lt;</code>/<code>==</code> аргументов.
</details>

<details><summary>Задание 5 — получить тег и выбрать путь</summary>
Категорию итератора берут так:
<code>typename std::iterator_traits&lt;It&gt;::iterator_category{}</code> — это объект
тега, его и передавай третьим аргументом в <code>distance_impl</code>. Перегрузки
<code>distance_impl</code> уже разнесены по <code>input_iterator_tag</code> и
<code>random_access_iterator_tag</code> (random-access наследуется от input, поэтому
для вектора выберется более точная — быстрая — версия). В быстрой не забудь
<code>++fast_path_calls();</code>.
</details>

<details><summary>Задание 5 — почему вектор берёт быстрый путь, а list — медленный</summary>
Категория итератора <code>std::vector</code> — <code>random_access_iterator_tag</code>,
она наследник <code>input_iterator_tag</code>, поэтому при выборе перегрузки побеждает
более специфичная (random-access). У <code>std::list</code> категория —
<code>bidirectional_iterator_tag</code>, она тоже наследник input, но <b>не</b>
random-access, так что подходит только обычная перегрузка. Это и есть tag dispatch
через иерархию тегов.
</details>

<details><summary>Задание 6 — all_of_pred: структура сигнатуры</summary>
Предикат <code>p</code> — произвольный вызываемый объект (лямбда, функтор).
Пакет <code>Ts... xs</code> — аргументы разных типов. Нужная свёртка:
<code>return (p(xs) && ...);</code> — правая свёртка по <code>&&</code>.
Пустой пакет: свёртка <code>(... && true)</code> — результат <code>true</code>,
что совпадает с математическим vacuous truth.
</details>

<details><summary>Задание 6 — if constexpr версия describe</summary>
Вместо двух перегрузок с <code>enable_if</code> — одна шаблонная функция:
<code>if constexpr (std::is_integral_v&lt;T&gt;) return "integer";</code>
<code>else if constexpr (std::is_floating_point_v&lt;T&gt;) return "floating";</code>
Внешний API не меняется — тесты задания 3 пройдут без изменений.
</details>

Когда увидишь зелёный прогон — попробуй намеренно сломать одну специализацию трейта и
посмотри, как меняется сообщение компилятора: это тренировка чтения шаблонных ошибок из
Идеи 9. Покажешь — разберём вместе.
