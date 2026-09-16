#!/usr/bin/env bash
# fetch_xex.sh — baixa o default.xex do SEU repositório privado (via GH_PAT)
# e o descriptografa para análise/recompilação.
#
# Uso: GH_PAT=ghp_xxx ./tools/fetch_xex.sh
# No CI: token vem de secrets.GH_PAT (nunca hardcoded).
#
# O repositório de origem é configurável via FH2_XEX_REPO (env) — cada usuário
# deve apontar para o SEU repositório privado contendo o XEX de SUA cópia legal.
# O padrão abaixo é o repo privado do dono do projeto.
set -euo pipefail

REPO="${FH2_XEX_REPO:-deivid22srk/forza-horizon-2-xex}"
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

# XEXs SECUNDARIOS do disco (mesmo repositorio): XMediaFacade (video/XMV)
# e SpeechFacade (voz/XMA) — recompilados junto com o titulo pelo
# XenonRecomp multi-modulo; o runtime carrega e liga esses modulos em tempo
# de boot (XexLoadImage do launcher do FH2).
for MOD in XMediaFacade_default.xex SpeechFacade_default.xex; do
    MOD_OUT="$OUT_DIR/$MOD"
    echo "== Baixando $MOD =="
    HTTP=$(curl -sS -L -H "Authorization: token $GH_PAT" \
        -H "Accept: application/vnd.github.v3.raw" \
        -o "$MOD_OUT" -w "%{http_code}" \
        "https://api.github.com/repos/$REPO/contents/$MOD") || HTTP="curl-erro"
    if [[ "$HTTP" != "200" ]]; then
        echo "::warning::baixa de $MOD falhou (HTTP $HTTP) — o jogo segue sem ele (modulo ausente na runtime)"
        rm -f "$MOD_OUT"
        continue
    fi
    MAGIC=$(head -c 4 "$MOD_OUT")
    if [[ "$MAGIC" != "XEX2" ]]; then
        echo "::warning::$MOD com magic invalido — ignorado" >&2
        rm -f "$MOD_OUT"
        continue
    fi
    echo "OK: $MOD_OUT ($(stat -c%s "$MOD_OUT") bytes, magic XEX2)"
done

echo "== Descriptografia (XexTool, se compilado) =="
XEXTOOL="tools/build/XexTool"
if [[ -x "$XEXTOOL" ]]; then
    "$XEXTOOL" -e u -o "$DEC" "$XEX"
    echo "OK: $DEC (retail unencrypted)"
else
    echo "AVISO: XexTool não encontrado em $XEXTOOL — rode tools/build_recomp_tools.sh"
    echo "       A recompilação exige o XEX descriptografado."
fi
