#!/usr/bin/env bash
# fetch_xex.sh — baixa o default.xex do repositório privado (via GH_PAT)
# e o descriptografa para análise/recompilação.
#
# Uso: GH_PAT=ghp_xxx ./tools/fetch_xex.sh
# No CI: token vem de secrets.GH_PAT (nunca hardcoded).
set -euo pipefail

REPO="deivid22srk/forza-horizon-2-xex"
OUT_DIR="${1:-recomp/private}"
XEX="$OUT_DIR/default.xex"
DEC="$OUT_DIR/default_dec.xex"

if [[ -z "${GH_PAT:-}" ]]; then
    echo "ERRO: GH_PAT não definido. Exporte GH_PAT=<token com acesso ao repo privado>." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

echo "== Baixando default.xex (API de Contents) =="
HTTP=$(curl -sS -L -H "Authorization: token $GH_PAT" \
    -H "Accept: application/vnd.github.v3.raw" \
    -o "$XEX" -w "%{http_code}" \
    "https://api.github.com/repos/$REPO/contents/default.xex")
if [[ "$HTTP" != "200" ]]; then
    echo "ERRO: download HTTP $HTTP — verifique o token/permissões" >&2
    exit 1
fi

echo "== Validação =="
SIZE=$(stat -c%s "$XEX")
if [[ "$SIZE" -le 0 ]]; then
    echo "ERRO: default.xex vazio" >&2
    exit 1
fi
MAGIC=$(head -c 4 "$XEX")
if [[ "$MAGIC" != "XEX2" ]]; then
    echo "ERRO: magic inválido '$MAGIC' (esperado XEX2)" >&2
    exit 1
fi
echo "OK: $XEX ($SIZE bytes, magic XEX2)"

echo "== Descriptografia (XexTool, se compilado) =="
XEXTOOL="tools/build/XexTool"
if [[ -x "$XEXTOOL" ]]; then
    "$XEXTOOL" -e u -o "$DEC" "$XEX"
    echo "OK: $DEC (retail unencrypted)"
else
    echo "AVISO: XexTool não encontrado em $XEXTOOL — rode tools/build_recomp_tools.sh"
    echo "       A recompilação exige o XEX descriptografado."
fi
