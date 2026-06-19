#!/usr/bin/env bash
# Единая команда курса Deep Learning: прогнать тесты.
#
#   ./deep-learning/run.sh        — прогнать ВСЕ тесты всех модулей
#   ./deep-learning/run.sh 00     — только тесты модуля 00
#   ./deep-learning/run.sh 15     — ... только модуля 15
#   ./deep-learning/run.sh clean  — удалить виртуальное окружение и кэши
#
# Первый запуск ДОЛГИЙ: скрипт создаёт .venv и ставит numpy + torch + pytest.
# torch — большой пакет (сотни МБ), его скачивание займёт время. Нужен интернет один раз.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV="$HERE/.venv"

if [ "${1:-all}" = "clean" ]; then
  rm -rf "$VENV" "$HERE/.pytest_cache"
  find "$HERE" -type d -name '__pycache__' -prune -exec rm -rf {} + 2>/dev/null || true
  echo "Окружение и кэши удалены."
  exit 0
fi

# torch нужен Python 3.11–3.13 (на 3.14 колёс может ещё не быть) — выбираем совместимый.
PY=""
for c in python3.12 python3.13 python3.11 python3.10 python3; do
  if command -v "$c" >/dev/null 2>&1; then PY="$c"; break; fi
done
if [ -z "$PY" ]; then echo "Не найден python3"; exit 1; fi

if [ ! -x "$VENV/bin/pytest" ]; then
  echo "==> Создаю окружение ($PY) и ставлю numpy + torch + pytest (один раз, torch большой)…"
  "$PY" -m venv "$VENV"
  "$VENV/bin/python" -m pip install -q --upgrade pip
  "$VENV/bin/python" -m pip install -q numpy torch pytest
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
