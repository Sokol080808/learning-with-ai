#!/usr/bin/env bash
# Единая команда курса Python: прогнать тесты.
#
#   ./python/run.sh        — прогнать ВСЕ тесты всех модулей
#   ./python/run.sh 00     — прогнать только тесты модуля 00
#   ./python/run.sh 07     — ... только модуля 07
#   ./python/run.sh clean  — удалить виртуальное окружение и кэши
#
# Первый запуск дольше: скрипт один раз создаёт .venv и ставит pytest. Нужен интернет один раз.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV="$HERE/.venv"

if [ "${1:-all}" = "clean" ]; then
  rm -rf "$VENV" "$HERE/.pytest_cache"
  find "$HERE" -type d -name '__pycache__' -prune -exec rm -rf {} + 2>/dev/null || true
  echo "Окружение и кэши удалены."
  exit 0
fi

# --- .venv + pytest (создаётся один раз) ---
if [ ! -x "$VENV/bin/pytest" ]; then
  echo "==> Создаю виртуальное окружение и ставлю pytest (один раз)…"
  python3 -m venv "$VENV"
  "$VENV/bin/python" -m pip install -q --upgrade pip pytest
fi
PYTEST="$VENV/bin/pytest"

ARG="${1:-all}"
echo "==> Тесты (${ARG})…"
if [ "$ARG" = "all" ]; then
  "$PYTEST" "$HERE"
else
  target=$(ls -d "$HERE/modules/${ARG}-"* 2>/dev/null | head -1 || true)
  if [ -z "$target" ]; then
    echo "Модуль '$ARG' не найден в $HERE/modules/"; exit 1
  fi
  "$PYTEST" "$target"
fi
