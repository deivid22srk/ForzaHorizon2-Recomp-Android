#!/usr/bin/env bash
# run_recomp.sh — pipeline de recompilação completo:
#   1. XenonAnalyse no XEX descriptografado → TOML de jump tables
#   2. Busca de funções save/restore (byte patterns documentados)
#   3. XenonRecomp (TOML + ppc_context.h) → recomp/generated/*.cpp
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
XEX="${1:-$ROOT/recomp/private/default_dec.xex}"
GEN="$ROOT/recomp/generated"
CONFIG="$ROOT/recomp/config"

[[ -f "$XEX" ]] || { echo "ERRO: $XEX não encontrado (rode tools/fetch_xex.sh)" >&2; exit 1; }

mkdir -p "$GEN" "$CONFIG"

echo "== 1/3: XenonAnalyse (jump tables) =="
"$ROOT/tools/build/XenonAnalyse" "$XEX" "$CONFIG/FH2_switch_tables.toml" || \
    echo "AVISO: XenonAnalyse terminou com código $? (jump tables podem estar incompletas — backlog)"

echo "== 2/3: save/restore functions =="
python3 "$ROOT/tools/find_save_restore.py" "$XEX" > "$CONFIG/save_restore.txt" || true
cat "$CONFIG/save_restore.txt"

echo "== 3/3: XenonRecomp =="
"$ROOT/tools/build/XenonRecomp" "$CONFIG/FH2.toml" "$ROOT/recomp/runtime/ppc_context.h"

echo "OK: código gerado em $GEN/ ($(ls "$GEN" | wc -l) arquivos)"
