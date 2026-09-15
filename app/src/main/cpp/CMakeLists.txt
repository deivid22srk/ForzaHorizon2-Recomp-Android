# CMakeLists nativo — libfh2recomp.so
# Runtime mobile-first: código recompilado (recomp/generated) + runtime Android
# (gráficos GLES3/Vulkan, áudio AAudio, input, fs). Se recomp/generated não
# existir, o app compila em modo "shell" (UI e pipeline de setup funcionam).
cmake_minimum_required(VERSION 3.22)
project(fh2recomp LANGUAGES CXX)

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
    runtime/ppc/ppc_runtime.cpp)

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

# --- Dependências do código gerado (ppc_context usa simde para VMX em ARM64) ---
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/simde)
    target_include_directories(fh2recomp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/simde)
endif()

find_library(log-lib log)
find_library(android-lib android)
find_library(EGL-lib EGL)
find_library(GLESv3-lib GLESv3)
find_library(aaudio-lib aaudio)

target_link_libraries(fh2recomp PRIVATE
    ${log-lib} ${android-lib} ${EGL-lib} ${GLESv3-lib} ${aaudio-lib})
