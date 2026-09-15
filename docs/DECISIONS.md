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

### D16 — Modelo de memória guest: mmap com hint fixa (revisão)
O código gerado endereça memória de forma ABSOLUTA (`base + endereço guest`,
com base = 0x82000000). A alocação original (~39 MB) era incompatível — o
primeiro acesso cometeria wild pointer. Fix: `mmap` com hint em 0x02000000 e
~2,25 GB MAP_NORESERVE → guest em 0x84000000, com espaço acima para a magic
function table. O Android 64-bit honra hints livres; boot falha com log claro
se o device negar o hint (comportamento explícito em vez de corrupção).

### D17 — Concorrência do runtime serializada (revisão)
boot (thread fh2-boot) × stop/pause/surface (UI thread) tinham corridas
(UAF/orphan thread). Estado global agora serializado por lifecycleMutex;
join do jogo fora do lock; superfície solta ANTES do pause (null surface).
Eixos de input viraram atômicos (deadzone radial reescalada).

### D18 — Multi-touch correto no HUD (revisão)
O HUD original rastreava UM botão pressionado e usava getX() do pointer 0 —
impossível dirigir+acelerar; soltar o gás quebrava a direção. Reescrito com
pointer IDs por controle (SparseIntArray), stick segue SEU dedo, botões
simultâneos, métricas em dp, opacidade aplicada ao alpha de cada cor e
haptics nos botões.

### D19 — SAF de verdade: cópia lazy (revisão)
O fluxo anterior persistia o URI e `fopen()`-ava a string `content://`
(impossível). Agora: `FsProvider::readFile` tenta o filesDir e, se ausente,
chama `NativeBridge.copyFromSaf` (JNI) que percorre a árvore DocumentsContract
e copia o arquivo único sob demanda — sem duplicar o jogo inteiro. Escritas
atômicas (tmp+rename), guard anti-traversal, filesDir resolvido via JNI
(compatível com o suffix .debug do build debug).

### D20 — CI: cache .cxx removido + assinatura real (revisão)
O cache de app/.cxx restaurava objetos com mtime novo → ninja poderia pular
TUs alterados (build stale silencioso); removido. Release agora assina com
keystore via secrets (KEYSTORE_BASE64/PASSWORD/ALIAS/KEY_PASSWORD) com
fallback debug-signed (APK instalável). fetch_xex.sh aceita FH2_XEX_REPO (env).

### D21 — SAF: leitura DIRETA por fd (sem cópia) + fix de takePersistableUriPermission
Bug de produção: ao escolher a pasta, o app fechava com
`IllegalArgumentException: Requested flags 0x41, but only 0x3 are allowed` —
o mask de `takePersistableUriPermission` incluía FLAG_GRANT_PERSISTABLE (0x40),
que NÃO é aceito pela API (só READ|WRITE), e o catch cobria só SecurityException.
Correção: mask = flags do intent & (READ|WRITE); catch amplia para
IllegalArgumentException (ROMs excêntricas).

Leitura: `NativeBridge.openSaf(guestPath)` resolve o caminho na árvore
DocumentsContract (cache por caminho, invalidado em setAppContext), abre via
`openFileDescriptor("r")` e devolve fd cru (`detachFd`); o native lê com
`pread` (tolera fd não-seekable com leitura incremental) e fecha — NENHUM
byte do jogo é copiado para o storage do app. `FsProvider::readFile` vira:
(1) fd direto SAF → (2) filesDir local (arquivo colocado à mão) → (3) cópia
lazy legada (provider sem openFileDescriptor). Escritas (saves) continuam no
filesDir do app — a pasta escolhida nunca é modificada.

### D22 — Instruções PPC: 65 mnemônicos implementados no recompilador (fork fh2)
O backlog de 4.249 sites foi erradicado na RAIZ: emitters reais no XenonRecomp
(fork deivid22srk/XenonRecomp branch fh2, commit a062ef6), não stubs. Decisões
de semântica documentadas no commit: (1) update-forms computam EA do rA
original antes de gravar rA=EA; (2) bdzf = CTR==0 && !CR[BI] (bit REAL do BI —
e fix do bdnzf upstream que assumia eq); (3) vsel/vsel128 = (A&C)|(B&~C)
(PowerISA; upstream tinha C invertido); (4) CA de addc/addme/subfze segue o
modelo bit-31 já usado por ADDE/ADDIC/ADDZE no upstream; (5) shifts de
registrador inteiro (vsl/vslo/vsro +128) usam a decodificação QEMU do
quantitativo no storage byte-reversed; (6) packs usam two-pass para serem
alias-safe com vD==vA/vB; (7) frsqrte = 1/sqrt exato (dentro da margem da
ISA; converge igual nas iterações de Newton-Raphson do jogo). CI clona o
fork (FH2_XENONRECOMP_REPO/BH overrides). Validação: 503/503 TUs gerados
compilam; 0 "Unrecognized instruction".

### D23 — mmap de base dinâmica + loader XEX2 real no runtime (boot no device)
Bug de produção (moto g34 5G, Android 15, log yAVtPSny): o boot abortava com
"mmap hint 0x0200000 falhou — modelo base+addr inviável". O código exigia que
o kernel honrasse a hint fixa (memcmp do endereço devolvido); o Android daquele
device devolveu 0x6ede710000 (região baixa ocupada) e o mapeamento VÁLIDO foi
descartado. Correção conceitual: no código gerado pelo XenonRecomp, `base` é um
ponteiro recebido POR PARÂMETRO em toda função (PPC_LOAD/STORE = *(base+addr)),
logo o modelo base+addr é válido em QUALQUER endereço — a hint é só conveniência
de depuração. Agora: (1) mmap com hint, sem MAP_FIXED; (2) se o kernel mover,
o endereço devolvido é ACEITO; (3) se MAP_FAILED, nova tentativa sem hint;
(4) só falha de verdade quando não há memória virtual. Validado no host
reproduzindo o modo de falha do device (hint bloqueada com MAP_FIXED_NOREPLACE).

Na mesma inicialização o runtime agora CARREGA o XEX2 de verdade
(runtime/ppc/xex_loader.cpp): lê default.xex via FsProvider (fd SAF direto —
D21), valida magic/tamanho, decodifica com o MESMO pipeline da análise
(XenonUtils::Xex2LoadImage — AES-128 retail key + descompressão none/basic/LZX
via libmspack), aplica o patch de imports idêntico ao do recompilador
(twi0;twi0;twi0;blr nos thunks de função) e copia a imagem para
memBase_+0x82000000. A consistência com o código gerado é garantida por ser o
MESMO código vendido em tools/. Validação real contra o jogo (host): base
0x82000000, entry 0x82BF2CD0, size 0x1700000, MZ na memória guest, e a tabela
de imports confere endereço por endereço: 789 descritores = 388 funções
patchadas (exatamente o número de stubs HLE — issue #16) + 401 variáveis
intactas. Execução do entry point continua bloqueada por kernel/IO (BACKLOG) —
sem promessa falsa: o boot agora chega ao frame loop com a imagem do jogo
carregada na memória guest.
