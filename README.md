# ForzaHorizon2-Recomp-Android

> ## ⚠️ AVISO LEGAL — LEIA PRIMEIRO
>
> **Este projeto NÃO distribui nenhum asset, ROM, ISO, XEX ou qualquer conteúdo
> protegido por direitos autorais de Forza Horizon 2.**
>
> É um projeto **homebrew/educacional** de engenharia reversa e recompilação
> estática, **sem qualquer afiliação com a Microsoft, Playground Games ou Turn
> 10 Studios**. "Forza Horizon" é marca da Microsoft/Xbox Game Studios — as
> menções aqui são apenas nominativas, para descrever interoperabilidade.
>
> **Para usar este app você precisa possuir uma cópia legalmente adquirida do
> jogo (disco do Forza Horizon 2, Xbox 360) e fornecer seus próprios arquivos
> de conteúdo.** O APK gerado pelo CI contém apenas: código recompilado
> derivado do binário do usuário (no build, não no repositório) e um runtime
> original. Os assets do jogo (texturas, áudio, mapa, carros) **não** estão
> aqui e devem ser fornecidos pelo usuário no primeiro boot, via seletor de
> pasta (SAF).

Port nativo **Android (ARM64)** de Forza Horizon 2 (Xbox 360) via
**recompilação estática** com [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) —
recompilando PowerPC → C++ → binário nativo, com runtime desenhado **mobile-first**
(não é um port de PC adaptado).

## Estado do projeto (v0.1.0)

| Componente | Estado |
|---|---|
| Pipeline XEX → análise → C++ | ✅ Funcional (128.081 funções geradas no CI) |
| Ferramentas (XexTool/XenonAnalyse/XenonRecomp) | ✅ Compilam no CI |
| App Android (UI, settings, ciclo de vida) | ✅ Funcional |
| SAF: leitura DIRETA da pasta escolhida (fd via ContentResolver, sem cópia p/ o app) | ✅ Funcional |
| HUD touch multi-touch (direção + pedais simultâneos) | ✅ Funcional |
| Runtime: GLES 3.1 (contexto/superfície com ciclo completo) | ✅ |
| Runtime: memória guest (~2,25 GB, base dinâmica — não depende de mmap hint) | ✅ |
| Runtime: loader XEX2 real (decripta AES-128 + LZX → memória guest, imports consistentes com a análise) | ✅ |
| Runtime: kernel HLE — 75 imports com semântica REAL (heap, tempo, threads guest reais, sync, TLS, printf, input), 26 falhas reais, 287 stubs | ✅ |
| Runtime: execução REAL do guest — tabela mágica de 128.708 funções + entry `_xstart` chamado (CRT do jogo roda) | ✅ |
| Runtime: AAudio (stream low-latency) | ✅ |
| Input: HUD touch de direção + gamepad → XamInputGetState | ✅ |
| Filesystem real para o guest (NtCreateFile/NtReadFile sobre FsProvider) | 🚧 Próximo |
| Tradução D3D9/Xenos → Vulkan/GLES | 🚧 Em progresso |

O APK de cada build está nos **artifacts do GitHub Actions** (workflow `Build APK`).

## Como funciona a recompilação

1. **Você** (dono do jogo) configura o secret `GH_PAT` com um token com acesso
   ao seu repositório privado que contém o `default.xex`.
2. O CI baixa o binário **do seu repo privado**, valida (`XEX2`), descriptografa
   com XexTool, roda XenonAnalyse + XenonRecomp e compila o código gerado
   dentro do APK.
3. O binário e o código gerado **nunca são commitados** aqui
   (`.gitignore` cobre `*.xex`, `recomp/generated/`, `recomp/private/`).

No seu próprio PC, o pipeline local equivale a:

```bash
export GH_PAT=ghp_seu_token
tools/fetch_xex.sh          # baixa + valida + descriptografa (recomp/private/)
tools/build_recomp_tools.sh # compila XexTool, XenonAnalyse, XenonRecomp
tools/run_recomp.sh         # análise + recomp → recomp/generated/
./gradlew assembleDebug     # APK com o código recompilado
```

## Uso no Android

1. Instale o APK (side-load; Android 10+).
2. Selecione a pasta dos arquivos do jogo (SAF) na primeira execução — os
   arquivos são lidos **diretamente da pasta escolhida**, sem cópia para
   dentro do app (só os saves ficam no storage do app). No boot, o `default.xex`
   dessa pasta é decodificado e carregado na memória do jogo pelo runtime.
3. Controles: **touch** (analógico virtual à esquerda = direção; GAS/FREIO à
   direita; MÃO = freio de mão; C+/C- = câmbio) ou **gamepad físico**
   (triggers analógicos = acelerador/freio, LB/RB = câmbio, X = freio de mão).
4. Configurações: escala de resolução (50–100%), FPS 30/60, backend GLES/Vulkan,
   opacidade do HUD.

## Estrutura

```
app/                        módulo Android (Gradle)
  src/main/java/…           MainActivity, GameActivity, TouchHudView, gamepad
  src/main/cpp/             JNI + runtime nativo (gfx/audio/input/fs/ppc)
recomp/
  config/                   FH2.toml (XenonRecomp) + jump tables
  generated/                código C++ gerado (gitignored — gerado no CI)
  runtime/                  ppc_context.h vendored (licença do XenonRecomp)
tools/                      fetch_xex.sh, build_recomp_tools.sh, run_recomp.sh
docs/                       arquitetura, decisões, backlog
.github/workflows/build.yml pipeline de APK
```

## Documentação

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — visão técnica e pipeline
- [docs/DECISIONS.md](docs/DECISIONS.md) — log cronológico de decisões
- [docs/BACKLOG.md](docs/BACKLOG.md) — backlog de kernel/IO (instruções PPC: 0 não implementadas desde o fork fh2 do XenonRecomp)
  (65 mnemônicos — ver issue #9), kernel/IO, próximos passos

## Licença

Código original do runtime e ferramentas de build: **MIT** (LICENSE).
`recomp/runtime/ppc_context.h` é do projeto XenonRecomp (MIT inclusa ao lado).
Nenhum binário ou asset do jogo é distribuído por este repositório.
