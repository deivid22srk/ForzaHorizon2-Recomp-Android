// guest_entry.h — execução REAL do código recompilado (PowerPC → ARM64)
//
// Fase "kernel/IO" da issue #16 concluída no essencial: este módulo popula a
// tabela mágica de funções na memória guest (usada por TODA chamada indireta
// gerada — vtables, function pointers, imports), resolve o entry point do
// XEX no PPCFuncMappings, monta o PPCContext (stack r1 + TLS r13) e chama o
// código do jogo de verdade — inclusive nas threads guest criadas via
// ExCreateThread.
//
// Compila apenas com FH2_HAS_RECOMP=1 (depende do código gerado).
#pragma once

#include <atomic>
#include <cstdint>

namespace fh2::kern { struct GuestThread; }

namespace fh2::ppc {

struct XexImageInfo;

/** Escreve os ponteiros de PPCFuncMappings na tabela mágica guest
 *  (base + PPC_IMAGE_BASE + PPC_IMAGE_SIZE + (addr-CODE_BASE)*2).
 *  Retorna o número de entradas populadas. Deve ser chamado UMA vez após
 *  carregar a imagem e antes de qualquer execução guest. */
uint64_t populateMagicTable(uint8_t* guestMemZero);

/** Resolve o endereço guest → PPCFunc* (mapa construído de PPCFuncMappings).
 *  nullptr se não houver função recompilada nesse endereço. */
void* resolveGuestFunc(uint32_t guestAddr);

/** Corpo real de uma thread guest (usado por kern::setThreadBody): monta
 *  stack/TLS/PPCContext e executa startRoutine(startContext). */
void guestThreadBody(kern::GuestThread& t, uint8_t* guestMemZero,
                     const XexImageInfo& img);

/** Executa o entry point do XEX na thread atual (gameMain). Bloqueia até o
 *  guest retornar/terminar ou stopRequested disparar o unwind. */
void runGuestMain(uint32_t entryAddr, uint8_t* guestMemZero,
                  const XexImageInfo& img, std::atomic<bool>& stopRequested);

} // namespace fh2::ppc
