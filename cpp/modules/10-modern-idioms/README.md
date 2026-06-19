# Модуль 10 — Современные идиомы C++

Этот модuль — про то, как пишут на C++ *сегодня*. Те же задачи, но короче, безопаснее и
выразительнее. Большая идея: современные возможности убирают шум и переносят больше
проверок на этап компиляции.

## Идея 1. `auto` и его границы

`auto` выводит тип из инициализатора — меньше дублирования, особенно для громоздких
типов итераторов:
```cpp
auto it = m.begin();   // вместо std::map<std::string,int>::iterator
```
Но не злоупотребляй: `auto x = foo();` хорош, когда тип очевиден из контекста или не
важен. Если читателю важно видеть тип — пиши его явно.

## Идея 2. Structured bindings — разложить на части

```cpp
std::pair<int, int> p = minmax(v);
auto [lo, hi] = p;             // вместо p.first / p.second

std::map<std::string, int> m;
for (const auto& [name, count] : m) {   // сразу ключ и значение
    std::cout << name << " = " << count << "\n";
}
```
Читается куда яснее, чем `.first`/`.second` или `it->second`.

## Идея 3. `enum class` — типобезопасные перечисления

```cpp
enum class Color { Red, Green, Blue };

Color c = Color::Red;          // обязательно с префиксом Color::
// int x = c;                  // ошибка — не приводится неявно к int (это хорошо!)
int x = static_cast<int>(c);   // если очень надо — явно
```
В отличие от старого `enum`, `enum class` не «протекает» именами в окружающую область и
не превращается случайно в int. Всегда предпочитай `enum class`.

## Идея 4. `constexpr` — вычисления на этапе компиляции

```cpp
constexpr int square(int n) { return n * n; }

constexpr int k = square(5);          // посчитано компилятором: 25
static_assert(square(4) == 16);       // проверка на этапе компиляции
int runtime = square(x);              // та же функция работает и в рантайме
```
`constexpr`-функцию можно вызвать и в компайл-тайме (если аргументы известны), и в
рантайме. Это бесплатная производительность и более сильные гарантии.

## Идея 5. Ranges (C++20) — конвейеры над данными

```cpp
#include <ranges>
namespace rv = std::views;

auto result = v
    | rv::filter([](int x){ return x % 2 == 0; })   // оставить чётные
    | rv::transform([](int x){ return x * x; });     // возвести в квадрат
// result — «ленивое» представление; материализуем в вектор:
std::vector<int> out(result.begin(), result.end());
```
Это тот же `filter`/`transform`, что в модуле 08, но как читаемый конвейер `|`. Views
**ленивые**: вычисляются при обходе, без промежуточных копий.

---

## Задания

Файл `src/task10.cpp` (интерфейс — `include/task10.hpp`). `square_ce` уже определена в
заголовке как `constexpr` — разберись, почему она там, а не в `.cpp` (спроси, если надо).

1. `std::pair<int,int> minmax_of(const std::vector<int>& v)` — пара `{минимум, максимум}`
   (вектор непустой). Внутри попробуй structured bindings.
2. `enum class Color { Red, Green, Blue }` (объявлен в заголовке) и
   `std::string color_name(Color c)` — `"Red"`, `"Green"`, `"Blue"`.
3. `constexpr int square_ce(int n)` — квадрат, **на этапе компиляции** (определена в
   заголовке; тест проверит `static_assert`). Реализуй её там.
4. `std::vector<int> evens_squared(const std::vector<int>& v)` — оставить чётные и
   возвести их в квадрат. Сделай через `std::views::filter | std::views::transform`.
5. `int sum_values(const std::map<std::string,int>& m)` — сумма всех значений; обходи
   через `for (const auto& [k, val] : m)`.

```bash
./cpp/run.sh 10
```

### Подсказки

<details><summary>Задание 3 — почему constexpr в заголовке</summary>
Чтобы вычислить функцию на этапе компиляции в другом файле, компилятор должен видеть
её ТЕЛО. Как и шаблоны (модуль 07), `constexpr`-функции обычно живут в заголовках.
</details>
<details><summary>Задание 4 — материализация view</summary>
`std::views::filter(...) | std::views::transform(...)` даёт ленивое представление.
Чтобы вернуть `std::vector<int>`, пройди по нему и собери элементы (например, циклом
`for (int x : view) out.push_back(x);`).
</details>

Сравни свой `evens_squared` с тем, как ту же задачу решали в модуле 08 алгоритмами.
Какой вариант читается лучше? Это хороший повод для разговора о вкусе в коде.
