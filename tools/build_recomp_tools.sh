#!/usr/bin/env bash
# build_recomp_tools.sh — clona e compila XexTool + XenonRecomp/XenonAnalyse
# (executado pelo CI e por quem quiser regerar o código localmente)
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TOOLS="$ROOT/tools/build"
mkdir -p "$TOOLS" "$ROOT/tools"

echo "== XexTool (Team-Resurgent) =="
if [[ ! -d "$ROOT/tools/XexTool" ]]; then
    git clone --depth 1 --recursive https://github.com/Team-Resurgent/XexTool.git "$ROOT/tools/XexTool"
fi
cmake -S "$ROOT/tools/XexTool" -B "$ROOT/tools/XexTool/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/tools/XexTool/build" -j"$(nproc)"
cp "$ROOT/tools/XexTool/build/XexTool" "$TOOLS/XexTool"

echo "== XenonRecomp (fork deivid22srk, branch fh2: +65 mnemônicos PPC implementados) =="
# Fork com os emitters completos (0 instruções não implementadas no FH2).
# Override via env FH2_XENONRECOMP_REPO/FH2_XENONRECOMP_BRANCH se necessário.
XENONRECOMP_REPO="${FH2_XENONRECOMP_REPO:-https://github.com/deivid22srk/XenonRecomp.git}"
XENONRECOMP_BRANCH="${FH2_XENONRECOMP_BRANCH:-fh2}"
if [[ ! -d "$ROOT/tools/XenonRecomp" ]]; then
    git clone --recursive --depth 1 -b "$XENONRECOMP_BRANCH" "$XENONRECOMP_REPO" "$ROOT/tools/XenonRecomp"
fi
# GCC 14+: anonymous aggregates não podem ter membros com construtor
grep -q "be<uint32_t> Error;" "$ROOT/tools/XenonRecomp/XenonUtils/xbox.h" && \
    sed -i 's/be<uint32_t> Error;/uint32_t Error;/; s/be<uint32_t> Length;/uint32_t Length;/' \
        "$ROOT/tools/XenonRecomp/XenonUtils/xbox.h" || true
grep -q "uint16_t u16;" "$ROOT/tools/XenonRecomp/XenonUtils/xdbf.h" || \
    sed -i 's/be<uint16_t> u16;/uint16_t u16;/' "$ROOT/tools/XenonRecomp/XenonUtils/xdbf.h" || true

# ---- PATCH fh2: time base do guest a 50 MHz REAIS --------------------------
# O emissor de mftb do upstream gera `__rdtsc()` — o tick do HOST (cntvct_el0
# do ARM tem frequência arbitrária do device: 19.2/24/32 MHz…). O título lê o
# TB do guest e divide pelo valor de KeQueryPerformanceFrequency (50 MHz do
# Xenon): com tick de host o relógio do guest fica N vezes mais rápido/lento
# e os timeouts de mídia do jogo estouram (o boot exibia "disco sujo" em ~34 s
# reais). Patch: mftb → fh2TimeBaseTicks() (50.000.000 ticks/s de tempo real,
# idêntico ao console), declarada no ppc_context.h COMMITADO em
# recomp/runtime/ — que o run_recomp.sh passa como headerFilePath e a
# ferramenta embute no ppc_context.h emitido consumido pelo código gerado.
if ! grep -q "fh2TimeBaseTicks" "$ROOT/tools/XenonRecomp/XenonRecomp/recompiler.cpp"; then
    sed -i 's/{}.u64 = __rdtsc();/{}.u64 = fh2TimeBaseTicks();/' \
        "$ROOT/tools/XenonRecomp/XenonRecomp/recompiler.cpp"
fi

cmake -S "$ROOT/tools/XenonRecomp" -B "$ROOT/tools/XenonRecomp/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/tools/XenonRecomp/build" -j"$(nproc)" --target XenonAnalyse XenonRecomp

cp "$ROOT/tools/XenonRecomp/build/XenonAnalyse/XenonAnalyse" "$TOOLS/XenonAnalyse"
cp "$ROOT/tools/XenonRecomp/build/XenonRecomp/XenonRecomp" "$TOOLS/XenonRecomp"

echo "OK: ferramentas em $TOOLS/"
"$TOOLS/XexTool" 2>&1 | head -1 || true
