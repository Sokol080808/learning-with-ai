# TODO — дорожная карта курсов и эталонные интернет-источники

Назначение: что делать дальше с курсами и **на какие внешние курсы/книги ориентироваться**
при создании и улучшении модулей. Покрытие тем в каждом треке сверяем с эталонным курсом
ниже — не выдумываем программу с нуля, а калибруем по сильным открытым источникам (весь
текст пишем сами, не копируем).

Провенанс: подобрано deep-research-прогоном (5 углов = 5 треков, 25 источников; см. ссылки).
Часть фактов прошла adversarial-верификацию, часть — общеизвестные стабильные ресурсы.

## Статус треков (на 2026-06-20)

- **На баре (4/5):** Python, C++, Rust, Deep Learning — эталоны на `reference`, `reference-green`
  зелёный, property-тесты кусают, теория seminar-глубины, существенная задача в каждом модуле.
- **НЕ доведён — следующий большой кусок:** **CAOS** (нет эталонов, нет CAOS-job в reference-green,
  рандом-тестов почти нет, теория тоньше). Приоритет №1 в работе.

## Как пользоваться этим файлом

1. Улучшаешь/добавляешь модуль → открой раздел трека → сверь покрытие тем с **эталоном-бенчмарком**
   и со списком «темы, которые каноничные курсы покрывают». Чего нет у нас — кандидат на новый
   модуль или на углубление.
2. Конвейер доведения трека до бара (как делали для Rust и DL):
   эталоны на `reference` → job в `reference-green.yml` → рандом/property-тесты (red-on-stub/green-on-ref) →
   углубление теории + существенные задачи по аудиту. Генерящие флоты — на Sonnet, самое сложное — Opus,
   центральная проверка обязательна.

---

## Трек C++ (`cpp/`) — на баре (advanced, 23 модуля)

**Эталон-бенчмарк:** [learncpp.com](https://www.learncpp.com/) (полный путь от нуля до advanced,
бесплатно, 28+ глав) + [«A Tour of C++» 3rd, Stroustrup](https://www.stroustrup.com/tour3.html)
(сжатый обзор современного C++20/23 от автора языка).

**Ключевые ресурсы:**
- [learncpp.com](https://www.learncpp.com/) — лучшая бесплатная программа; move-семантика и умные
  указатели (гл. 22), value categories (гл. 12), шаблоны (гл. 11/13/26), constexpr (гл. 5/14/F),
  контейнеры (гл. 16-17), исключения/noexcept (гл. 27).
- [Federico Busato, «Modern C++ Programming» (Univ. Verona)](https://github.com/federico-busato/Modern-CPP-Programming)
  — 29 лекций, 2000+ слайдов, C++03→C++26; отдельная глава на move/value categories/perfect forwarding/
  copy elision; SFINAE + variadic + C++20 concepts.
- [cppreference](https://en.cppreference.com/w/cpp) — справочник стандарта (C++11→26), не учебник.
- CppCon (YouTube), Jason Turner (C++ Weekly) — доклады по конкретным темам.

**Что сверить / кандидаты на доработку:** ranges глубже (модуль 17 был самым тонким по аудиту);
`std::expected` и современная обработка ошибок (C++23); const-correctness и perfect forwarding как
сквозные темы; (вне junior, но для полноты) корутины, модули C++20.

---

## Трек Python (`python/`) — на баре (15 модулей)

**Эталон-бенчмарк:** [«Fluent Python» 2nd, Ramalho](https://www.oreilly.com/library/view/fluent-python-2nd/9781492056348/)
— даёт каркас тем junior-уровня (data model, dataclasses, typing, decorators/closures, итераторы/
генераторы, context managers) пятью частями (Data Structures / Functions as Objects / Classes &
Protocols / Control Flow / Metaprogramming).

**Ключевые ресурсы:**
- [Fluent Python 2nd](https://www.oreilly.com/library/view/fluent-python-2nd/9781492056348/) — тематический спайн.
- [CS50P (Harvard)](https://cs50.harvard.edu/python/) — бесплатно; 10 недель: функции, условия, циклы,
  исключения, библиотеки, **юнит-тесты**, file I/O, **регулярки**, ООП.
- [Официальный туториал python.org](https://docs.python.org/3/tutorial/index.html) — авторитетная база
  (но не для абсолютных новичков и НЕ покрывает декораторы/typing/dataclasses/тесты — их добавляем сами).
- Real Python, Corey Schafer (YouTube) — точечные объяснения.

**Что сверить / кандидаты на новые модули (Python):** **регулярные выражения** (есть у CS50P, у нас
отдельного модуля нет); **async/asyncio + concurrency** (нет в курсе); **метапрограммирование вглубь**
(Fluent Python Part V: дескрипторы, метаклассы, `__init_subclass__`); упаковка/packaging глубже.

---

## Трек Rust (`rust/`) — на баре (15 модулей)

**Эталон-бенчмарк:** [The Rust Book (doc.rust-lang.org/book)](https://doc.rust-lang.org/book/) —
официальный, бесплатный; ownership/borrowing/lifetimes, traits/generics, smart pointers, error
handling (`Result`/`?`, без исключений), concurrency (потоки, каналы, `Send`/`Sync`).

**Ключевые ресурсы:**
- [The Rust Book](https://doc.rust-lang.org/book/) — канонический спайн.
- [Rustlings](https://github.com/rust-lang/rustlings) — официальные упражнения параллельно книге
  (ownership, structs, enums, error handling, generics, traits, lifetimes, iterators, smart pointers, threads).
- [Rust by Example](https://doc.rust-lang.org/rust-by-example/) — примеры по темам.
- [Comprehensive Rust (Google)](https://google.github.io/comprehensive-rust/) — интенсив от команды Android.
- [100 Exercises to Learn Rust](https://rust-exercises.com/100-exercises/) — упражнения.

**Что сверить / кандидаты:** **модуль конкуренции** (потоки/каналы/`Send`+`Sync` — The Rust Book гл.16;
у нас тема лёгкая); организация кода (модули/крейты) глубже; (advanced) `async`/await.

---

## Трек Deep Learning (`deep-learning/`) — на баре (18 модулей)

**Эталон-бенчмарк:** [Karpathy «Neural Networks: Zero to Hero»](https://karpathy.ai/zero-to-hero.html)
— строим всё с нуля: micrograd (≈ наш autograd-движок), makemore (≈ наши seq-models), nanoGPT
(≈ наш капстоун nano-LM). Идеальная калибровка для «from scratch».

**Ключевые ресурсы:**
- [Karpathy Zero-to-Hero](https://karpathy.ai/zero-to-hero.html) — backprop/autograd/языковые модели с нуля.
- [fast.ai (Practical Deep Learning)](https://course.fast.ai/) — top-down практический курс.
- [d2l.ai (Dive into Deep Learning)](https://d2l.ai/) — интерактивный учебник, numpy+PyTorch, очень широкий.
- [Stanford CS231n](https://cs231n.github.io/) — CNN/зрение, классические заметки.
- PyTorch tutorials (pytorch.org) — официальные.

**Что сверить / кандидаты:** аугментация данных и transfer learning; современные приёмы тренировки
(LR-расписания, mixed precision, регуляризация глубже); метрики/оценка; внимание/трансформер можно
расширить ближе к nanoGPT. (Трек уже самый полный — это шлифовка, не пробелы.)

---

## Трек CAOS (`caos/`) — ⚠️ НЕ на баре, приоритет №1

C11 + CMake + GoogleTest (C-стабы, тесты C++ через `extern "C"`, pthreads). 15 модулей + капстоун
Tiny VM. Сейчас: нет эталонов на `reference`, нет CAOS-job в `reference-green`, рандом-тестов почти нет,
теория тоньше (avg 3.5), нет существенной задачи в 00/04/07.

**Эталон-бенчмарк:** [CS:APP (CMU 15-213)](https://csapp.cs.cmu.edu/) — «Computer Systems: A Programmer's
Perspective». Прямо ложится на наши модули (биты/целые/IEEE-754/память/кэш/ассемблер/аллокаторы).

**Ключевые ресурсы:**
- [CS:APP + CMU 15-213](https://www.cs.cmu.edu/~213/schedule.html) — архитектура с точки зрения программиста;
  знаменитые лабы (Data Lab, Bomb, Attack, Cache, Malloc, Shell, Proxy) — образец «существенных задач».
- [OSTEP (Wisconsin)](https://pages.cs.wisc.edu/~remzi/OSTEP/) — бесплатный учебник по ОС: процессы,
  планирование, виртуальная память, конкуренция (локи/CV/семафоры) — наши модули 10–14.
- [MIT 6.1810 / xv6](https://pdos.csail.mit.edu/6.1810/2025/) — реальная маленькая ОS на C (лабы).
- [Berkeley CS61C](https://cs61c.org/) — Great Ideas in Computer Architecture.
- nand2tetris — сборка машины снизу вверх (для интуиции, опционально).

**План доведения CAOS до бара (тот же конвейер):**
1. Эталонные решения всех 15 модулей + капстоун Tiny VM на `reference` (Sonnet-флот, самое сложное —
   ассемблер/аллокаторы/синхронизация — на Opus; центральная сборка+ctest).
2. Добавить CAOS-job в `reference-green.yml` (cmake + ctest, как cpp).
3. Рандом/property-тесты (C через GoogleTest, seeded `std::mt19937`), red-on-stub/green-on-ref, PR в main.
4. Аудит → углубить теорию (многие модули на 3/5) + существенные задачи в 00/04/07.
5. **Темы по CS:APP/OSTEP, которые стоит проверить на покрытие:** линковка/загрузка, динамический
   аллокатор как лаба (наш модуль 09), кэш-лаба, IEEE-754 округление и краевые случаи, семафоры/
   condition variables, виртуальная память и трансляция адресов.

---

## После того как все 5 треков на баре

- Финальная сводка доков (корневой README + per-course PROGRESS/NEXT_STEPS) + **один** прогон
  `graphify update .` на `main` (граф устарел относительно нового материала Rust/DL).
- Капстоуны вне `reference-green` (cpp MiniDB, rust minidb) — дописать эталон+property и включить в CI.
