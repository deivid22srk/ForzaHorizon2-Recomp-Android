// kernel_state.cpp — implementação do estado do kernel HLE (semântica real)
//
// Toda a concorrência do kernel vive aqui: registry de threads guest, mapas
// de objetos sincronizáveis, TLS do kernel, tempo do host e o pedido de
// stop global (com acordas de emergência nos waits).
#include "kernel_state.h"

#include <algorithm>
#include <android/log.h>
#include <chrono>

#include <chrono>
#include <cstring>
#include <ctime>

#define KLOG(...) __android_log_print(ANDROID_LOG_INFO, "FH2/KERN", __VA_ARGS__)

namespace fh2::kern {

namespace {

std::mutex g_stateM;
uint8_t* g_guestMem = nullptr;
uint64_t g_guestMemBytes = 0;

fh2::ppc::GuestHeap g_heap;
fh2::ppc::GuestVirtWindow g_virt;
fh2::ppc::GuestVirtWindow g_phys;

input::InputState* g_input = nullptr;

// --- threads ---
ThreadBody g_threadBody;
std::map<uint32_t, std::shared_ptr<GuestThread>> g_threads;
uint32_t g_nextThreadHandle = 0x90000000;

// --- waitables ---
struct WaitEntry {
    std::unique_ptr<Waitable> w;
    uint64_t key; // handle (Nt*) ou endereço guest (Ke*)
};
std::map<uint32_t, WaitEntry> g_handles;      // Nt handles → waitable
std::map<uint64_t, WaitEntry> g_keyed;        // endereço guest → waitable
uint32_t g_nextHandle = 0xA0000000;

// --- TLS do kernel ---
void* g_tls[64] = {};
bool g_tlsUsed[64] = {};

// --- módulos XEX carregados (virtuais p/ xam.xex/xboxkrnl.exe) ---
std::map<std::string, KernelModule> g_modules; // nome (lowercase) → módulo
// faixas guest reservadas por módulos secundários (addr → size)
static std::map<uint64_t, uint64_t> g_moduleRanges;
uint32_t g_nextModuleHandle = 0x8F000001;

// --- identidade de thread (kernel) ---
std::atomic<uint32_t> g_nextKernelThreadId{2}; // 1 = thread principal
thread_local uint32_t t_kernelThreadId = 0; // 0 = ainda não atribuído

// --- stop ---
std::atomic<bool> g_stop{false};

// --- título: launch/terminate/relaunch (semântica real do XAM) ---
std::mutex g_launchM;
std::vector<uint8_t> g_launchData;            // XamLoaderSetLaunchData
std::string g_launchPath;                     // XamLoaderLaunchTitle
uint32_t g_launchFlags = 0;
std::atomic<bool> g_relaunchRequested{false};
std::atomic<bool> g_titleTerminated{false};

// Um ÚNICO mutex+cv para TODOS os waitables: elimina classes inteiras de
// deadlock (ordem de locks) e de wakeup perdido (wait-all em cvs distintos).
// O boot do jogo não é sensível a essa contenção — correção primeiro.
std::mutex g_waitM;
std::condition_variable g_waitCv;

thread_local jmp_buf* t_unwind = nullptr;

inline uint64_t nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

} // namespace

// ------------------------------------------------------------- acesso

void setGuestMemory(uint8_t* guestMemZero, uint64_t guestMemBytes) {
    std::lock_guard<std::mutex> lk(g_stateM);
    g_guestMem = guestMemZero;
    g_guestMemBytes = guestMemBytes;
}
uint8_t* guestMem() { return g_guestMem; }
uint64_t guestMemBytes() { return g_guestMemBytes; }

bool guestPtrValid(uint64_t addr, uint64_t bytes) {
    // mask 32-bit: registradores 64-bit do guest carregam extensão de sinal
    // (0xFFFFFFFF8xxxxxxx) que o endereçamento real do Xenon ignora.
    const uint64_t a32 = (uint32_t)addr;
    return g_guestMem && a32 != 0 && a32 + bytes <= g_guestMemBytes;
}
uint32_t readGuestU32(uint64_t addr) {
    if (!guestPtrValid(addr, 4)) return 0;
    return __builtin_bswap32(*(uint32_t*)(g_guestMem + addr));
}
void writeGuestU32(uint64_t addr, uint32_t v) {
    if (!guestPtrValid(addr, 4)) return;
    *(uint32_t*)(g_guestMem + addr) = __builtin_bswap32(v);
}

fh2::ppc::GuestHeap& heap() { return g_heap; }
fh2::ppc::GuestVirtWindow& virtWindow() { return g_virt; }
fh2::ppc::GuestVirtWindow& physWindow() { return g_phys; }

void setInputBridge(input::InputState* input) { g_input = input; }
input::InputState* inputBridge() { return g_input; }

// ------------------------------------------------------------ threads

void setThreadBody(ThreadBody body) { g_threadBody = std::move(body); }

uint32_t createThread(uint64_t stackSize, uint64_t startRoutine,
                      uint64_t startContext, uint32_t* outId) {
    auto t = std::make_shared<GuestThread>();
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        t->handle = g_nextThreadHandle++;
        // ID do kernel compartilhado com currentThreadId(): sem colisão com a
        // thread principal (1) nem com threads host efêmeras.
        t->id = g_nextKernelThreadId.fetch_add(1);
        g_threads[t->handle] = t;
    }
    t->startRoutine = startRoutine;
    t->startContext = startContext;
    t->stackSize = stackSize;
    if (outId) *outId = t->id;

    std::weak_ptr<GuestThread> weak = t;
    t->host = std::thread([weak] {
        auto t = weak.lock();
        if (!t) return;
        if (g_threadBody) {
            g_threadBody(*t);
        } else {
            KLOG("thread %u criada sem corpo (setThreadBody ausente) — "
                 "encerrando", t->handle);
        }
        t->finished = true;
    });
    return t->handle;
}

bool resumeThread(uint32_t handle) {
    std::shared_ptr<GuestThread> t;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        auto it = g_threads.find(handle);
        if (it == g_threads.end()) return false;
        t = it->second;
    }
    // contagem de suspensão REAL: só inicia quando chega a 0
    const uint32_t prev = t->suspendCount.fetch_sub(1);
    if (prev > 1) return true; // ainda suspenso (N níveis)
    {
        std::lock_guard<std::mutex> lk(t->startM);
        t->started = true;
    }
    t->startCv.notify_all();
    return true;
}

uint32_t suspendThread(uint32_t handle) {
    std::shared_ptr<GuestThread> t;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        auto it = g_threads.find(handle);
        if (it == g_threads.end()) return 0xFFFFFFFFu;
        t = it->second;
    }
    return t->suspendCount.fetch_add(1);
}

GuestThread* findThread(uint32_t handle) {
    std::lock_guard<std::mutex> lk(g_stateM);
    auto it = g_threads.find(handle);
    return it == g_threads.end() ? nullptr : it->second.get();
}

void forEachThread(const std::function<void(GuestThread&)>& fn) {
    std::vector<std::shared_ptr<GuestThread>> snapshot;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        for (auto& [h, t] : g_threads) snapshot.push_back(t);
    }
    for (auto& t : snapshot) fn(*t);
}

std::vector<std::shared_ptr<GuestThread>> allThreads() {
    std::vector<std::shared_ptr<GuestThread>> snapshot;
    std::lock_guard<std::mutex> lk(g_stateM);
    for (auto& [h, t] : g_threads) snapshot.push_back(t);
    return snapshot;
}

void terminateCurrentThread(uint32_t exitCode) {
    KLOG("ExTerminateThread exit=%u", exitCode);
    // A thread do host sai do corpo (guest_entry longjmp p/ saída limpa).
    jmp_buf* env = t_unwind;
    if (env) longjmp(*env, 2); // 2 = thread terminou explicitamente
    // Sem env (não-guest): encerra a std::thread implicitamente ao retornar.
}

void requestStop() {
    g_stop = true;
    wakeAll();
    forEachThread([](GuestThread& t) {
        t.startCv.notify_all();
        t.startM.lock(); t.startM.unlock();
    });
}

bool stopRequested() { return g_stop; }

jmp_buf* unwindTarget() { return t_unwind; }
void setUnwindTarget(jmp_buf* env) { t_unwind = env; }

size_t joinAll(long timeoutMs) {
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeoutMs);
    size_t alive = 0;
    for (;;) {
        alive = 0;
        std::vector<std::shared_ptr<GuestThread>> snapshot = allThreads();
        for (auto& t : snapshot) {
            if (t->finished) {
                if (t->host.joinable()) t->host.join();
            } else {
                ++alive;
            }
        }
        if (alive == 0 ||
            std::chrono::steady_clock::now() > deadline) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return alive;
}

// ----------------------------------------------------------- waitables

Waitable* waitable(uint64_t key, bool create, WaitKind kind) {
    std::lock_guard<std::mutex> lk(g_stateM);
    auto it = g_keyed.find(key);
    if (it != g_keyed.end()) return it->second.w.get();
    if (!create) return nullptr;
    WaitEntry e;
    e.key = key;
    e.w = std::make_unique<Waitable>();
    e.w->kind = kind;
    g_keyed[key] = std::move(e);
    return g_keyed[key].w.get();
}

Waitable* waitableByHandle(uint32_t handle) {
    std::lock_guard<std::mutex> lk(g_stateM);
    auto it = g_handles.find(handle);
    return it == g_handles.end() ? nullptr : it->second.w.get();
}

uint32_t registerHandle(Waitable* w) {
    std::lock_guard<std::mutex> lk(g_stateM);
    const uint32_t h = g_nextHandle++;
    WaitEntry e;
    e.key = h;
    e.w = std::unique_ptr<Waitable>(w); // handle assume a propriedade
    g_handles[h] = std::move(e);
    return h;
}

void closeHandle(uint32_t handle) {
    std::lock_guard<std::mutex> lk(g_stateM);
    auto it = g_handles.find(handle);
    if (it == g_handles.end()) return;
    // objeto por endereço (Ke*) continua vivo; handles Nt* soltam a única ref
    g_handles.erase(it);
    g_waitCv.notify_all();
}

long setEvent(Waitable* w) {
    if (!w) return 0;
    std::lock_guard<std::mutex> lk(g_waitM);
    const long prev = w->signaled ? 1 : 0;
    w->signaled = true;
    g_waitCv.notify_all();
    return prev;
}

long resetEvent(Waitable* w) {
    if (!w) return 0;
    std::lock_guard<std::mutex> lk(g_waitM);
    const long prev = w->signaled ? 1 : 0;
    w->signaled = false;
    return prev;
}

long releaseSemaphore(Waitable* w, long count) {
    if (!w) return 0;
    std::lock_guard<std::mutex> lk(g_waitM);
    const long prev = w->count;
    w->count = std::min(w->count + count, w->maxCount);
    // créditos disponíveis sinalizam o waitable (semântica real do NT: um
    // semáforo é "signaled" enquanto count > 0; o consume decrementa)
    w->signaled = w->count > 0;
    g_waitCv.notify_all();
    return prev;
}

bool setTimerDue(Waitable* w, int64_t due100ns) {
    if (!w) return false;
    std::lock_guard<std::mutex> lk(g_waitM);
    const bool wasPending = w->dueAbs100ns != 0;
    if (due100ns < 0) {
        // relativo: agora + (-due) — mesmo relógio do waitForMultiple
        w->dueAbs100ns = systemTime100ns() + (uint64_t)(-(due100ns + 1)) + 1;
    } else {
        w->dueAbs100ns = (uint64_t)due100ns;
    }
    w->signaled = false;
    g_waitCv.notify_all();
    return wasPending;
}

bool cancelTimer(Waitable* w) {
    if (!w) return false;
    std::lock_guard<std::mutex> lk(g_waitM);
    const bool wasPending = w->dueAbs100ns != 0;
    w->dueAbs100ns = 0;
    w->signaled = false;
    g_waitCv.notify_all();
    return wasPending;
}

long releaseMutant(Waitable* w, bool& abandoned) {
    if (!w) return 0;
    std::lock_guard<std::mutex> lk(g_waitM);
    abandoned = w->abandoned;
    w->abandoned = false;
    w->signaled = true; // livre
    w->ownerThread = 0;
    g_waitCv.notify_all();
    return 0;
}

std::mutex& waitMutex() { return g_waitM; }
std::condition_variable& waitCv() { return g_waitCv; }

uint32_t currentThreadId() {
    uint32_t id = t_kernelThreadId;
    if (id == 0) {
        // thread principal = 1; demais recebem ids sequenciais do kernel
        id = g_nextKernelThreadId.fetch_add(1);
        t_kernelThreadId = id;
    }
    return id;
}

void setHostThreadId(uint32_t id) { t_kernelThreadId = id; }

void wakeAll() {
    g_waitCv.notify_all();
    forEachThread([](GuestThread& t) {
        t.startCv.notify_all();
    });
}

uint32_t waitForMultiple(Waitable** objs, size_t count, bool waitAny,
                         int64_t timeout, bool alertable) {
    // timeout em unidades de 100 ns (PLARGE_INTEGER NT real):
    //   kNoTimeout (INT64_MIN) = espera INFINITA (NULL do guest)
    //   0                      = teste imediato (poll)
    //   negativo               = RELATIVO (-x = x×100ns a partir de agora)
    //   positivo               = ABSOLUTO (epoch 1601-01-01, 100ns) — o mesmo
    //                            relógio de systemTime100ns(); convertemos p/
    //                            relativo com o relógio do host REAL
    // (void)alertable: entrega de APC user-mode ainda não modelada; waits
    // alertáveis retornam pelos mesmos critérios dos não-alertáveis.
    if (count == 0) return kWaitTimeout;

    std::unique_lock<std::mutex> lk(g_waitM);
    auto pred = [&] {
        for (size_t i = 0; i < count; ++i) {
            Waitable& w = *objs[i];
            // timer vencido sinaliza LAZILY (one-shot) — sem thread de clock;
            // wait_until acorda no vencimento (deadline combinado abaixo)
            if (w.kind == WaitKind::Timer && w.dueAbs100ns != 0 && !w.signaled &&
                systemTime100ns() >= w.dueAbs100ns) {
                w.signaled = true;
                w.dueAbs100ns = 0;
                g_waitCv.notify_all();
            }
            if (w.signaled) return true;
        }
        return false;
    };
    auto consume = [&]() -> uint32_t {
        uint32_t firstSignaled = kWaitTimeout;
        for (size_t i = 0; i < count; ++i) {
            Waitable& w = *objs[i];
            if (!w.signaled) continue;
            if (waitAny) {
                if (w.autoReset) w.signaled = false;
                // semáforo: consumir um crédito a cada wait bem-sucedido
                if (w.kind == WaitKind::Semaphore && w.count > 0) {
                    --w.count;
                    w.signaled = w.count > 0;
                }
                return uint32_t(i) + (w.abandoned ? kWaitAbandoned0
                                                  : kWaitSignaled0);
            }
            if (firstSignaled == kWaitTimeout) firstSignaled = kWaitSignaled0;
            if (w.autoReset) w.signaled = false;
            if (w.kind == WaitKind::Semaphore && w.count > 0) {
                --w.count;
                w.signaled = w.count > 0;
            }
        }
        return firstSignaled;
    };
    // vencimento do timer agendado mais cedo (recalculado a cada iteração):
    // dá ao wait infinito um deadline real para timers, sem thread de clock
    auto timerDeadline = [&]() -> std::chrono::steady_clock::time_point {
        std::chrono::steady_clock::time_point dl;
        bool have = false;
        const auto now = std::chrono::steady_clock::now();
        for (size_t i = 0; i < count; ++i) {
            const Waitable& w = *objs[i];
            if (w.kind != WaitKind::Timer || w.dueAbs100ns == 0) continue;
            const int64_t rel100 = (int64_t)w.dueAbs100ns -
                                   (int64_t)systemTime100ns();
            // clamp: rel100×100 ns não pode estourar int64 (5e15×100ns = ~14h
            // — timers de jogo ficam MUITO abaixo disso)
            const int64_t ns = rel100 <= 0 ? 0
                                           : (rel100 > (int64_t)5e15
                                                  ? (int64_t)5e15
                                                  : rel100 * 100ll);
            const auto d = now + std::chrono::nanoseconds(ns);
            if (!have || d < dl) {
                dl = d;
                have = true;
            }
        }
        return have ? dl : std::chrono::steady_clock::time_point::max();
    };

    if (timeout == kNoTimeout) {
        // INFINITO de verdade: bloqueia até sinal externo (NtSetEvent,
        // ReleaseSemaphore/Mutant, closeHandle, wakeAll/stop) — sem deadline.
        // Era AQUI que o worker do FH2 girava: o antigo sentinel -1 virava
        // deadline de 1ns → 0x102 instantâneo → spin de milhões de iterações.
        // Timers agendados inserem um deadline REAL (sem thread de clock).
        for (;;) {
            const auto dl = timerDeadline();
            if (dl == std::chrono::steady_clock::time_point::max()) {
                g_waitCv.wait(lk, [&] { return pred() || g_stop; });
            } else {
                g_waitCv.wait_until(lk, dl, [&] { return pred() || g_stop; });
            }
            if (pred()) return consume();
            if (g_stop) {
                // stop: UNWIND da thread guest (padrão KeDelay/critsec) — sem
                // isso threads em loops de espera sobrevivem ao joinAll e
                // segfaultam no teardown (faults pós-run no log host 16_09).
                lk.unlock();
                jmp_buf* env = unwindTarget();
                if (env) longjmp(*env, 1);
                return kWaitTimeout; // não-guest: timeout p/ caller decidir
            }
            // timer reagendado no futuro durante o wait: loop recalcuma o
            // deadline (timerDeadline() é recalculado a cada iteração)
        }
    }
    if (timeout == 0) {
        if (g_stop && !pred()) {
            lk.unlock();
            jmp_buf* env = unwindTarget();
            if (env) longjmp(*env, 1);
        }
        return pred() ? consume() : kWaitTimeout;
    }

    int64_t ns;
    if (timeout < 0) {
        ns = -(timeout + 1) * 100ll + 100ll; // relativo: ×100 → ns (sem overflow)
    } else {
        // absoluto epoch 1601 (100ns) → restante relativo
        const int64_t now100 = (int64_t)systemTime100ns();
        if (timeout <= now100) return pred() ? consume() : kWaitTimeout;
        ns = (timeout - now100) * 100ll;
    }
    if (ns <= 0) return pred() ? consume() : kWaitTimeout;
    const auto userDeadline = std::chrono::steady_clock::now() +
                              std::chrono::nanoseconds(ns);
    for (;;) {
        // deadline combinado: timeout do usuário OU vencimento de timer
        const auto tdl = timerDeadline();
        const auto deadline = std::min(userDeadline, tdl);
        g_waitCv.wait_until(lk, deadline, [&] { return pred() || g_stop; });
        if (pred()) return consume();
        if (g_stop) {
            // stop durante espera com timeout: unwind (mesma semântica)
            lk.unlock();
            jmp_buf* env = unwindTarget();
            if (env) longjmp(*env, 1);
        }
        if (std::chrono::steady_clock::now() >= userDeadline)
            return kWaitTimeout;
        // acordou no vencimento de timer sem sinalizar? (relógio adiante) —
        // o loop recalcula e continua esperando pelo timeout do usuário
    }
}

// --------------------------------------------------------- title id

uint32_t g_titleId = 0;
void setTitleId(uint32_t id) { g_titleId = id; }
uint32_t currentTitleId() { return g_titleId; }

// info do módulo carregado (base + privilégios do EXECUTION_INFO) — usados
// por XexGetModuleHandle/Section/CheckExecutablePrivilege (semântica real)
uint32_t g_imageBase = 0;
uint64_t g_imagePrivileges = 0;
void setImageInfo(uint32_t base, uint64_t privileges) {
    g_imageBase = base;
    g_imagePrivileges = privileges;
}
uint32_t imageBase() { return g_imageBase; }
uint64_t imagePrivileges() { return g_imagePrivileges; }

std::vector<KernelResource> g_imageResources;
void setImageResources(const KernelResource* res, size_t count) {
    g_imageResources.assign(res, res + count);
}
const KernelResource* findImageResource(const char* id) {
    for (const auto& r : g_imageResources) {
        if (strncmp(r.id, id, 8) == 0) return &r;
    }
    return nullptr;
}

// --------------------------------------- título: launch/terminate/relaunch

void setLaunchData(const uint8_t* data, uint32_t size) {
    std::lock_guard<std::mutex> lk(g_launchM);
    if (!data || size == 0) {
        g_launchData.clear();
        return;
    }
    if (size > 512) size = 512; // XAM_LAUNCH_DATA real: 512 bytes
    g_launchData.assign(data, data + size);
}

uint32_t getLaunchData(uint8_t* out, uint32_t maxLen) {
    std::lock_guard<std::mutex> lk(g_launchM);
    if (g_launchData.empty()) return 0;
    const uint32_t n = (uint32_t)g_launchData.size();
    if (out && maxLen) {
        const uint32_t copy = n < maxLen ? n : maxLen;
        memcpy(out, g_launchData.data(), copy);
    }
    return n;
}

uint32_t getLaunchDataSize() {
    std::lock_guard<std::mutex> lk(g_launchM);
    return (uint32_t)g_launchData.size();
}

void requestTitleRelaunch(const std::string& path, uint32_t flags) {
    {
        std::lock_guard<std::mutex> lk(g_launchM);
        g_launchPath = path;
        g_launchFlags = flags;
    }
    g_relaunchRequested = true;
    // Encerra o título: as threads guest desligam nos próximos pontos de
    // bloqueio (o mesmo mecanismo do stop — ver terminateTitle()).
    terminateTitle();
}

bool titleRelaunchRequested() { return g_relaunchRequested; }

bool consumeTitleRelaunch(std::string& pathOut, uint32_t& flagsOut) {
    if (!g_relaunchRequested.exchange(false)) return false;
    std::lock_guard<std::mutex> lk(g_launchM);
    pathOut = g_launchPath;
    flagsOut = g_launchFlags;
    return true;
}

void terminateTitle() {
    // Encerramento REAL: mesmo mecanismo do stop (g_stop desliga os waits e
    // dispara o unwind em cada thread guest). A diferença é que o runtime
    // (PpcRuntime::run) trata o encerramento como fim DO TÍTULO e não do app:
    // se há relaunch pendente, o guest re-executa com estado zerado.
    g_titleTerminated = true;
    requestStop();
}

bool titleTerminated() { return g_titleTerminated; }

void resetTitleState() {
    // Threads DEVEM estar finalizadas (joinAll) antes deste ponto.
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        g_threads.clear();
        g_handles.clear();
        g_keyed.clear();
        g_nextHandle = 0xA0000000;
        g_nextThreadHandle = 0x90000000;
        memset(g_tls, 0, sizeof(g_tls));
        memset(g_tlsUsed, 0, sizeof(g_tlsUsed));
        g_nextKernelThreadId = 2; // 1 = thread principal (do próximo boot)
    }
    filesCloseAll();
    g_heap.reset();
    g_virt.reset();
    g_phys.reset();
    // Vídeo (Vd*): o novo boot reconfigura TUDO via Vd* — inclui o buffer de
    // sistema (a janela virtual foi resetada, os endereços antigos morreram).
    vdGraphics() = VdGraphicsState{};
    g_stop = false;
    g_titleTerminated = false;
    // launch data: PRESERVADO (o relaunch o consome). Módulos secundários:
    // DESCARREGADOS como no console (o novo boot re-executa XexLoadImage e
    // recarrega do storage — a imagem anterior pode ter sido sobrescrita
    // pelas alocações do boot que terminou).
    releaseSecondaryModules();
}

// ------------------------------------------------- módulos XEX carregados

uint32_t registerModule(const std::string& name, uint32_t handle,
                        std::map<uint32_t, uint32_t> exports) {
    std::string key;
    key.reserve(name.size());
    for (char c : name) {
        c = (char)tolower((unsigned char)c);
        const size_t slash = key.find('/') ; // nada — normalizamos abaixo
        (void)slash;
        // mantém apenas o nome de arquivo (sem "game:\", "device\image\")
        key.push_back(c);
    }
    const size_t bs = key.find_last_of("\\/:");
    if (bs != std::string::npos) key = key.substr(bs + 1);
    std::lock_guard<std::mutex> lk(g_stateM);
    KernelModule& m = g_modules[key];
    if (m.name.empty()) m.name = key;
    if (handle != 0 && m.handle == 0) m.handle = handle;
    if (m.handle == 0) m.handle = g_nextModuleHandle++;
    if (!exports.empty()) {
        // mescla: imports do boot + possíveis registros incrementais
        for (auto& [ord, va] : exports) m.exports[ord] = va;
    }
    return m.handle;
}

KernelModule* findModuleByName(const std::string& name) {
    std::string key;
    key.reserve(name.size());
    for (char c : name) key.push_back((char)tolower((unsigned char)c));
    const size_t bs = key.find_last_of("\\/:");
    if (bs != std::string::npos) key = key.substr(bs + 1);
    std::lock_guard<std::mutex> lk(g_stateM);
    auto it = g_modules.find(key);
    return it == g_modules.end() ? nullptr : &it->second;
}

KernelModule* findModuleByHandle(uint32_t handle) {
    if (handle == 0) return nullptr;
    std::lock_guard<std::mutex> lk(g_stateM);
    for (auto& [key, m] : g_modules) {
        if (m.handle == handle) return &m;
    }
    return nullptr;
}

bool moduleRangeFree(uint64_t addr, uint64_t size) {
    if (size == 0) return false;
    std::lock_guard<std::mutex> lk(g_stateM);
    for (const auto& [b, s] : g_moduleRanges) {
        if (addr < b + s && b < addr + size) return false;
    }
    return true;
}

bool reserveModuleRange(uint64_t addr, uint64_t size) {
    if (size == 0) return false;
    std::lock_guard<std::mutex> lk(g_stateM);
    for (const auto& [b, s] : g_moduleRanges) {
        if (addr < b + s && b < addr + size) return false;
    }
    g_moduleRanges[addr] = size;
    return true;
}

void releaseSecondaryModules() {
    std::lock_guard<std::mutex> lk(g_stateM);
    uint8_t* ram = guestMem();
    const uint64_t ramBytes = guestMemBytes();
    for (auto it = g_modules.begin(); it != g_modules.end();) {
        if (it->second.secondary) {
            // zero REAL da memória do módulo — nenhum dado do boot anterior
            // sobrevive (o console descarrega a imagem ao terminar o título)
            if (ram && it->second.imageGuest && it->second.imageSpan &&
                it->second.imageGuest + it->second.imageSpan <= ramBytes) {
                memset(ram + it->second.imageGuest, 0, it->second.imageSpan);
            }
            it = g_modules.erase(it);
        } else {
            ++it;
        }
    }
    g_moduleRanges.clear();
}

void markModuleSecondary(uint32_t handle, uint32_t imageGuest,
                         uint32_t imageSpan) {
    std::lock_guard<std::mutex> lk(g_stateM);
    for (auto& [key, m] : g_modules) {
        if (m.handle == handle) {
            m.secondary = true;
            m.imageGuest = imageGuest;
            m.imageSpan = imageSpan;
            return;
        }
    }
}

uint32_t executableModuleHandle() { return g_imageBase; }


} // namespace fh2::kern

// ------- arquivos abertos (fora do namespace p/ includes de std) -------

#include "runtime/fs/fs_provider.h"

namespace fh2::kern {

fs::FsProvider* g_fsBridge = nullptr;
void setFsBridge(fs::FsProvider* fs) { g_fsBridge = fs; }
fs::FsProvider* fsBridge() { return g_fsBridge; }

uint32_t g_nextFileHandle = 0x60000000;
std::map<uint32_t, std::unique_ptr<GuestFile>> g_files;

uint32_t fileOpen(const std::string& guestPath, const std::string& display,
                  bool write, GuestFile** out) {
    *out = nullptr;
    if (!g_fsBridge) return 0;
    uint64_t size = 0;
    int fd = write ? g_fsBridge->openLocalWriteFd(guestPath)
                   : g_fsBridge->openFileFd(guestPath, size);
    if (fd < 0) return 0; // arquivo não existe — falha REAL
    if (write) size = 0;
    auto f = std::make_unique<GuestFile>();
    f->path = guestPath;
    f->display = display;
    f->fd = fd;
    f->size = size;
    f->write = write;
    GuestFile* p = f.get();
    uint32_t h;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        h = g_nextFileHandle += 4;
        g_files[h] = std::move(f);
    }
    *out = p;
    return h;
}

uint32_t fileOpenDir(const std::string& guestPath, const std::string& display,
                     GuestFile** out) {
    *out = nullptr;
    auto f = std::make_unique<GuestFile>();
    f->path = guestPath;
    f->display = display;
    f->dir = true;
    GuestFile* p = f.get();
    uint32_t h;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        h = g_nextFileHandle += 4;
        g_files[h] = std::move(f);
    }
    *out = p;
    return h;
}

uint32_t fileOpenRawDevice(const std::string& display, uint64_t sizeBytes,
                           GuestFile** out) {
    *out = nullptr;
    auto f = std::make_unique<GuestFile>();
    f->path = R"(\Device\Harddisk0\Partition0)";
    f->display = display;
    f->rawDevice = true;
    f->size = sizeBytes;
    GuestFile* p = f.get();
    uint32_t h;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        h = g_nextFileHandle += 4;
        g_files[h] = std::move(f);
    }
    *out = p;
    return h;
}

GuestFile* fileGet(uint32_t handle) {
    std::lock_guard<std::mutex> lk(g_stateM);
    auto it = g_files.find(handle);
    return it == g_files.end() ? nullptr : it->second.get();
}

void fileClose(uint32_t handle) {
    std::unique_ptr<GuestFile> f;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        auto it = g_files.find(handle);
        if (it == g_files.end()) return;
        f = std::move(it->second);
        g_files.erase(it);
    }
    if (f->fd >= 0) close(f->fd);
}

void filesCloseAll() {
    std::map<uint32_t, std::unique_ptr<GuestFile>> all;
    {
        std::lock_guard<std::mutex> lk(g_stateM);
        all = std::move(g_files);
        g_files.clear();
    }
    for (auto& [h, f] : all) {
        if (f->fd >= 0) close(f->fd);
    }
}

// ---------------------------------------------------------------- TLS

uint32_t tlsAlloc() {
    std::lock_guard<std::mutex> lk(g_stateM);
    for (uint32_t i = 0; i < 64; ++i) {
        if (!g_tlsUsed[i]) {
            g_tlsUsed[i] = true;
            g_tls[i] = nullptr;
            return i;
        }
    }
    return 0xFFFFFFFF;
}

void tlsFree(uint32_t slot) {
    std::lock_guard<std::mutex> lk(g_stateM);
    if (slot < 64) {
        g_tlsUsed[slot] = false;
        g_tls[slot] = nullptr;
    }
}

void* tlsGetValue(uint32_t slot) {
    std::lock_guard<std::mutex> lk(g_stateM);
    return slot < 64 ? g_tls[slot] : nullptr;
}

bool tlsSetValue(uint32_t slot, void* value) {
    std::lock_guard<std::mutex> lk(g_stateM);
    if (slot >= 64) return false;
    g_tls[slot] = value;
    return true;
}

// ---------------------------------------------------------------- tempo

uint64_t systemTime100ns() {
    using namespace std::chrono;
    // 1601-01-01 → 1970-01-01 = 11.644.473.600 s
    const auto now = system_clock::now().time_since_epoch();
    const uint64_t ns = duration_cast<nanoseconds>(now).count();
    return ns / 100ull + 116444736000000000ull;
}

uint64_t perfCounter50MHz() {
    return nowNs() / 20ull; // 50 MHz → 20 ns por tick
}

// ------------------------------------------------------ strings guest

std::string readGuestCString(uint64_t addr, size_t maxLen) {
    addr = (uint32_t)addr; // mask 32-bit (extensão de sinal do guest)
    if (!g_guestMem || addr == 0 || addr >= g_guestMemBytes) return {};
    std::string s;
    s.reserve(64);
    for (size_t i = 0; i < maxLen; ++i) {
        const uint64_t a = addr + i;
        if (a >= g_guestMemBytes) break;
        const char c = (char)g_guestMem[a];
        if (c == 0) break;
        s.push_back(c);
    }
    return s;
}

std::string readGuestWString(uint64_t addr, size_t maxLen) {
    addr = (uint32_t)addr; // mask 32-bit
    if (!g_guestMem || addr == 0 || addr >= g_guestMemBytes) return {};
    std::string s;
    s.reserve(64);
    for (size_t i = 0; i < maxLen; ++i) {
        const uint64_t a = addr + 2ull * i;
        if (a + 2 > g_guestMemBytes) break;
        const uint16_t wc =
            (uint16_t)(g_guestMem[a] << 8) | g_guestMem[a + 1]; // big-endian
        if (wc == 0) break;
        if (wc < 0x80) s.push_back((char)wc);
        else if (wc < 0x800) {
            s.push_back((char)(0xC0 | (wc >> 6)));
            s.push_back((char)(0x80 | (wc & 0x3F)));
        } else {
            s.push_back((char)(0xE0 | (wc >> 12)));
            s.push_back((char)(0x80 | ((wc >> 6) & 0x3F)));
            s.push_back((char)(0x80 | (wc & 0x3F)));
        }
    }
    return s;
}

// ------------------------------------------------- varargs do guest

namespace {

struct FmtSink {
    char* dst = nullptr;
    size_t cap = 0;
    size_t n = 0;
    bool wide = false;
    uint16_t* wdst = nullptr;

    void put(char c) {
        if (wide) return;
        if (n + 1 < cap) dst[n] = c;
        ++n;
    }
    void putW(uint16_t c) {
        if (!wide) return;
        if (n + 1 < cap) wdst[n] = c;
        ++n;
    }
    size_t done() {
        if (wide) {
            if (n < cap) wdst[n] = 0;
            else if (cap) wdst[cap - 1] = 0;
        } else {
            if (n < cap) dst[n] = 0;
            else if (cap) dst[cap - 1] = 0;
        }
        return n;
    }
};

void emitStr(FmtSink& s, const std::string& v) {
    if (s.wide) {
        for (unsigned char c : v) {
            s.putW(c);
            if (s.n >= s.cap) break;
        }
    } else {
        for (unsigned char c : v) {
            s.put((char)c);
            if (s.n >= s.cap) break;
        }
    }
}

void emitU64(FmtSink& s, uint64_t mag, unsigned base, bool upper,
             bool negative, int width, char padChar, int prec) {
    char tmp[32];
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int len = 0;
    do {
        tmp[len++] = digits[mag % base];
        mag /= base;
    } while (mag);
    // precisão mínima de dígitos
    while (prec > len) tmp[len++] = '0';
    std::string body;
    if (negative) body.push_back('-');
    while (len > 0) body.push_back(tmp[--len]);
    if ((int)body.size() < width) {
        const char pc = negative ? ' ' : padChar; // '-0123' não é válido
        body.insert(negative ? body.begin() + 1 : body.begin(),
                    width - body.size(), pc);
    }
    emitStr(s, body);
}

void emitF64(FmtSink& s, double v, int prec, int width) {
    char tmp[128];
    if (prec < 0) prec = 6;
    snprintf(tmp, sizeof(tmp), "%*.*f", width, prec, v);
    emitStr(s, tmp);
}

} // namespace

int guestFormat(char* dstHost, size_t dstBytes, const std::string& fmt,
                GuestArgs& args) {
    FmtSink s;
    s.dst = dstHost;
    s.cap = dstBytes;
    int intIdx = 0;
    int fpIdx = 0;

    size_t i = 0;
    while (i < fmt.size()) {
        char c = fmt[i++];
        if (c != '%') { s.put(c); continue; }
        if (i < fmt.size() && fmt[i] == '%') { s.put('%'); ++i; continue; }

        // flags
        char padChar = ' ';
        while (i < fmt.size() && (fmt[i] == '-' || fmt[i] == '0' ||
                                  fmt[i] == '+' || fmt[i] == ' ')) {
            if (fmt[i] == '0') padChar = '0';
            ++i;
        }
        // width
        int width = 0;
        while (i < fmt.size() && fmt[i] >= '0' && fmt[i] <= '9') {
            width = width * 10 + (fmt[i++] - '0');
        }
        // precision
        int prec = -1;
        if (i < fmt.size() && fmt[i] == '.') {
            ++i;
            prec = 0;
            while (i < fmt.size() && fmt[i] >= '0' && fmt[i] <= '9') {
                prec = prec * 10 + (fmt[i++] - '0');
            }
        }
        // length
        enum Len { L_DEF, L_H, L_L, L_LL, L_I64, L_I32 } len = L_DEF;
        if (i + 2 < fmt.size() && fmt[i] == 'I' && fmt[i+1] == '6' && fmt[i+2] == '4') { len = L_I64; i += 3; }
        else if (i + 2 < fmt.size() && fmt[i] == 'I' && fmt[i+1] == '3' && fmt[i+2] == '2') { len = L_I32; i += 3; }
        else if (i < fmt.size() && fmt[i] == 'l') {
            len = L_L; ++i;
            if (i < fmt.size() && fmt[i] == 'l') { len = L_LL; ++i; }
        } else if (i < fmt.size() && fmt[i] == 'h') {
            len = L_H; ++i;
        }

        if (i >= fmt.size()) break;
        const char spec = fmt[i++];
        switch (spec) {
        case 'c': {
            const uint64_t v = args.nextInt(intIdx++);
            s.put((char)v);
            break;
        }
        case 's': {
            const uint64_t v = args.nextInt(intIdx++);
            emitStr(s, readGuestCString(v));
            break;
        }
        case 'S': { // wide string (UTF-16 BE)
            const uint64_t v = args.nextInt(intIdx++);
            emitStr(s, readGuestWString(v));
            break;
        }
        case 'd': case 'i': {
            bool neg;
            uint64_t mag;
            if (len == L_LL || len == L_I64) {
                const int64_t v = (int64_t)args.nextInt(intIdx++);
                neg = v < 0;
                mag = neg ? (uint64_t)(-v) : (uint64_t)v;
            } else {
                const int32_t v = (int32_t)args.nextInt(intIdx++);
                neg = v < 0;
                mag = neg ? (uint64_t)(-v) : (uint64_t)(uint32_t)v;
            }
            emitU64(s, mag, 10, false, neg, width, padChar, prec);
            break;
        }
        case 'u': {
            const uint64_t v = args.nextInt(intIdx++);
            emitU64(s, v, 10, false, false, width, padChar, prec);
            break;
        }
        case 'x': case 'X': {
            const uint64_t v = args.nextInt(intIdx++);
            emitU64(s, v, 16, spec == 'X', false, width, padChar, prec);
            break;
        }
        case 'p': {
            const uint64_t v = args.nextInt(intIdx++);
            std::string body = "0x";
            char tmp[16];
            snprintf(tmp, sizeof(tmp), "%08X", (uint32_t)v);
            body += tmp;
            emitStr(s, body);
            break;
        }
        case 'f': case 'F': {
            const double v = args.nextDouble(fpIdx++);
            emitF64(s, v, prec < 0 ? 6 : prec, width);
            break;
        }
        case 'g': case 'G': case 'e': case 'E': {
            const double v = args.nextDouble(fpIdx++);
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "%.*g", prec < 0 ? 6 : prec, v);
            emitStr(s, tmp);
            break;
        }
        case 'n': { // write-back de comprimento: perigoso — apenas consome
            ++intIdx;
            KLOG("printf %%n ignorado (segurança)");
            break;
        }
        default:
            s.put('%');
            s.put(spec);
            break;
        }
    }
    return (int)s.done();
}

int guestFormatW(uint16_t* dstHost, size_t dstChars, const std::string& fmt,
                 GuestArgs& args) {
    FmtSink s;
    s.wide = true;
    s.wdst = dstHost;
    s.cap = dstChars;
    int intIdx = 0;
    int fpIdx = 0;

    size_t i = 0;
    while (i < fmt.size()) {
        char c = fmt[i++];
        if (c != '%') { s.putW((uint16_t)c); continue; }
        if (i < fmt.size() && fmt[i] == '%') { s.putW('%'); ++i; continue; }
        // versão compacta: flags/width/prec ignoradas em wide (raro no boot)
        while (i < fmt.size() && (fmt[i] == '-' || fmt[i] == '0' ||
                                  fmt[i] == '+' || fmt[i] == ' ')) ++i;
        while (i < fmt.size() && fmt[i] >= '0' && fmt[i] <= '9') ++i;
        if (i < fmt.size() && fmt[i] == '.') {
            ++i;
            while (i < fmt.size() && fmt[i] >= '0' && fmt[i] <= '9') ++i;
        }
        if (i < fmt.size() && fmt[i] == 'l') ++i;
        if (i >= fmt.size()) break;
        const char spec = fmt[i++];
        switch (spec) {
        case 'c': s.putW((uint16_t)args.nextInt(intIdx++)); break;
        case 's': case 'S': {
            const uint64_t v = args.nextInt(intIdx++);
            // em função wide, %s = string wide
            emitStr(s, readGuestWString(v));
            break;
        }
        case 'd': case 'i': {
            const uint64_t v = (uint64_t)(int64_t)(int32_t)args.nextInt(intIdx++);
            const bool neg = (int32_t)v < 0;
            emitU64(s, neg ? (uint64_t)(-(int64_t)v) : v, 10, false, neg, 0, ' ', -1);
            break;
        }
        case 'u': emitU64(s, args.nextInt(intIdx++), 10, false, false, 0, ' ', -1); break;
        case 'x': case 'X': emitU64(s, args.nextInt(intIdx++), 16, spec == 'X', false, 0, ' ', -1); break;
        case 'f': case 'g': case 'e': {
            emitF64(s, args.nextDouble(fpIdx++), 6, 0);
            break;
        }
        default: s.putW('%'); s.putW((uint16_t)spec); break;
        }
    }
    return (int)s.done();
}

// ------------------------------------------------- física/memória misc

MemStats memStats() {
    MemStats s;
    s.heapUsed = g_heap.used();
    s.heapSize = g_heap.size();
    s.virtUsed = g_virt.used();
    s.virtSize = g_virt.size();
    return s;
}

} // namespace fh2::kern
