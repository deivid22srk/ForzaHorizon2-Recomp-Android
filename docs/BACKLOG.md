# Backlog — instruções não implementadas e próximos passos

## Warnings de recompilação (XenonRecomp v0.1)

Total: **4.249 sites** de "Unrecognized instruction" em **65 mnemônicos**
distintos. Nenhum foi ignorado silenciosamente: o inventário completo está na
issue #9 (tabela integral) e o agrupamento por família nas issues #10-#13.
Top 15 por impacto:

| # | Instrução | Ocorrências | Tipo | Complexidade |
|---|-----------|-------------|------|--------------|
| 1 | `stfsu` | 865 | store FP com update | baixa (variante de stfs) |
| 2 | `bdzf` | 695 | branch decrement if zero | média |
| 3 | `lfsu` | 441 | load FP com update | baixa |
| 4 | `sthu` | 364 | store halfword com update | baixa |
| 5 | `lhzu` | 224 | load halfword com update | baixa |
| 6 | `vsel128` | 147 | VMX128 select | média |
| 7 | `eqv` | 134 | equivalent (NOT XOR) | baixa |
| 8 | `subfze` | 101 | subtract from extended | média (carry) |
| 9 | `lfsux` | 78 | load FP com update indexado | baixa |
| 10 | `bso` | 65 | branch if summary overflow | média |
| 11 | `stvlxl128` | 64 | VMX128 store least | média |
| 12 | `lvxl128` | 64 | VMX128 load least | média |
| 13 | `vaddsws` | 63 | VMX add signed saturate | média |
| 14 | `vpkswss128` | 62 | VMX128 pack saturate | média |
| 15 | `vslo128` | 60 | VMX128 shift left | média |

**Nota**: os mnemônicos `xxx128` são o conjunto VMX128 do Xenon (aliases de
instruções VMX com operandos estendidos); o XenonRecomp os reporta porque a
tabela de aliases está incompleta. Implementá-los no recompilador elimina os
~4.249 pontos de `PPC_BREAK` gerados. Rastreamento: #9 (umbrella), #10-#13
(famílias).

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
