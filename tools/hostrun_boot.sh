#!/usr/bin/env bash
# hostrun_boot.sh — compila e roda o boot REAL do guest no HOST (x86_64)
# Uso: nohup bash scripts/hostrun_boot.sh > workspace/hostrun/build.log 2>&1 &
set -u
WS="${HOSTRUN_WS:-$(pwd)/../..}"
REPO=$WS/repo
B=$WS/hostrun/build
mkdir -p "$B"

INC="-I$WS/hostrun/include \
     -I$WS/hostcheck/include \
     -I$REPO/app/src/main/cpp \
     -I$REPO/recomp/runtime \
     -I$REPO/recomp/generated \
     -I$REPO/tools/XenonRecomp/thirdparty/simde \
     -I$REPO/tools/XenonRecomp/XenonUtils \
     -I$REPO/tools/XenonRecomp/thirdparty/tiny-AES-c \
     -I$REPO/tools/XenonRecomp/thirdparty/TinySHA1 \
     -I$REPO/tools/XenonRecomp/thirdparty/libmspack/libmspack/mspack"

CXXFLAGS="-std=c++20 -c -O0 -g0 -w -fPIC -DFH2_HAS_RECOMP=1 -DANDROID -DANDROID_API=28"

echo "== [1/4] runtime TUs =="
RT_TUS="app/src/main/cpp/native_main.cpp
 app/src/main/cpp/runtime/ppc/ppc_runtime.cpp
 app/src/main/cpp/runtime/ppc/kernel_state.cpp
 app/src/main/cpp/runtime/ppc/kernel_real.cpp
 app/src/main/cpp/runtime/ppc/kernel_hle.cpp
 app/src/main/cpp/runtime/ppc/guest_alloc.cpp
 app/src/main/cpp/runtime/ppc/guest_entry.cpp
 app/src/main/cpp/runtime/ppc/xex_loader.cpp
 app/src/main/cpp/runtime/fs/fs_provider.cpp
 app/src/main/cpp/runtime/input/input_state.cpp"
for f in $RT_TUS; do
    o="$B/rt_$(basename $f .cpp).o"
    g++ $CXXFLAGS $INC "$REPO/$f" -o "$o" || { echo "FALHOU: $f"; exit 1; }
done
g++ $CXXFLAGS $INC -I$REPO/tools/XenonRecomp/thirdparty/libmspack/libmspack/mspack /home/z/my-project/scripts/hostrun_boot.cpp -o "$B/hostrun_boot.o" || exit 1

echo "== [2/4] XenonUtils + AES + LZX =="
for f in xex image xex_patcher memory_mapped_file; do
    g++ $CXXFLAGS $INC "$REPO/tools/XenonRecomp/XenonUtils/$f.cpp" -o "$B/xu_$f.o" || exit 1
done
gcc -c -O1 -w -fPIC "$REPO/tools/XenonRecomp/thirdparty/tiny-AES-c/aes.c" -o "$B/aes.o" || exit 1
gcc -c -O1 -w -fPIC -I$REPO/tools/XenonRecomp/thirdparty/libmspack/libmspack/mspack "$REPO/tools/XenonRecomp/thirdparty/libmspack/libmspack/mspack/lzxd.c" -o "$B/lzxd.o" || exit 1

echo "== [3/4] código gerado (506 TUs, 2 jobs) — $(date) =="
ls $REPO/recomp/generated/*.cpp | xargs -P 2 -I{} bash -c '
    o="'"$B"'/gen_$(basename {} .cpp).o"
    [ -f "$o" ] || g++ '"$CXXFLAGS $INC"' {} -o "$o" || echo "FALHOU: {}"
'
GEN_FAILED=$(grep -c FALHOU "$B/../build.log" 2>/dev/null || true)
echo "gerados: $(ls $B/gen_*.o 2>/dev/null | wc -l) objetos"

echo "== [4/4] link =="
g++ -o "$B/boot_test" $B/*.o -lpthread || { echo "LINK FALHOU"; exit 1; }
echo "== BOOT TEST =="
cd $WS/hostrun && "$B/boot_test" gamedir 45
RC=$?
echo "== boot_test rc=$RC — $(date) =="
