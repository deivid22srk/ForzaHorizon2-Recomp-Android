// kernel_state.h — estado do "kernel" HLE do xboxkrnl (semântica REAL)
//
// Este é o núcleo da camada kernel/IO (issue #16): objetos de sincronização
// reais (event/semaphore/mutant com mutex+condvar do host), threads guest
// reais (uma std::thread do host por thread guest, com PPCContext próprio),
// TLS do kernel (KeTls*), tempo real do host, heap e janela de memória
// virtual reais (guest_alloc) e unwind seguro no stop (jmp_buf por thread).
//
// Este header NÃO depende de ppc_context.h: o glue com o PPCContext vive em
// kernel_real.cpp (implementações __imp__) e guest_entry.cpp (corpo das
// threads / entry). Assim tudo compila também sem código recompilado.
#pragma once

#include <atomic>
#include <condition_variable>
#include <csetjmp>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "runtime/ppc/guest_alloc.h"

namespace fh2::input { struct InputState; }
namespace fh2::fs { class FsProvider; }

namespace fh2::kern {

// ---------------------------------------------------------------- tipos

// Espécie de objeto sincronizável (chaveado por endereço guest do objeto
// passado aos Ke*/ou pelo HANDLE devolvido pelos Nt*).
enum class WaitKind : uint8_t { Event, Semaphore, Mutant };

struct Waitable {
    WaitKind kind = WaitKind::Event;
    bool signaled = false;      // event: estado; mutant: livre
    bool autoReset = false;     // SynchronizationEvent reseta ao acordar
    long count = 0;             // semáforo
    long maxCount = 1;
    uint32_t ownerThread = 0;   // mutant: handle do dono
    bool abandoned = false;
};

struct GuestThread;

struct GuestThread {
    uint32_t handle = 0;
    uint32_t id = 0;
    uint64_t startRoutine = 0;   // endereço guest da rotina
    uint64_t startContext = 0;   // parâmetro p/ r3
    uint64_t stackGuest = 0;     // base da stack (janela virtual)
    uint64_t stackSize = 0;
    uint64_t tlsGuest = 0;       // bloco TLS próprio (ou 0)
    std::thread host;
    std::mutex startM;
    std::condition_variable startCv;
    bool started = false;        // KeResumeThread libera
    std::atomic<uint32_t> suspendCount{1}; // criada SUSPENSA (kernel real)
    std::atomic<bool> finished{false};
    jmp_buf unwindEnv;           // alvo do longjmp no stop
    std::atomic<bool> envValid{false};
};

// ------------------------------------------------------------- acesso

/** Guarda o ponteiro host correspondente ao endereço guest 0 (memBase_). */
void setGuestMemory(uint8_t* guestMemZero, uint64_t guestMemBytes);
uint8_t* guestMem();                 // nullptr antes do boot
uint64_t guestMemBytes();

// ---- acesso BE à memória guest (para HLE fora de kernel_real.cpp) ----
bool guestPtrValid(uint64_t addr, uint64_t bytes);
uint32_t readGuestU32(uint64_t addr);
void writeGuestU32(uint64_t addr, uint32_t v);

/** Janelas de alocação (chamadas uma vez no boot pelo PpcRuntime). */
fh2::ppc::GuestHeap& heap();
fh2::ppc::GuestVirtWindow& virtWindow();
/** Janela FÍSICA do kernel (MmAllocatePhysicalMemoryEx) — endereços no
 *  alias físico do guest (0xA0000000+), espelho das mesmas páginas RAM. */
fh2::ppc::GuestVirtWindow& physWindow();

/** Input real (HUD + gamepads) para XamInputGetState. */
void setInputBridge(input::InputState* input);
input::InputState* inputBridge();

// ------------------------------------------------------------ threads

using ThreadBody = std::function<void(GuestThread&)>;

/** Registra o corpo real da thread (feito em guest_entry.cpp — precisa do
 *  PPCContext/magic table). Deve ser chamado antes de criar threads. */
void setThreadBody(ThreadBody body);

/** ExCreateThread: cria registro SUSPENSO e dispara a thread do host. */
uint32_t createThread(uint64_t stackSize, uint64_t startRoutine,
                      uint64_t startContext, uint32_t* outId);
/** NtResumeThread/KeResumeThread. */
bool resumeThread(uint32_t handle);

/** Suspende a thread (contagem real; a parada efetiva ocorre no próximo
 *  ponto de bloqueio do guest — suspensão preemptiva de código recompilado
 *  exigiria checkpoints por instrução). Retorna a contagem anterior (UINT32_MAX
 *  se handle desconhecido). */
uint32_t suspendThread(uint32_t handle);
/** Término do próprio thread (ExTerminateThread/HalReturnToFirmware). */
void terminateCurrentThread(uint32_t exitCode);

// --------------------------------- título: launch/terminate/relaunch
//
// Semântica real do XAM (o launcher do FH2 depende disto):
//   XamLoaderSetLaunchData(data,size)  — guarda os dados de launch
//   XamLoaderLaunchTitle(path,flags)   — NÃO RETORNA: o kernel termina o
//       título e o XAM o relança com launch data preservado
//   XamLoaderTerminateTitle()          — NÃO RETORNA: termina o título
//
// O relaunch reexecuta o guest do entry com estado zerado (threads, heap,
// waitables, TLS, arquivos), mantendo APENAS o launch data — o boot do
// título o lê via XamLoaderGetLaunchData (isso é o que dá o boot 2 do
// launcher do FH2: launcher → launch → jogo).

/** XamLoaderSetLaunchData: guarda até 512 bytes (tamanho real do título). */
void setLaunchData(const uint8_t* data, uint32_t size);
/** XamLoaderGetLaunchData: copia para out (até maxLen). Retorna o tamanho
 *  real (0 = sem dados — boot frio). */
uint32_t getLaunchData(uint8_t* out, uint32_t maxLen);
/** Tamanho atual do launch data (0 = nenhum). */
uint32_t getLaunchDataSize();

/** Pedido de relaunch com path/flags (XamLoaderLaunchTitle). O caminho é
 *  informativo (o título é sempre o mesmo XEX); flags ficam disponíveis
 *  para o log/estado. */
void requestTitleRelaunch(const std::string& path, uint32_t flags);
/** true se o título pediu relaunch e ainda não foi consumido. */
bool titleRelaunchRequested();
/** Consome o pedido de relaunch (o runtime reinicia o guest). */
bool consumeTitleRelaunch(std::string& pathOut, uint32_t& flagsOut);

/** Encerramento do título ativo (threads guest desligam nos próximos
 *  pontos de bloqueio; a thread chamadora sofre unwind imediato). */
void terminateTitle();
/** true após terminateTitle até o reset do estado. */
bool titleTerminated();

/** Reset do estado do título p/ relaunch REAL: threads, waitables, handles
 *  Nt*, arquivos, TLS do kernel, heap/janelas de alocação e flags de
 *  encerramento — PRESERVA o launch data. O guest re-executa o CRT do
 *  zero, então nada do boot anterior pode sobreviver. */
void resetTitleState();

/** Pedido global de parada: acorda todos os waits e dispara unwind. */
void requestStop();
std::vector<std::shared_ptr<GuestThread>> allThreads();
void forEachThread(const std::function<void(GuestThread&)>& fn);
GuestThread* findThread(uint32_t handle);
/** Espera todas as threads guest terminarem (timeout ms). Retorna o
 *  número de threads que continuam vivas. */
size_t joinAll(long timeoutMs);

// ----------------------------------------------------------- waitables

/** Obtém (criando se `create`) o estado do objeto em `key` — para Ke*, a
 *  key é o ENDEREÇO guest do objeto; para Nt*, o HANDLE marcado. */
Waitable* waitable(uint64_t key, bool create, WaitKind kind);
/** Resolve HANDLE Nt* → Waitable (nullptr se inválido). */
Waitable* waitableByHandle(uint32_t handle);
/** Registra handle Nt* → waitable recém-criado. Retorna o handle. */
uint32_t registerHandle(Waitable* w);
/** NtClose: remove handle (o objeto por endereço permanece p/ Ke*). */
void closeHandle(uint32_t handle);

/** Espera REAL em um conjunto de waitables. timeoutNs: negativo =
 *  relativo (convertido), 0 = sem espera, positivo = absoluto (tratado
 *  como relativo com log). Retorna índice do sinalizado ou WAIT_TIMEOUT. */
constexpr uint32_t kWaitSignaled0 = 0;
constexpr uint32_t kWaitAbandoned0 = 64;   // base p/ abandoned
constexpr uint32_t kWaitTimeout = 0x00000102; // STATUS_TIMEOUT
uint32_t waitForMultiple(Waitable** objs, size_t count, bool waitAny,
                         int64_t timeoutNs, bool alertable);
/** Sinaliza event (retorna estado anterior). */
long setEvent(Waitable* w);
/** Reseta event (retorna estado anterior). */
long resetEvent(Waitable* w);
/** Libera semáforo/mutant. */
long releaseSemaphore(Waitable* w, long count);
long releaseMutant(Waitable* w, bool& abandoned);
/** Acorda TUDO (stop) — waits retornam WAIT_TIMEOUT imediatamente. */
void wakeAll();
/** Mutex global dos waitables (para testes atômicos fora de waitForMultiple). */
std::mutex& waitMutex();
/** CV global do kernel — par do waitMutex (waits customizados, ex. critsecs). */
std::condition_variable& waitCv();

// ------------------------------------------------- identidade da thread

/** ID de thread do kernel da thread host atual. Estável por thread: guest
 *  threads usam o mesmo id gravado no TEB (ClientId.UniqueThread, +0x24),
 *  de modo que comparações OwningThread == TEB->ClientId.UniqueThread do
 *  guest funcionam exatamente como no kernel real. */
uint32_t currentThreadId();
/** Sobrescreve o id da thread atual (guest threads chamam com t->id). */
void setHostThreadId(uint32_t id);

// --------------------------------------------------------- title id

/** ID do título (lido do XEX_HEADER_EXECUTION_INFO no boot). */
void setTitleId(uint32_t id);
uint32_t currentTitleId();

// ------------------------------------------- info do módulo XEX carregado

/** Base da imagem + privilégios do EXECUTION_INFO (fixados no boot) —
 *  semântica real para XexGetModuleHandle/XexGetModuleSection/
 *  XexCheckExecutablePrivilege. */
void setImageInfo(uint32_t base, uint64_t privileges);
uint32_t imageBase();
uint64_t imagePrivileges();

/** Recursos nomeados do XEX (XEX_HEADER_RESOURCE_INFO) — expostos via
 *  XexGetModuleSection (semântica real do kernel do 360). */
struct KernelResource {
    char id[9];
    uint32_t va;
    uint32_t size;
};
void setImageResources(const KernelResource* res, size_t count);
const KernelResource* findImageResource(const char* id);

// ------------------------------------------- módulos XEX carregados
//
// No console, xam.xex/xboxkrnl.exe são SEMPRE carregados. O runtime os
// representa como módulos VIRTUAIS: as exports que o título importa têm
// stubs reais na imagem (nop;nop;nop;blr — resolvidos pela tabela mágica
// p/ os __imp__ HLE). XexGetProcedureAddress devolve o VA do stub; chamar
// esse VA executa o HLE — exatamente como o console devolve o endereço da
// função real e o título chama indiretamente.
struct KernelModule {
    std::string name;                 // "xam.xex", "xboxkrnl.exe", "default.xex"
    uint32_t handle = 0;              // handle opaco estável
    uint32_t imageBase = 0;           // 0 p/ módulos virtuais sem imagem
    std::map<uint32_t, uint32_t> exports; // ordinal → VA do stub (guest)
};
/** Registra/atualiza um módulo (chamado no boot com os imports decodados
 *  do XEX). handle 0 → aloca. Retorna o handle. */
uint32_t registerModule(const std::string& name, uint32_t handle,
                        std::map<uint32_t, uint32_t> exports);
/** Busca por nome (case-insensitive, sem caminho — "xam.xex"). */
KernelModule* findModuleByName(const std::string& name);
/** Busca por handle (nullptr se inválido). */
KernelModule* findModuleByHandle(uint32_t handle);
/** Handle do executável do título (0x82000000). */
uint32_t executableModuleHandle();

// ---------------------------------------------------- arquivos abertos

/** Objeto de arquivo aberto (equivalente do FILE_OBJECT do kernel real). */
struct GuestFile {
    std::string path;      // caminho guest normalizado (game:/… → relativo)
    std::string display;   // caminho original p/ log
    int fd = -1;           // fd host (SAF direto ou local) — pread real
    uint64_t size = 0;     // tamanho real do arquivo
    uint64_t pos = 0;      // ponteiro lógico (uso quando ByteOffset=NULL)
    bool write = false;    // aberto para escrita (só paths locais)
};

/** Abre (ou falha REAL se o arquivo não existir). Handle 0 = erro. */
uint32_t fileOpen(const std::string& guestPath, const std::string& display,
                  bool write, GuestFile** out);
/** Resolve handle → objeto (nullptr se inválido). */
GuestFile* fileGet(uint32_t handle);
/** Fecha o fd e remove o handle. */
void fileClose(uint32_t handle);
/** Fecha tudo (stop do runtime). */
void filesCloseAll();
/** Registra o FsProvider usado por fileOpen (chamado no boot). */
void setFsBridge(fs::FsProvider* fs);

// ---------------------------------------------------------------- TLS

// TLS do KERNEL (KeTlsAlloc/GetValue/SetValue): 64 slots por processo.
uint32_t tlsAlloc();
void tlsFree(uint32_t slot);
void* tlsGetValue(uint32_t slot);
bool tlsSetValue(uint32_t slot, void* value);

// ---------------------------------------------------------------- tempo

/** Tempo do sistema (100 ns desde 1601-01-01) — real, CLOCK_REALTIME. */
uint64_t systemTime100ns();
/** Contador de performance (50 MHz, clock monotônico do host). */
uint64_t perfCounter50MHz();

// --------------------------------------------------------------- unwind

/** Marca a thread atual como guest e registra o alvo do unwind. */
void setUnwindTarget(jmp_buf* env);
jmp_buf* unwindTarget();
/** true se um stop foi pedido (waits verificam e dão longjmp). */
bool stopRequested();

// ------------------------------------------------------ strings guest

/** Lê C string do espaço guest com limite de segurança. */
std::string readGuestCString(uint64_t addr, size_t maxLen = 4096);
/** Lê UTF-16LE/BE? (Xenon: big-endian) string wide do guest. */
std::string readGuestWString(uint64_t addr, size_t maxLen = 4096);

// ------------------------------------------------- varargs do guest

/** Iterador de varargs da ABI Xenon (PPC64): inteiros em r3..r10 depois
 *  stack; doubles em paralelo em f1..f13. O glue do PPCContext é passado
 *  via accessors (kernel_real.cpp). */
struct GuestArgs {
    std::function<uint64_t(int idx)> nextInt;   // idx lógico 0.. (0 = r3 p/ fmt já consumido fora)
    std::function<double(int idx)> nextDouble;
};

/** printf real sobre varargs do guest. Escreve em `dstHost` (até dstBytes).
 *  Retorna número de bytes escritos (como _snprintf truncado). */
int guestFormat(char* dstHost, size_t dstBytes, const std::string& fmt,
                GuestArgs& args);
/** Variante wide (UTF-16 big-endian no guest, 2 bytes/char). */
int guestFormatW(uint16_t* dstHost, size_t dstChars, const std::string& fmt,
                 GuestArgs& args);

// ------------------------------------------------- física/memória misc

struct MemStats {
    uint64_t heapUsed = 0, heapSize = 0;
    uint64_t virtUsed = 0, virtSize = 0;
};
MemStats memStats();

} // namespace fh2::kern
