# Decisões técnicas — log cronológico

## 2026-09-15 — Sessão inicial do pipeline

### D1 — Download via API de Contents (não via `/raw/`)
O caminho `https://github.com/<repo>/raw/refs/heads/main/default.xex` retornou
HTTP 404 mesmo com PAT válido (verificado em `GET /user` → login OK e
`GET /repos/...` → repo privado acessível). A API de Contents
(`GET /repos/<repo>/contents/default.xex` com `Accept: application/vnd.github.v3.raw`)
funcionou: 21.807.104 bytes, magic XEX2 confirmado.

### D2 — Descriptografia do XEX antes da análise
XexTool identificou o módulo como **RETAIL + UNCOMPRESSED + ENCRYPTED**
("Forza Horizon 2", Default.exe, load 0x82000000, entry 0x82BF2CD0,
Title 4D530AA4/MS-2724). XenonAnalyse/XenonRecomp exigem código legível:
convertemos para unencrypted (`XexTool -e u`). O arquivo descriptografado
nunca vai para o repo (gitignored em `recomp/private/`).

### D3 — Patch GCC 14 nas ferramentas upstream
O XenonRecomp não compila no GCC ≥ 14 (`be<uint32_t>` com construtor dentro
de anonymous aggregates — `xbox.h`/`xdbf.h`). Correção mínima e sem efeito
semântico (campos não usados): trocar por POD. Aplicada também no
`tools/build_recomp_tools.sh` para o CI.

### D4 — Caminho absoluto no TOML segfaulta o XenonRecomp
`Recompiler::LoadConfig` concatena `config.directoryPath + config.filePath`;
caminho absoluto no TOML produz path inválido → `LoadFile` vazio →
`Image::ParseImage(nullptr)` → SEGV (confirmado com build ASan em
`image.cpp:34`). Regra: sempre caminhos **relativos** ao TOML.

### D5 — Jump tables do FH2 não detectadas pelo XenonAnalyse
Execução limpa (15,6 s, exit 0), mas zero tabelas — a heurística é calibrada
para o padrão de compilador do Sonic Unleashed. Impacto: funções com jump
tables viram saltos indiretos genéricos (e boundaries podem ser imprecisos).
Backlog [#5]: estender a detecção (busca por `mtctr r0; bctr` com padrões do
compilador do FH2) ou fornecer boundaries manuais para as funções críticas.

### D6 — setjmp/longjmp omitidos do TOML
Buscas por padrões de jmp_buf (`ld/std r14..r31, X(r3)` com offsets
consecutivos) não localizaram candidatos únicos. Risco de identificar errado
> benefício; README do XenonRecomp prevê omissão. Backlog [#4].

### D7 — Código gerado não é commitado (regenerado no CI)
506 arquivos (~266 MB) excedem o razoável para um repositório de código e o
binário original é conteúdo protegido — o CI regenera a partir do XEX do
usuário via `secrets.GH_PAT`. Cache de objetos `.cxx` reduz builds seguintes.
Também simplifica a posição legal do repo (só ferramentas + runtime próprios).

### D8 — Save/restore functions: endereços confirmados
Padrões de bytes do README produziram hit **único** para as 8 funções:

| Função | Endereço |
|---|---|
| restgprlr_14 | 0x82C010D0 |
| savegprlr_14 | 0x82C01080 |
| restfpr_14 | 0x82C015EC |
| savefpr_14 | 0x82C015A0 |
| restvmx_14 | 0x82C026D8 |
| savevmx_14 | 0x82C02440 |
| restvmx_64 | 0x82C0276C |
| savevmx_64 | 0x82C024D4 |

### D9 — Compilação do código gerado: clang builtins + `-O1`
O código usa `__builtin_assume`, `__builtin_rotateleft32/64`,
`__builtin_debugtrap` (clang). NDK r27 (clang) suporta todos nativamente.
Teste local (GCC+shim): 1 arquivo = 3,5 s a `-O1` → ~506 arquivos ≈ 30 min
em 1 core, ~10 min em runner 4-core. `-O3` fica para depois do runtime
funcional (readme do XenonRecomp recomenda otimizações por último).

### D10 — NDK r27 + 16 KB pages
`-Wl,-z,max-page-size=16384` no link do `.so` (exigência do Android 15 para
ARM64) e `useLegacyPackaging true` para garantir `.so` extraído (APK
side-load em qualquer device).

## 2026-09-15 — Sessão de correções de build (continuação)

### D11 — Shims GCC no ppc_context.h (builtins do Clang)
O código gerado usa `__builtin_rotateleft32/64`, `__builtin_assume` e
`__builtin_debugtrap` (exclusivos do Clang) e assume `__rdtsc` de headers
de sistema em x86. O CI (NDK/clang, ARM64) cobre tudo nativamente, mas o
header agora provê shims GCC (rotação portátil com cuidado de shift por 0,
`__builtin_trap`, `<x86intrin.h>`), permitindo compilar o código gerado com
g++ localmente. Caminho Clang/ARM64 permanece idêntico.

### D12 — simde resolvido do clone do XenonRecomp
`ppc_context.h` inclui `<x86/avx.h>` (simde) — sem isso nem o CI compila.
O CMake agora busca simde em `tools/XenonRecomp/thirdparty/simde` (já clonado
por `build_recomp_tools.sh`) ou `app/src/main/cpp/thirdparty/simde`, e falha
com mensagem clara se `FH2_HAS_RECOMP=1` e simde não existir.

### D13 — libvulkan explícita no link
O backend Vulkan chama `vkCreateInstance`/`vkEnumeratePhysicalDevices`
diretamente: sem `find_library(vulkan-lib vulkan)` o link do `.so` falharia
no CI (detectado na análise de símbolos do harness local).

### D14 — Keycode canônico no native
O mapeamento gamepad Android→guest foi unificado: Java envia o keycode
Android bruto (`e.getKeyCode()`) e o native (`input_state.cpp`) mantém o
único mapa canônico (AKEYCODE_BUTTON_x/DPAD → máscara XInput-like). Elimina
dupla tradução e divergência entre Java e C++. Eventos chegam via
`dispatchKeyEvent`/`dispatchGenericMotionEvent` na GameActivity.

### D15 — Harness local de validação (hostcheck)
Antes de cada push, o runtime passa por `-fsyntax-only` no g++ com stubs
Android mínimos (`android/log.h`, `jni.h`, `aaudio/AAudio.h`,
`android/input.h` com constantes reais) + headers reais Vulkan/EGL (Khronos)
e uma amostra estratificada do código gerado é compilada a objeto, com
análise de símbolos indefinidos (nm). Objetivo: encurtar o ciclo de
feedback sem depender de rodadas de CI de ~20 min.
