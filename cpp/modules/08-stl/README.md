# Модуль 08 — STL: контейнеры, итераторы, алгоритмы, лямбды

STL (Standard Template Library) — это огромная библиотека готовых контейнеров и
алгоритмов. Большая идея: **не пиши циклы руками там, где есть алгоритм**. Код через
алгоритмы короче, выразительнее и труднее сломать. Junior обязан свободно владеть STL.

## Идея 1. Контейнеры — выбирай по задаче

- `std::vector<T>` — динамический массив. Твой выбор по умолчанию. Быстрый доступ по
  индексу, быстрый `push_back` в конец.
- `std::array<T, N>` — массив фиксированного размера на стеке.
- `std::map<K, V>` — отсортированный словарь (ключ→значение), `O(log n)`.
- `std::unordered_map<K, V>` — хеш-словарь, в среднем `O(1)`.
- `std::set` / `std::unordered_set` — множества уникальных значений.
- `std::deque`, `std::list` — реже, под особые нужды.

```cpp
std::vector<int> v{1, 2, 3};
v.push_back(4);
std::map<std::string, int> ages;
ages["Anna"] = 30;        // вставка/обновление
if (ages.contains("Anna")) { /* C++20 */ }
```

## Идея 2. Итераторы — общий «язык» обхода

Итератор — обобщение указателя: умеет `*it` (значение), `++it` (следующий). Любой
контейнер даёт `begin()` (первый) и `end()` (за-последним). Алгоритмы работают с
парой `[begin, end)`, поэтому один алгоритм подходит к любому контейнеру.

```cpp
for (auto it = v.begin(); it != v.end(); ++it) std::cout << *it;
for (int x : v) std::cout << x;   // range-based for — то же самое, короче
```

## Идея 3. Лямбды — функция «на месте»

```cpp
auto is_even = [](int x) { return x % 2 == 0; };
is_even(4);  // true

int threshold = 10;
auto big = [threshold](int x) { return x > threshold; };  // [threshold] — захват
```
`[...]` — список захвата (что из окружения видно внутри). `[=]` — захват всего по
значению, `[&]` — по ссылке. Лямбды — топливо для алгоритмов.

## Идея 4. Алгоритмы `<algorithm>` / `<numeric>`

```cpp
#include <algorithm>
#include <numeric>

std::sort(v.begin(), v.end());                          // сортировка
std::sort(v.begin(), v.end(), [](int a, int b){ return a > b; }); // по убыванию
int n  = std::count_if(v.begin(), v.end(), is_even);    // сколько подходит
auto it = std::find(v.begin(), v.end(), 42);            // поиск (it==end() если нет)
int sum = std::accumulate(v.begin(), v.end(), 0);       // сумма
bool ok = std::all_of(v.begin(), v.end(), is_even);     // все ли подходят
std::transform(v.begin(), v.end(), out.begin(), f);     // применить f к каждому
std::copy_if(v.begin(), v.end(), std::back_inserter(out), pred); // фильтр
```
`std::back_inserter(out)` — итератор, который на каждое присваивание делает
`out.push_back(...)`. Удобно «накапливать» результат.

---

## Задания

Файл `src/task08.cpp` (интерфейс — `include/task08.hpp`). Старайся использовать
алгоритмы, а не ручные циклы:

1. `std::vector<int> evens(const std::vector<int>& v)` — только чётные (`copy_if`).
2. `int count_greater(const std::vector<int>& v, int threshold)` — сколько больше
   порога (`count_if` + лямбда с захватом).
3. `std::vector<int> squared(const std::vector<int>& v)` — каждый элемент в квадрат
   (`transform`).
4. `std::vector<int> sorted_desc(std::vector<int> v)` — по убыванию (`sort` + лямбда).
   Заметь: `v` принимается **по значению** — можно сортировать копию и вернуть её.
5. `std::map<char,int> char_frequency(const std::string& s)` — сколько раз встречается
   каждый символ.

## Микро-проект: частотный словарь слов

В `project/` — ядро в `project/include/word_freq.hpp` / `project/src/word_freq.cpp`:

- `std::map<std::string,int> word_count(const std::string& text)` — разбить текст по
  пробелам/переводам строк на слова и посчитать, сколько раз встречается каждое.
  Слова сравниваются как есть (без приведения регистра).
- `std::vector<std::pair<std::string,int>> top_n(const std::map<std::string,int>& freq, int n)`
  — `n` самых частых слов, отсортированных по убыванию частоты; при равной частоте —
  по алфавиту (по возрастанию).

`project/src/main.cpp` — готовая программа: читает текст со stdin и печатает топ-слова.

```bash
./cpp/run.sh 08
echo "a b a c a b" | ./cpp/build/modules/08-stl/m08_wordfreq_app
```

### Подсказки

<details><summary>Разбиение текста на слова</summary>
Проще всего — через `std::istringstream iss(text);` и чтение `iss >> word` в цикле:
оператор `>>` для строки сам пропускает пробелы и переводы строк.
</details>
<details><summary>top_n — сортировка пар</summary>
Скопируй пары из map в vector, отсортируй компаратором: сначала сравни частоты (больше
— раньше), при равенстве — слова (меньше — раньше). Потом возьми первые n.
</details>

Покажи, как ты разбиваешь текст и как пишешь компаратор для `top_n` — там легко
ошибиться со знаком сравнения, разберём.
