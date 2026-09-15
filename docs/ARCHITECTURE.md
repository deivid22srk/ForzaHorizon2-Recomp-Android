# Arquitetura — ForzaHorizon2-Recomp-Android

## Visão geral

```
┌─────────────────────────────────────────────────────────────┐
│                     APK (arm64-v8a)                         │
│                                                             │
│  ┌───────────── Java/Kotlin (Gradle, app/) ──────────────┐  │
│  │ MainActivity (disclaimer, SAF, settings)              │  │
│  │ GameActivity (SurfaceView + ciclo de vida)            │  │
│  │ TouchHudView (HUD de direção) + GameControllerManager │  │
│  └───────────────────────┬───────────────────────────────┘  │
│                          │ JNI (NativeBridge)               │
│  ┌───────────────────────▼───────────────────────────────┐  │
│  │            libfh2recomp.so (C++20, NDK r27)           │  │
│  │                                                       │  │
│  │  native_main (lifecycle, input queue)                 │  │
│  │   ├── runtime/gfx    (GLES 3.1 ✓ / Vulkan skeleton)   │  │
│  │   ├── runtime/audio  (AAudio, low-latency)            │  │
│  │   ├── runtime/input  (estado X360 consolidado)        │  │
│  │   ├── runtime/fs     (SAF + app storage)              │  │
│  │   ├── runtime/ppc    (memória guest, boot)            │  │
│  │   └── recomp/generated/*.cpp  (128.081 funções PPC    │  │
│  │       recompiladas — gerado no CI a partir do XEX     │  │
│  │       do usuário; NÃO commitado no repo)              │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Pipeline de recompilação (CI)

1. **Download autenticado**: `tools/fetch_xex.sh` baixa o `default.xex` do repo
   privado do usuário via `secrets.GH_PAT` (API de Contents; `/raw/` retorna 404
   para esse repo). Validações: HTTP 200, tamanho > 0, magic `XEX2`.
2. **Descriptografia**: XexTool (Team-Resurgent, xorloser v7.0) — o XEX do FH2
   é `RETAIL + UNCOMPRESSED + ENCRYPTED`; convertemos para unencrypted.
3. **Análise**: XenonAnalyse gera o TOML de jump tables.
   ⚠️ No FH2 a detecção atual (calibrada para Sonic Unleashed) não encontrou
   padrões — backlog [#2].
4. **Save/restore functions**: `tools/find_save_restore.py` localiza as 8
   funções (`__restgprlr_14` etc.) via byte patterns documentados. Todas
   encontradas com hit único — ver `recomp/config/save_restore.txt`.
5. **XenonRecomp**: TOML (`recomp/config/FH2.toml`) + `ppc_context.h` →
   506 arquivos C++, ~266 MB, 128.081 funções.
6. **Compilação nativa**: CMake/NDK (clang), `-O1` no código gerado,
   `-Wl,-z,max-page-size=16384` (Android 15). STL: `c++_shared`.

## Decisões mobile-first

| Área | Decisão | Racional |
|------|---------|----------|
| Gráficos | Vulkan preferencial, GLES 3.1 fallback | Adreno/Turnip; GLES cobre o resto. Sem backend D3D intermediário. |
| Áudio | AAudio (NDK nativo), 48 kHz float, EXCLUSIVE/LOW_LATENCY | Sem dependências externas; latência de game audio. |
| Input | HUD de direção (stick esq., acelerador/freio à direita) + gamepad BT/USB com triggers analógicos | Pensado para corrida, não para combate. |
| Arquivos | SAF (ACTION_OPEN_DOCUMENT_TREE) + app-specific storage | Sem permissões de storage brutas; usuário aponta a pasta do jogo. |
| Memória | Imagem guest alinhada a 64 KB + 16 MB de margem | Page size do Xenon; streaming de mundo aberto exige gestão cuidadosa. |
| App lifecycle | surfaceCreated/Destroyed → nativeSetSurface/onSurfaceLost; pause/resume nativos | Perda de contexto ao minimizar é obrigatória em Android. |

## Setjmp/longjmp

Não identificados no binário com os padrões atuais (busca por `ld r14,X(r3)`
consecutivos não casou). Foram omitidos do TOML: o recompilador os trata como
funções comuns. Consequência: SjLj/exceptions do guest não funcionam —
rastreado como backlog [#4]. Jogos usam isso principalmente em C++ exceptions;
o FH2 pode rodar sem até que um caminho de exceção seja atingido.
