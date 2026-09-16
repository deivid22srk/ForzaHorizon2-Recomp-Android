// kernel_export_tables.cpp — tabelas COMPLETAS de exports dos módulos de
// sistema (xboxkrnl.exe: 922 ordinais; xam.xex: 1735 ordinais) e registro
// dos ordinais que o título resolve em runtime via XexGetProcedureAddress.
//
// Fonte REAL dos nomes/ordinais: as tabelas do XenonUtils (xbox/*.inc,
// derivadas do projeto Xenia — BSD). O título do FH2 resolve dinamicamente
// funções que NÃO importa estaticamente (ex.: XInputdFF* 642-649 — force
// feedback do gamepad; XamParty* 2815/2816/2827/2832 — presença em party;
// FileTimeToSystemTime 442). No console TODAS as ordinais resolvem — o
// runtime precisa devolver um endereço para cada consulta, senão o jogo
// entra em caminhos de erro (polling infinito de dispositivos ausentes).
//
// Os ordinais IMPORTADOS estaticamente continuam com os VAs reais dos
// stubs de import (despachados para o HLE pela tabela mágica). Os demais
// recebem STUBS SINTÉTICOS em 0x9F400000+ (faixa livre acima da região de
// módulos): chamadas desviam para o thunk fh2_guest_unmapped_code, que
// LOGA o nome da função (throttled) e retorna r3 = 0 — comportamento
// definido, visível no logcat, nunca um crash silencioso.
#include <android/log.h>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

#include "runtime/ppc/kernel_state.h"

namespace fh2::kern {

namespace {

struct ExpEntry {
    uint32_t ordinal;
    const char* name;
};

#define XE_EXPORT(MODULE, ORDINAL, NAME, TYPE) {(ORDINAL), #NAME}

ExpEntry g_kernelTable[] = {
#include "xbox/xboxkrnl_table.inc"
};
ExpEntry g_xamTable[] = {
#include "xbox/xam_table.inc"
};

#undef XE_EXPORT

// faixa sintética (fora da faixa de código gerado: o dispatch cai no thunk
// de diagnóstico — definido em kernel_real.cpp)
constexpr uint32_t kSyntheticKernelBase = 0x9F400000u;
constexpr uint32_t kSyntheticXamBase = 0x9F480000u;

std::unordered_map<uint32_t, std::string> g_stubNames; // VA → "módulo!nome"
std::mutex g_namesM;

void mergeTable(const char* moduleName, uint32_t syntheticBase,
                const ExpEntry* table, size_t count) {
    KernelModule* m = findModuleByName(moduleName);
    if (!m) {
        __android_log_print(ANDROID_LOG_WARN, "FH2/KRN",
                            "tabela completa: módulo '%s' ainda não "
                            "registrado — ignorado",
                            moduleName);
        return;
    }
    std::lock_guard<std::mutex> lk(g_namesM);
    uint32_t added = 0;
    for (size_t i = 0; i < count; ++i) {
        const uint32_t ord = table[i].ordinal;
        if (m->exports.find(ord) != m->exports.end()) continue;
        // 16 bytes por slot sintético — sem colisões entre ordinais
        const uint32_t va = syntheticBase + ord * 16;
        m->exports[ord] = va;
        g_stubNames.emplace(va,
                            std::string(moduleName) + "!" + table[i].name);
        ++added;
    }
    __android_log_print(
        ANDROID_LOG_INFO, "FH2/KRN",
        "tabela completa de %s: +%u ordinais sintéticos (total %zu)",
        moduleName, added, m->exports.size());
}

} // namespace

void registerFullExportTables() {
    mergeTable("xboxkrnl.exe", kSyntheticKernelBase, g_kernelTable,
               sizeof(g_kernelTable) / sizeof(g_kernelTable[0]));
    mergeTable("xam.xex", kSyntheticXamBase, g_xamTable,
               sizeof(g_xamTable) / sizeof(g_xamTable[0]));
}

const char* syntheticStubName(uint32_t va) {
    std::lock_guard<std::mutex> lk(g_namesM);
    auto it = g_stubNames.find(va);
    return it == g_stubNames.end() ? nullptr : it->second.c_str();
}

} // namespace fh2::kern
