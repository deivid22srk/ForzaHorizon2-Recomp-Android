#!/usr/bin/env bash
# run_recomp.sh — pipeline de recompilação completo (multi-módulo):
#   1. Descriptografa os XEXs secundários (XMediaFacade/SpeechFacade) via XexTool
#   2. XenonAnalyse no XEX do título + secundários → TOML de jump tables mesclado
#   3. Busca de funções save/restore (byte patterns documentados)
#   4. XenonRecomp (TOML + ppc_context.h) → recomp/generated/*.cpp
#      — o FH2.toml lista os secundários em extra_file_paths (quando presentes)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
XEX="${1:-$ROOT/recomp/private/default_dec.xex}"
PRIV="$ROOT/recomp/private"
GEN="$ROOT/recomp/generated"
CONFIG="$ROOT/recomp/config"

[[ -f "$XEX" ]] || { echo "ERRO: $XEX não encontrado (rode tools/fetch_xex.sh)" >&2; exit 1; }

mkdir -p "$GEN" "$CONFIG"

echo "== 1/4: descriptografia dos módulos secundários (XexTool) =="
EXTRA_ARGS=""
for MOD in XMediaFacade_default.xex SpeechFacade_default.xex; do
    SRC="$PRIV/$MOD"
    DEC="$PRIV/${MOD%.xex}_dec.xex"
    if [[ -f "$SRC" && -x "$ROOT/tools/build/XexTool" ]]; then
        "$ROOT/tools/build/XexTool" -e u -o "$DEC" "$SRC"
        echo "OK: $DEC"
    elif [[ -f "$DEC" ]]; then
        echo "OK: $DEC (já descriptografado)"
    else
        echo "::warning::$MOD indisponível — o título recompila SEM esse módulo (vídeo/voz degradados, sem crash)"
    fi
done

echo "== 2/4: XenonAnalyse (jump tables — título + secundários) =="
"$ROOT/tools/build/XenonAnalyse" "$XEX" "$CONFIG/FH2_switch_tables.toml" || \
    echo "AVISO: XenonAnalyse terminou com código $? (jump tables podem estar incompletas — backlog)"

# jump tables dos secundários: análise por módulo + mescla no TOML final
python3 - "$ROOT/tools/build/XenonAnalyse" "$CONFIG" "$PRIV" <<'PYEOF'
import subprocess, sys, os, re
analyser, config_dir, priv = sys.argv[1], sys.argv[2], sys.argv[3]
main_toml = os.path.join(config_dir, "FH2_switch_tables.toml")
for mod in ("XMediaFacade_default_dec.xex", "SpeechFacade_default_dec.xex"):
    src = os.path.join(priv, mod)
    if not os.path.isfile(src):
        continue
    part = os.path.join(config_dir, f"switch_{mod}.toml")
    r = subprocess.run([analyser, src, part])
    if r.returncode != 0 or not os.path.isfile(part):
        print(f"AVISO: análise de {mod} falhou (jump tables ausentes p/ esse módulo)")
        continue
    # mescla: concatena entradas [[switch]] dos dois TOMLs
    def entries(path):
        text = open(path).read() if os.path.isfile(path) else ""
        blocks, cur = [], []
        for line in text.splitlines():
            if line.strip().startswith("[["):
                if cur: blocks.append("\n".join(cur)); cur = []
            cur.append(line)
        if cur and any(l.strip() for l in cur): blocks.append("\n".join(cur))
        # mantém apenas o cabeçalho + blocos [[switch]]
        return [b for b in blocks if "[[switch]]" in b]
    merged = "# mesclado (título + módulos secundários) — regenerado pelo pipeline\n\n"
    merged += "\n\n".join(entries(main_toml) + entries(part))
    open(main_toml, "w").write(merged + "\n")
    os.remove(part)
    print(f"OK: jump tables de {mod} mescladas em {main_toml}")
PYEOF

echo "== 3/4: save/restore functions =="
python3 "$ROOT/tools/find_save_restore.py" "$XEX" > "$CONFIG/save_restore.txt" || true
cat "$CONFIG/save_restore.txt"

echo "== 4/4: XenonRecomp (título + módulos secundários) =="
# FH2.toml base (sem extras) + extra_file_paths dos módulos que EXISTEM —
# o TOML final é regenerado a cada run para refletir a disponibilidade real
python3 - "$CONFIG" "$PRIV" <<'PYEOF'
import os, sys
config_dir, priv = sys.argv[1], sys.argv[2]
src = os.path.join(config_dir, "FH2.toml")
text = open(src).read()
# remove bloco extra_file_paths anterior (regeneração idempotente)
lines = [l for l in text.splitlines()]
out, skip = [], False
for l in lines:
    if l.strip().startswith("extra_file_paths"):
        skip = "=" not in l.split("#")[0]  # continua até a linha que fecha o array "]"
        skip = True
        continue
    if skip:
        if "]" in l:
            skip = False
        continue
    out.append(l)
extras = []
for mod, dec in (("XMediaFacade", "XMediaFacade_default_dec.xex"),
                 ("SpeechFacade", "SpeechFacade_default_dec.xex")):
    if os.path.isfile(os.path.join(priv, dec)):
        # caminho RELATIVO ao FH2.toml (recomp/config/) — os decodificados
        # ficam em recomp/private/
        extras.append(f'"../private/{dec}"')
if extras:
    out.append("")
    out.append("# Módulos secundários recompilados junto com o título (video/XMV e voz/XMA);")
    out.append("# o runtime carrega esses módulos no boot e resolve as chamadas via tabelas extras.")
    out.append("extra_file_paths = [")
    out.append("    " + ",\n    ".join(extras) + ",")
    out.append("]")
open(src, "w").write("\n".join(out) + "\n")
print(f"extra_file_paths: {len(extras)} módulo(s)")
PYEOF

"$ROOT/tools/build/XenonRecomp" "$CONFIG/FH2.toml" "$ROOT/recomp/runtime/ppc_context.h"

echo "OK: código gerado em $GEN/ ($(ls "$GEN" | wc -l) arquivos)"
