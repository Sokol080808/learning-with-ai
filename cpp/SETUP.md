# Что установить перед началом курса C++

Курсу нужно немного: компилятор с поддержкой **C++20**, **CMake** и `git`. GoogleTest
скачивать вручную не надо — CMake подтянет его сам при первой сборке (нужен интернет один раз).

## Минимум

| Инструмент | Версия | Зачем |
|---|---|---|
| Компилятор C++20 | **g++ ≥ 11** или **clang++ ≥ 14** | собирать твой код |
| **CMake** | **≥ 3.20** | система сборки курса |
| git | любая свежая | версионировать свой прогресс |
| интернет | один раз | CMake скачает GoogleTest при первом запуске |

Опционально (пригодится в модуле 12 «Инструменты»): `gdb` или `lldb` (отладчик),
`clang-format`, `clang-tidy`, санитайзеры (идут с компилятором).

## Установка по ОС

### macOS
```bash
# Компилятор: либо Apple Clang из Command Line Tools…
xcode-select --install
# …либо свежий g++/clang через Homebrew:
brew install cmake llvm     # llvm даёт современный clang++ и clang-tidy/clang-format
# (g++ при желании: brew install gcc)
```
Apple Clang из CLT обычно уже поддерживает C++20 — этого достаточно.

### Linux (Debian/Ubuntu)
```bash
sudo apt update
sudo apt install -y build-essential g++ cmake git gdb clang-format clang-tidy
```
Проверь, что `g++ --version` ≥ 11. Если нет — поставь новее (`sudo apt install g++-12`).

### Windows
- Поставь **Visual Studio 2022** (Community) с компонентом «Разработка на C++» — это даёт
  MSVC + CMake, и всё работает из коробки.
- Либо **MSYS2/MinGW-w64** (g++) + **CMake** с сайта cmake.org.
- Запускать `./cpp/run.sh` удобнее всего в **WSL** или Git Bash.

## Проверка

```bash
g++ --version      # или: clang++ --version  → должен поддерживать C++20
cmake --version    # ≥ 3.20
git --version
```

Готово? Тогда первый прогон (соберёт всё и один раз скачает GoogleTest):
```bash
./cpp/run.sh 00
```
Почти все тесты будут **красными** — это нормально, это твои задания. Дальше — по
[`README.md`](README.md), начиная с [`modules/00-setup/README.md`](modules/00-setup/README.md).

> Не ставится компилятор или CMake? Покажи мне ошибку — разберёмся вместе.
