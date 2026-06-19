#!/usr/bin/env bash
# Единая команда курса CAOS: собрать всё и прогнать тесты.
#
#   ./caos/run.sh        — собрать и прогнать ВСЕ тесты
#   ./caos/run.sh 00     — собрать всё и прогнать только тесты модуля 00
#   ./caos/run.sh 11     — ... только модуля 11
#   ./caos/run.sh clean  — удалить каталог сборки
#
# Нужен компилятор C/C++ (gcc/clang) и CMake. Первый запуск дольше: CMake скачает GoogleTest.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$HERE/build"

if [ "${1:-all}" = "clean" ]; then
  rm -rf "$BUILD"
  echo "Каталог сборки удалён."
  exit 0
fi

echo "==> Конфигурация (CMake)…"
cmake -S "$HERE" -B "$BUILD" >/dev/null

echo "==> Сборка…"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
cmake --build "$BUILD" -j"$JOBS"

ARG="${1:-all}"
echo "==> Тесты (${ARG})…"
if [ "$ARG" = "all" ]; then
  ctest --test-dir "$BUILD" --output-on-failure
else
  ctest --test-dir "$BUILD" --output-on-failure -L "$ARG"
fi
