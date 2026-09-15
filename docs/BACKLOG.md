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

- [ ] **Imports do XEX**: resolver a tabela de imports (xam/xboxkrnl) para
      stubs nativos — pré-requisito para chamar o entry point do guest
- [ ] **Chamada do entry**: `0x82BF2CD0` via function table (perfeito hash do
      ppc_func_mapping.cpp já gerado)
- [ ] **MMIO**: XMA decoder (áudio do jogo usa XMA), GPU pushbuffer
- [ ] **Tradução Xenos→Vulkan/GLES**: vertex/fragment shaders Xenos
      (microcode) → SPIR-V/GLSL; pipeline cache persistente em disco
- [ ] **Streaming de mundo**: gerenciador de memória mobile (FH2 é open-world
      com streaming constante)
- [ ] **Setjmp/longjmp**: identificar endereços (issue #15)
- [ ] **Jump tables**: detecção para o padrão do compilador do FH2 (issue #14)

## Infra

- [ ] Assinatura de release com keystore via secrets (KEYSTORE_BASE64 etc.)
- [ ] Cache de artefato do código gerado (zip por hash do XEX) para acelerar
      builds que não alteram o binário
- [ ] Testes unitários do runtime de input (snapshot/máscara atômica)
