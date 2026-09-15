# CMakeLists nativo — libfh2recomp.so
# Runtime mobile-first: código recompilado (recomp/generated) + runtime Android
# (gráficos GLES3/Vulkan, áudio AAudio, input, fs). Se recomp/generated não
# existir, o app compila em modo "shell" (UI e pipeline de setup funcionam).
cmake_minimum_required(VERSION 3.22)
project(fh2recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 16 KB page alignment (Android 15+ / NDK r27)
add_link_options(-Wl,-z,max-page-size=16384)

set(RECOMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../recomp)
set(GENERATED_DIR ${RECOMP_DIR}/generated)

add_library(fh2recomp SHARED
    native_main.cpp
    runtime/gfx/graphics_backend.cpp
    runtime/gfx/gles3_backend.cpp
    runtime/gfx/vulkan_backend.cpp
    runtime/audio/audio_output.cpp
    runtime/input/input_state.cpp
    runtime/fs/fs_provider.cpp
    runtime/ppc/ppc_runtime.cpp
    runtime/ppc/kernel_hle.cpp)
target_include_directories(fh2recomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RECOMP_DIR}/runtime)

# --- Código recompilado (gerado pelo XenonRecomp a partir do default.xex do usuário) ---
file(GLOB_RECURSE RECOMP_SOURCES ${GENERATED_DIR}/*.cpp)
list(LENGTH RECOMP_SOURCES RECOMP_COUNT)
message(STATUS "Arquivos recompilados encontrados: ${RECOMP_COUNT}")

if(RECOMP_COUNT GREATER 0)
    target_sources(fh2recomp PRIVATE ${RECOMP_SOURCES})
    target_compile_definitions(fh2recomp PRIVATE FH2_HAS_RECOMP=1)
    # O código gerado é enorme (128k funções): -O1 equilibra tempo de CI e desempenho
    set_source_files_properties(${RECOMP_SOURCES} PROPERTIES COMPILE_OPTIONS "-O1")
    target_include_directories(fh2recomp PRIVATE ${GENERATED_DIR} ${RECOMP_DIR}/runtime)
else()
    target_compile_definitions(fh2recomp PRIVATE FH2_HAS_RECOMP=0)
endif()

# --- Loader XEX2 do runtime: reutiliza o XenonUtils vendido (mesmo pipeline
# --- de decriptação/descompressão da análise — consistência garantida) ---
set(TOOLS_RECOMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../tools/XenonRecomp)
set(XU_DIR ${TOOLS_RECOMP_DIR}/XenonUtils)
if(EXISTS ${XU_DIR}/xex.cpp)
    target_sources(fh2recomp PRIVATE
        runtime/ppc/xex_loader.cpp
        ${XU_DIR}/xex.cpp
        ${XU_DIR}/image.cpp
        ${XU_DIR}/xex_patcher.cpp
        ${XU_DIR}/memory_mapped_file.cpp
        ${TOOLS_RECOMP_DIR}/thirdparty/tiny-AES-c/aes.c
        ${TOOLS_RECOMP_DIR}/thirdparty/libmspack/libmspack/mspack/lzxd.c)
    target_include_directories(fh2recomp PRIVATE
        ${XU_DIR}
        ${TOOLS_RECOMP_DIR}/thirdparty/tiny-AES-c
        ${TOOLS_RECOMP_DIR}/thirdparty/TinySHA1
        ${TOOLS_RECOMP_DIR}/thirdparty/libmspack/libmspack/mspack)
elseif(RECOMP_COUNT GREATER 0)
    message(FATAL_ERROR "FH2_HAS_RECOMP=1 mas XenonUtils ausente em tools/XenonRecomp "
        "— o loader XEX do runtime não pode compilar. Rode tools/build_recomp_tools.sh primeiro.")
endif()

# --- Vulkan priorizado (Adreno/Turnip); GLES 3.1 permanece como fallback ---
target_compile_definitions(fh2recomp PRIVATE FH2_ENABLE_VULKAN=1)

# --- Dependências do código gerado (ppc_context usa simde para VMX em ARM64) ---
# O clone do XenonRecomp (tools/build_recomp_tools.sh) já traz o simde pinado;
# um checkout em app/src/main/cpp/thirdparty/simde também é aceito para builds locais.
set(SIMDE_CANDIDATES
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/simde
    ${RECOMP_DIR}/../tools/XenonRecomp/thirdparty/simde)
set(SIMDE_DIR "")
foreach(CAND ${SIMDE_CANDIDATES})
    if(EXISTS ${CAND})
        set(SIMDE_DIR ${CAND})
        break()
    endif()
endforeach()
if(SIMDE_DIR)
    target_include_directories(fh2recomp PRIVATE ${SIMDE_DIR})
elseif(RECOMP_COUNT GREATER 0)
    message(FATAL_ERROR "FH2_HAS_RECOMP=1 mas simde não encontrado (ppc_context.h inclui <x86/avx.h>). "
        "Rode tools/build_recomp_tools.sh ou clone simde em app/src/main/cpp/thirdparty/simde.")
endif()

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)
find_library(aaudio-lib aaudio)
find_library(vulkan-lib vulkan)

target_link_libraries(fh2recomp PRIVATE
    ${log-lib} ${android-lib} ${EGL-lib} ${GLESv3-lib} ${aaudio-lib} ${vulkan-lib})
