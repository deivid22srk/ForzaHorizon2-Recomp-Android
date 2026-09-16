# Backlog — instruções não implementadas e próximos passos

## Instruções PPC — RESOLVIDO (fork do XenonRecomp, branch fh2)

Estado anterior: 4.249 sites de "Unrecognized instruction" em 65 mnemônicos
(inventário integral na issue #9, famílias nas issues #10-#13).

**Resolvido em 2026-09-16**: os 65 mnemônicos foram implementados no
próprio recompilador (fork deivid22srk/XenonRecomp, branch `fh2`, commit
a062ef6) — load/store com update (D e X-form), família bdz*, bso/bns,
addc/addme/subfze, eqv, mullhwu., todos os VMX/VMX128 ausentes (vsel/
vsel128 com a semântica PowerISA corrigida, packs com two-pass alias-safe,
shifts de registrador inteiro com decodificação QEMU, conversões
fixpoint) e lvehx/lhbrx/dcbst/frsqrte. Validação: regeneração completa
(503 TUs) compila 100% no host (g++ -fsyntax-only + amostra -c).

O pipeline (tools/build_recomp_tools.sh) clona o fork automaticamente;
override via FH2_XENONRECOMP_REPO/FH2_XENONRECOMP_BRANCH.

## Runtime — feito nesta fase

- [x] **Loader XEX2 real** (2026-09-16): `runtime/ppc/xex_loader.cpp` lê o
      default.xex do usuário (fd SAF direto), decodifica (AES-128 retail +
      none/basic/LZX), aplica o patch de imports igual ao da análise e copia
      para a memória guest (base 0x82000000, entry 0x82BF2CD0 validados
      contra o jogo real; 388 thunks de função patchados = 388 stubs HLE)
- [x] **Memória guest sem hint fixa** (2026-09-16): base dinâmica aceita
      (o código gerado recebe `base` por parâmetro) — boot funciona em
      devices que não honram mmap hints baixas

## Kernel/IO (runtime)

- [x] **Imports do XEX**: 388 imports despachados — 78 com SEMÂNTICA REAL
      (heap/janela virtual, tempo, threads guest reais, events/semaphores/
      mutants, KeTls*, critical sections, printf/DbgPrint reais sobre memória
      guest, XamInputGetState real, Mm* físicas), 26 com falha REAL do estado
      (FS/perfis/rede ainda indisponíveis), 287 stubs log-once (2026-09-16)
- [x] **Boot do CRT resolvido** (2026-09-16, D25): o guest saía por
      HalReturnToFirmware(1) em 7 ms porque NtAllocateVirtualMemory usava
      assinatura errada (size em r5 em vez de *r4; tipo em r6 em vez de r5).
      Com a assinatura real do xboxkrnl (Xenia como referência), LARGE_PAGES
      64k, commit-dentro-de-reserva e NtQueryVirtualMemory real, o CRT cria
      o heap inicial e o boot PROSSEGUE (validado no host: 60 s executando).
- [x] **Rastreio HLE** (2026-09-16, D25): traceCall/traceReturn nos 388
      imports (ring de 256 + dump FH2/TRACE no encerramento) — o próximo
      log do device mostra a sequência exata de HLE que o guest executar.
- [x] **Chamada do entry**: `0x82BF2CD0` chamado de verdade (guest_entry.cpp):
      tabela mágica de 128.708 funções populada na memória guest, PPCContext
      com stack 16 MB + TLS do XEX_HEADER_TLS_INFO, CRT `_xstart` executa
      (2026-09-16). Boot REAL validado no host (hostrun_boot.sh — o C++
      recompilado é portável) e no device na próxima build
- [ ] **Filesystem real**: NtCreateFile/NtReadFile/… sobre o FsProvider (fd
      SAF direto) — o guest hoje recebe STATUS_OBJECT_NAME_NOT_FOUND (falha
      honesta); próximo passo natural após o boot
- [ ] **VdSwap/GPU**: VdSwap e VdInitializeRingBuffer são os pontos de
      entrada do caminho gráfico real (issue #17)
- [ ] **MMIO**: XMA decoder (áudio do jogo usa XMA), GPU pushbuffer
- [ ] **Tradução Xenos→Vulkan/GLES**: vertex/fragment shaders Xenos
      (microcode) → SPIR-V/GLSL; pipeline cache persistente em disco
- [ ] **Streaming de mundo**: gerenciador de memória mobile (FH2 é open-world
      com streaming constante)
- [ ] **Setjmp/longjmp**: identificar endereços (issue #15)
- [ ] **Jump tables**: detecção para o padrão do compilador do FH2 (issue #14)
- [ ] **Threads**: preempção de guest sem pontos de HLE (spin) — hoje o stop
      retém a memória guest (leak controlado, D24) em vez de matar no meio

## Infra

- [x] Assinatura de release com keystore via secrets (KEYSTORE_BASE64 etc.)
- [ ] Boot test do host no CI (hostrun_boot.sh como gate executável)
- [ ] Cache de artefato do código gerado (zip por hash do XEX) para acelerar
      builds que não alteram o binário
- [ ] Testes unitários do runtime de input (snapshot/máscara atômica)
