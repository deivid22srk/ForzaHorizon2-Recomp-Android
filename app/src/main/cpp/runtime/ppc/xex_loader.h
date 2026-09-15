// xex_loader.h — carregamento da imagem XEX2 do jogo na memória guest
//
// Usa o MESMO pipeline da análise do recompilador (XenonUtils::Xex2LoadImage,
// vendido em tools/XenonRecomp/XenonUtils): decripta AES-128 com a chave
// retail do XEX (cabeçalho de segurança), descomprime (none/basic/normal-LZX),
// aplica o patch de imports (nop;nop;nop;blr) IDÊNTICO ao aplicado antes da
// geração de código — mantendo a memória guest consistente com o C++ gerado —
// e copia a imagem decodificada para guestMem + base.
//
// Nada simulado: cada falha real (arquivo ausente, magic inválido, decodificação
// mal-sucedida, imagem fora dos limites) retorna false com mensagem descritiva.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace fh2::fs {
class FsProvider;
}

namespace fh2::ppc {

struct XexImageInfo {
    uint32_t base = 0;        // endereço guest da imagem (FH2: 0x82000000)
    uint32_t entryPoint = 0;  // entry point (FH2: 0x82BF2CD0)
    uint32_t imageSize = 0;   // bytes da imagem decodificada
    uint32_t titleId = 0;     // XEX_HEADER_EXECUTION_INFO
    uint32_t defaultStackSize = 0; // XEX_HEADER_DEFAULT_STACK_SIZE
    uint32_t defaultHeapSize = 0;  // XEX_HEADER_DEFAULT_HEAP_SIZE
    // XEX_HEADER_TLS_INFO (copiado para cada thread guest; r13 = base)
    uint32_t tlsNumberOfSlots = 0;
    uint32_t tlsSlotSize = 0;
    uint32_t tlsBytes = 0;        // numberOfBytes (bloco por thread)
    uint32_t tlsDataStart = 0;    // VA do template de init (na imagem)
    uint32_t tlsRawDataEnd = 0;
    uint32_t tlsDataEnd = 0;
    uint32_t tlsIndexAddr = 0;
    uint32_t tlsBaseAddr = 0;     // VA da variável base do CRT
};

/**
 * Lê "default.xex" via FsProvider (leitura DIRETA — fd SAF, sem cópia),
 * decodifica e copia para a memória guest.
 *
 * @param guestMem       ponteiro host correspondente ao endereço guest 0
 * @param guestMemBytes  tamanho da reserva (validação de limites)
 * @param out            preenchido com base/entry/tamanho reais do XEX
 * @param error          motivo legível quando retornar false
 */
bool loadXexImage(fs::FsProvider& fs, uint8_t* guestMem, size_t guestMemBytes,
                  XexImageInfo& out, std::string& error);

} // namespace fh2::ppc
