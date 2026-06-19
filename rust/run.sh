#!/usr/bin/env bash
# Единая команда курса Rust: собрать и прогнать тесты через cargo.
#
#   ./rust/run.sh        — прогнать ВСЕ тесты воркспейса (все модули)
#   ./rust/run.sh 00     — прогнать тесты только модуля 00
#   ./rust/run.sh 07     — ... только модуля 07
#   ./rust/run.sh clean  — cargo clean (удалить target/)
#
# Нужен установленный Rust (rustc + cargo). Первая сборка дольше: cargo скачает индекс/зависимости.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$HERE"

if [ "${1:-all}" = "clean" ]; then
  cargo clean
  echo "target/ удалён."
  exit 0
fi

ARG="${1:-all}"
echo "==> Тесты (${ARG})…"
if [ "$ARG" = "all" ]; then
  cargo test
else
  member=$(ls -d "$HERE/modules/${ARG}-"* 2>/dev/null | head -1 || true)
  if [ -z "$member" ]; then
    echo "Модуль '$ARG' не найден в $HERE/modules/"; exit 1
  fi
  # имя пакета из его Cargo.toml
  pkg=$(sed -n 's/^name *= *"\(.*\)"/\1/p' "$member/Cargo.toml" | head -1)
  cargo test -p "$pkg"
fi
