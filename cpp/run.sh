#!/usr/bin/env bash
# Единая команда курса: собрать всё и прогнать тесты.
#
#   ./cpp/run.sh        — собрать и прогнать ВСЕ тесты всех модулей
#   ./cpp/run.sh 00     — собрать всё и прогнать только тесты модуля 00
#   ./cpp/run.sh 07     — ... только модуля 07
#   ./cpp/run.sh clean  — удалить каталог сборки
#
# Первый запуск дольше: CMake скачает GoogleTest. Нужен интернет один раз.
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
# Число параллельных задач: nproc (Linux) → sysctl (macOS) → запасные 2.
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
cmake --build "$BUILD" -j"$JOBS"

ARG="${1:-all}"
echo "==> Тесты (${ARG})…"
if [ "$ARG" = "all" ]; then
  ctest --test-dir "$BUILD" --output-on-failure
else
  ctest --test-dir "$BUILD" --output-on-failure -L "$ARG"
fi
