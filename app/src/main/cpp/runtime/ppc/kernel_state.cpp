// kernel_state.cpp — implementação do estado do kernel HLE (semântica real)
//
// Toda a concorrência do kernel vive aqui: registry de threads guest, mapas
// de objetos sincronizáveis, TLS do kernel, tempo do host e o pedido de
// stop global (com acordas de emergência nos waits).
#include "kernel_state.h"

#include <algorithm>
#include <android/log.h>

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

input::InputState* g_input = nullptr;

// --- threads ---
ThreadBody g_threadBody;
std::map<uint32_t, std::shared_ptr<GuestThread>> g_threads;
uint32_t g_nextThreadHandle = 0x90000000;
uint32_t g_nextThreadId = 1;

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

// --- stop ---
std::atomic<bool> g_stop{false};

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

fh2::ppc::GuestHeap& heap() { return g_heap; }
fh2::ppc::GuestVirtWindow& virtWindow() { return g_virt; }

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
        t->id = g_nextThreadId++;
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
    {
        std::lock_guard<std::mutex> lk(t->startM);
        t->started = true;
    }
    t->startCv.notify_all();
    return true;
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
    g_waitCv.notify_all();
    return prev;
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

void wakeAll() {
    g_waitCv.notify_all();
    forEachThread([](GuestThread& t) {
        t.startCv.notify_all();
    });
}

uint32_t waitForMultiple(Waitable** objs, size_t count, bool waitAny,
                         int64_t timeoutNs, bool alertable) {
    // timeout: negativo = relativo; 0 = teste; positivo = absoluto
    // (convertido com log — hosts não têm o epoch do console).
    (void)alertable;
    if (count == 0) return kWaitTimeout;

    std::unique_lock<std::mutex> lk(g_waitM);
    auto pred = [&] {
        for (size_t i = 0; i < count; ++i)
            if (objs[i]->signaled) return true;
        return false;
    };
    auto consume = [&]() -> uint32_t {
        uint32_t firstSignaled = kWaitTimeout;
        for (size_t i = 0; i < count; ++i) {
            Waitable& w = *objs[i];
            if (!w.signaled) continue;
            if (waitAny) {
                if (w.autoReset) w.signaled = false;
                return uint32_t(i) + (w.abandoned ? kWaitAbandoned0
                                                  : kWaitSignaled0);
            }
            if (firstSignaled == kWaitTimeout) firstSignaled = kWaitSignaled0;
            if (w.autoReset) w.signaled = false;
        }
        return firstSignaled;
    };

    if (timeoutNs == 0) return pred() ? consume() : kWaitTimeout;

    int64_t rel = timeoutNs;
    if (rel > 0) {
        KLOG("wait com timeout ABSOLUTO %lld — tratado como relativo",
             (long long)rel);
    } else {
        rel = -rel;
    }
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::nanoseconds(rel);
    bool ok = g_waitCv.wait_until(lk, deadline, [&] {
        return pred() || g_stop;
    });
    if (!pred()) {
        (void)ok;
        return kWaitTimeout; // timeout ou stop — caller decide
    }
    return consume();
}

// --------------------------------------------------------- title id

uint32_t g_titleId = 0;
void setTitleId(uint32_t id) { g_titleId = id; }
uint32_t currentTitleId() { return g_titleId; }

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
