// kernel_hle.cpp — despacho HLE dos imports de xboxkrnl.exe/xam.xex (FH2)
//
// GERADO por tools/gen_kernel_hle.py — 388 símbolos exigidos por
// ppc_func_mapping.cpp. Três caminhos:
//   REAL  (75): delega para fh2::kern::real_* com semântica real
//         (heap, tempo, threads, sync, TLS, printf, input — kernel_real.cpp)
//   FAIL  (26): retorna status de falha REAL do estado do sistema
//   stub  (287): log-once + NTSTATUS 0 (semântica pendente — issue #16)
// NÃO EDITAR À MÃO.
#if FH2_HAS_RECOMP

#include <android/log.h>
#include "ppc_config.h"
#include "ppc_context.h"
#include "runtime/ppc/kernel_real.h"

#define HLOG(...) __android_log_print(ANDROID_LOG_WARN, "FH2/HLE", __VA_ARGS__)

#define FH2_HLE_ONCE(name, note) do { static bool _logged_##name = false; if (!_logged_##name) { _logged_##name = true; HLOG("HLE stub: %s — %s", #name, note); } } while (0)

// DbgBreakPoint: semântica REAL (kernel_real.cpp)
void __imp__DbgBreakPoint(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_DbgBreakPoint(ctx, base);
}

// DbgPrint: semântica REAL (kernel_real.cpp)
void __imp__DbgPrint(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_DbgPrint(ctx, base);
}

// EtxProducerLog: semântica real pendente (issue #16)
void __imp__EtxProducerLog(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(EtxProducerLog, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// EtxProducerRegister: semântica real pendente (issue #16)
void __imp__EtxProducerRegister(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(EtxProducerRegister, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// EtxProducerUnregister: semântica real pendente (issue #16)
void __imp__EtxProducerUnregister(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(EtxProducerUnregister, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ExAllocatePool: semântica REAL (kernel_real.cpp)
void __imp__ExAllocatePool(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_ExAllocatePool(ctx, base);
}

// ExAllocatePoolTypeWithTag: semântica REAL (kernel_real.cpp)
void __imp__ExAllocatePoolTypeWithTag(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_ExAllocatePoolTypeWithTag(ctx, base);
}

// ExCreateThread: semântica REAL (kernel_real.cpp)
void __imp__ExCreateThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_ExCreateThread(ctx, base);
}

// ExFreePool: semântica REAL (kernel_real.cpp)
void __imp__ExFreePool(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_ExFreePool(ctx, base);
}

// ExGetXConfigSetting: semântica real pendente (issue #16)
void __imp__ExGetXConfigSetting(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ExGetXConfigSetting, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ExRegisterTitleTerminateNotification: semântica real pendente (issue #16)
void __imp__ExRegisterTitleTerminateNotification(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ExRegisterTitleTerminateNotification, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ExTerminateThread: semântica REAL (kernel_real.cpp)
void __imp__ExTerminateThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_ExTerminateThread(ctx, base);
}

// FscSetCacheElementCount: semântica real pendente (issue #16)
void __imp__FscSetCacheElementCount(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(FscSetCacheElementCount, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// HalReturnToFirmware: semântica REAL (kernel_real.cpp)
void __imp__HalReturnToFirmware(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_HalReturnToFirmware(ctx, base);
}

// InterlockedFlushSList: semântica real pendente (issue #16)
void __imp__InterlockedFlushSList(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(InterlockedFlushSList, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// InterlockedPopEntrySList: semântica real pendente (issue #16)
void __imp__InterlockedPopEntrySList(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(InterlockedPopEntrySList, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoCheckShareAccess: semântica real pendente (issue #16)
void __imp__IoCheckShareAccess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoCheckShareAccess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoCompleteRequest: semântica real pendente (issue #16)
void __imp__IoCompleteRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoCompleteRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoCreateDevice: semântica real pendente (issue #16)
void __imp__IoCreateDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoCreateDevice, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoDeleteDevice: semântica real pendente (issue #16)
void __imp__IoDeleteDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoDeleteDevice, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoDismountVolume: semântica real pendente (issue #16)
void __imp__IoDismountVolume(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoDismountVolume, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoDismountVolumeByFileHandle: semântica real pendente (issue #16)
void __imp__IoDismountVolumeByFileHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoDismountVolumeByFileHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoInvalidDeviceRequest: semântica real pendente (issue #16)
void __imp__IoInvalidDeviceRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoInvalidDeviceRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoRemoveShareAccess: semântica real pendente (issue #16)
void __imp__IoRemoveShareAccess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoRemoveShareAccess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// IoSetShareAccess: semântica real pendente (issue #16)
void __imp__IoSetShareAccess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(IoSetShareAccess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeAcquireSpinLockAtRaisedIrql: semântica real pendente (issue #16)
void __imp__KeAcquireSpinLockAtRaisedIrql(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeAcquireSpinLockAtRaisedIrql, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeBugCheck: semântica REAL (kernel_real.cpp)
void __imp__KeBugCheck(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeBugCheck(ctx, base);
}

// KeBugCheckEx: semântica REAL (kernel_real.cpp)
void __imp__KeBugCheckEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeBugCheckEx(ctx, base);
}

// KeDelayExecutionThread: semântica REAL (kernel_real.cpp)
void __imp__KeDelayExecutionThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeDelayExecutionThread(ctx, base);
}

// KeEnterCriticalRegion: semântica real pendente (issue #16)
void __imp__KeEnterCriticalRegion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeEnterCriticalRegion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeGetCurrentProcessType: semântica REAL (kernel_real.cpp)
void __imp__KeGetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeGetCurrentProcessType(ctx, base);
}

// KeInitializeDpc: semântica real pendente (issue #16)
void __imp__KeInitializeDpc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeInitializeDpc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeInitializeMutant: semântica REAL (kernel_real.cpp)
void __imp__KeInitializeMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeInitializeMutant(ctx, base);
}

// KeInsertQueueDpc: semântica real pendente (issue #16)
void __imp__KeInsertQueueDpc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeInsertQueueDpc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeLeaveCriticalRegion: semântica real pendente (issue #16)
void __imp__KeLeaveCriticalRegion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeLeaveCriticalRegion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeLockL2: semântica real pendente (issue #16)
void __imp__KeLockL2(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeLockL2, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeQueryBasePriorityThread: semântica real pendente (issue #16)
void __imp__KeQueryBasePriorityThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeQueryBasePriorityThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeQueryPerformanceFrequency: semântica REAL (kernel_real.cpp)
void __imp__KeQueryPerformanceFrequency(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeQueryPerformanceFrequency(ctx, base);
}

// KeQuerySystemTime: semântica REAL (kernel_real.cpp)
void __imp__KeQuerySystemTime(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeQuerySystemTime(ctx, base);
}

// KeReleaseMutant: semântica REAL (kernel_real.cpp)
void __imp__KeReleaseMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeReleaseMutant(ctx, base);
}

// KeReleaseSpinLockFromRaisedIrql: semântica real pendente (issue #16)
void __imp__KeReleaseSpinLockFromRaisedIrql(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeReleaseSpinLockFromRaisedIrql, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeResetEvent: semântica REAL (kernel_real.cpp)
void __imp__KeResetEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeResetEvent(ctx, base);
}

// KeRestoreFloatingPointState: semântica real pendente (issue #16)
void __imp__KeRestoreFloatingPointState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeRestoreFloatingPointState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeResumeThread: semântica REAL (kernel_real.cpp)
void __imp__KeResumeThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeResumeThread(ctx, base);
}

// KeSaveFloatingPointState: semântica real pendente (issue #16)
void __imp__KeSaveFloatingPointState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeSaveFloatingPointState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeSetAffinityThread: semântica real pendente (issue #16)
void __imp__KeSetAffinityThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeSetAffinityThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeSetBasePriorityThread: semântica real pendente (issue #16)
void __imp__KeSetBasePriorityThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeSetBasePriorityThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeSetCurrentProcessType: semântica REAL (kernel_real.cpp)
void __imp__KeSetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeSetCurrentProcessType(ctx, base);
}

// KeSetCurrentStackPointers: semântica real pendente (issue #16)
void __imp__KeSetCurrentStackPointers(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeSetCurrentStackPointers, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeSetEvent: semântica REAL (kernel_real.cpp)
void __imp__KeSetEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeSetEvent(ctx, base);
}

// KeTlsAlloc: semântica REAL (kernel_real.cpp)
void __imp__KeTlsAlloc(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeTlsAlloc(ctx, base);
}

// KeTlsFree: semântica REAL (kernel_real.cpp)
void __imp__KeTlsFree(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeTlsFree(ctx, base);
}

// KeTlsGetValue: semântica REAL (kernel_real.cpp)
void __imp__KeTlsGetValue(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeTlsGetValue(ctx, base);
}

// KeTlsSetValue: semântica REAL (kernel_real.cpp)
void __imp__KeTlsSetValue(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeTlsSetValue(ctx, base);
}

// KeUnlockL2: semântica real pendente (issue #16)
void __imp__KeUnlockL2(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeUnlockL2, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeWaitForMultipleObjects: semântica REAL (kernel_real.cpp)
void __imp__KeWaitForMultipleObjects(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeWaitForMultipleObjects(ctx, base);
}

// KeWaitForSingleObject: semântica REAL (kernel_real.cpp)
void __imp__KeWaitForSingleObject(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_KeWaitForSingleObject(ctx, base);
}

// KfAcquireSpinLock: semântica real pendente (issue #16)
void __imp__KfAcquireSpinLock(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KfAcquireSpinLock, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KfReleaseSpinLock: semântica real pendente (issue #16)
void __imp__KfReleaseSpinLock(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KfReleaseSpinLock, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KiApcNormalRoutineNop: semântica real pendente (issue #16)
void __imp__KiApcNormalRoutineNop(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KiApcNormalRoutineNop, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// LDICreateDecompression: semântica real pendente (issue #16)
void __imp__LDICreateDecompression(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(LDICreateDecompression, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// LDIDecompress: semântica real pendente (issue #16)
void __imp__LDIDecompress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(LDIDecompress, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// LDIDestroyDecompression: semântica real pendente (issue #16)
void __imp__LDIDestroyDecompression(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(LDIDestroyDecompression, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmAllocatePhysicalMemoryEx: semântica REAL (kernel_real.cpp)
void __imp__MmAllocatePhysicalMemoryEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_MmAllocatePhysicalMemoryEx(ctx, base);
}

// MmCreateKernelStack: semântica REAL (kernel_real.cpp)
void __imp__MmCreateKernelStack(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_MmCreateKernelStack(ctx, base);
}

// MmDeleteKernelStack: semântica REAL (kernel_real.cpp)
void __imp__MmDeleteKernelStack(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_MmDeleteKernelStack(ctx, base);
}

// MmFreePhysicalMemory: semântica REAL (kernel_real.cpp)
void __imp__MmFreePhysicalMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_MmFreePhysicalMemory(ctx, base);
}

// MmGetPhysicalAddress: semântica REAL (kernel_real.cpp)
void __imp__MmGetPhysicalAddress(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_MmGetPhysicalAddress(ctx, base);
}

// MmMapIoSpace: semântica real pendente (issue #16)
void __imp__MmMapIoSpace(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmMapIoSpace, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmQueryAddressProtect: semântica real pendente (issue #16)
void __imp__MmQueryAddressProtect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmQueryAddressProtect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmQueryAllocationSize: semântica REAL (kernel_real.cpp)
void __imp__MmQueryAllocationSize(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_MmQueryAllocationSize(ctx, base);
}

// MmQueryStatistics: semântica real pendente (issue #16)
void __imp__MmQueryStatistics(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmQueryStatistics, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmSetAddressProtect: semântica real pendente (issue #16)
void __imp__MmSetAddressProtect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmSetAddressProtect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSACancelOverlappedIO: semântica real pendente (issue #16)
void __imp__NetDll_WSACancelOverlappedIO(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSACancelOverlappedIO, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSACleanup: semântica real pendente (issue #16)
void __imp__NetDll_WSACleanup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSACleanup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSACloseEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSACloseEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSACloseEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSACreateEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSACreateEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSACreateEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSAEventSelect: semântica real pendente (issue #16)
void __imp__NetDll_WSAEventSelect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSAEventSelect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSAGetLastError: semântica real pendente (issue #16)
void __imp__NetDll_WSAGetLastError(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSAGetLastError, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSAGetOverlappedResult: semântica real pendente (issue #16)
void __imp__NetDll_WSAGetOverlappedResult(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSAGetOverlappedResult, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSARecv: semântica real pendente (issue #16)
void __imp__NetDll_WSARecv(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSARecv, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSARecvFrom: semântica real pendente (issue #16)
void __imp__NetDll_WSARecvFrom(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSARecvFrom, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSAResetEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSAResetEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSAResetEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSASend: semântica real pendente (issue #16)
void __imp__NetDll_WSASend(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSASend, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSASendTo: semântica real pendente (issue #16)
void __imp__NetDll_WSASendTo(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSASendTo, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSASetEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSASetEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSASetEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSASetLastError: semântica real pendente (issue #16)
void __imp__NetDll_WSASetLastError(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSASetLastError, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_WSAStartup: falha REAL — rede real pendente (issue #19)
void __imp__NetDll_WSAStartup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSAStartup, "rede real pendente (issue #19)");
    ctx.r3.u32 = 0x800704CFu;
}

// NetDll_WSAWaitForMultipleEvents: semântica real pendente (issue #16)
void __imp__NetDll_WSAWaitForMultipleEvents(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSAWaitForMultipleEvents, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetCleanup: semântica real pendente (issue #16)
void __imp__NetDll_XNetCleanup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetCleanup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetConnect: semântica real pendente (issue #16)
void __imp__NetDll_XNetConnect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetConnect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetCreateKey: semântica real pendente (issue #16)
void __imp__NetDll_XNetCreateKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetCreateKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetGetConnectStatus: semântica real pendente (issue #16)
void __imp__NetDll_XNetGetConnectStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetGetConnectStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetGetEthernetLinkStatus: semântica real pendente (issue #16)
void __imp__NetDll_XNetGetEthernetLinkStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetGetEthernetLinkStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetGetTitleXnAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetGetTitleXnAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetGetTitleXnAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetInAddrToString: semântica real pendente (issue #16)
void __imp__NetDll_XNetInAddrToString(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetInAddrToString, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetInAddrToXnAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetInAddrToXnAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetInAddrToXnAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetQosListen: semântica real pendente (issue #16)
void __imp__NetDll_XNetQosListen(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetQosListen, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetQosLookup: semântica real pendente (issue #16)
void __imp__NetDll_XNetQosLookup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetQosLookup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetQosRelease: semântica real pendente (issue #16)
void __imp__NetDll_XNetQosRelease(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetQosRelease, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetRandom: semântica real pendente (issue #16)
void __imp__NetDll_XNetRandom(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetRandom, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetRegisterKey: semântica real pendente (issue #16)
void __imp__NetDll_XNetRegisterKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetRegisterKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetServerToInAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetServerToInAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetServerToInAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetStartup: semântica real pendente (issue #16)
void __imp__NetDll_XNetStartup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetStartup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetUnregisterInAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetUnregisterInAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetUnregisterInAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetUnregisterKey: semântica real pendente (issue #16)
void __imp__NetDll_XNetUnregisterKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetUnregisterKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetXnAddrToInAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetXnAddrToInAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetXnAddrToInAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XNetXnAddrToMachineId: semântica real pendente (issue #16)
void __imp__NetDll_XNetXnAddrToMachineId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XNetXnAddrToMachineId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_XnpGetConfigStatus: semântica real pendente (issue #16)
void __imp__NetDll_XnpGetConfigStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_XnpGetConfigStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll___WSAFDIsSet: semântica real pendente (issue #16)
void __imp__NetDll___WSAFDIsSet(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll___WSAFDIsSet, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_accept: semântica real pendente (issue #16)
void __imp__NetDll_accept(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_accept, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_bind: semântica real pendente (issue #16)
void __imp__NetDll_bind(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_bind, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_closesocket: semântica real pendente (issue #16)
void __imp__NetDll_closesocket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_closesocket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_connect: semântica real pendente (issue #16)
void __imp__NetDll_connect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_connect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_getpeername: semântica real pendente (issue #16)
void __imp__NetDll_getpeername(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_getpeername, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_getsockname: semântica real pendente (issue #16)
void __imp__NetDll_getsockname(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_getsockname, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_getsockopt: semântica real pendente (issue #16)
void __imp__NetDll_getsockopt(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_getsockopt, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_inet_addr: semântica real pendente (issue #16)
void __imp__NetDll_inet_addr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_inet_addr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_ioctlsocket: semântica real pendente (issue #16)
void __imp__NetDll_ioctlsocket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_ioctlsocket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_listen: semântica real pendente (issue #16)
void __imp__NetDll_listen(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_listen, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_recv: semântica real pendente (issue #16)
void __imp__NetDll_recv(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_recv, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_recvfrom: semântica real pendente (issue #16)
void __imp__NetDll_recvfrom(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_recvfrom, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_select: semântica real pendente (issue #16)
void __imp__NetDll_select(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_select, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_send: semântica real pendente (issue #16)
void __imp__NetDll_send(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_send, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_sendto: semântica real pendente (issue #16)
void __imp__NetDll_sendto(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_sendto, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_setsockopt: semântica real pendente (issue #16)
void __imp__NetDll_setsockopt(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_setsockopt, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_shutdown: semântica real pendente (issue #16)
void __imp__NetDll_shutdown(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_shutdown, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NetDll_socket: semântica real pendente (issue #16)
void __imp__NetDll_socket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_socket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtAllocateVirtualMemory: semântica REAL (kernel_real.cpp)
void __imp__NtAllocateVirtualMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtAllocateVirtualMemory(ctx, base);
}

// NtCancelIoFile: falha REAL — IO pendente
void __imp__NtCancelIoFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCancelIoFile, "IO pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtCancelTimer: semântica real pendente (issue #16)
void __imp__NtCancelTimer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCancelTimer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtClearEvent: semântica REAL (kernel_real.cpp)
void __imp__NtClearEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtClearEvent(ctx, base);
}

// NtClose: semântica REAL (kernel_real.cpp)
void __imp__NtClose(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtClose(ctx, base);
}

// NtCreateEvent: semântica REAL (kernel_real.cpp)
void __imp__NtCreateEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtCreateEvent(ctx, base);
}

// NtCreateFile: falha REAL — STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente
void __imp__NtCreateFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCreateFile, "STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente");
    ctx.r3.u32 = 0xC0000034u;
}

// NtCreateMutant: semântica REAL (kernel_real.cpp)
void __imp__NtCreateMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtCreateMutant(ctx, base);
}

// NtCreateSemaphore: semântica REAL (kernel_real.cpp)
void __imp__NtCreateSemaphore(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtCreateSemaphore(ctx, base);
}

// NtCreateTimer: semântica real pendente (issue #16)
void __imp__NtCreateTimer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCreateTimer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtDeviceIoControlFile: falha REAL — IOCTL pendente
void __imp__NtDeviceIoControlFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtDeviceIoControlFile, "IOCTL pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtDuplicateObject: semântica real pendente (issue #16)
void __imp__NtDuplicateObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtDuplicateObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtFlushBuffersFile: falha REAL — FS pendente
void __imp__NtFlushBuffersFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtFlushBuffersFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtFreeVirtualMemory: semântica REAL (kernel_real.cpp)
void __imp__NtFreeVirtualMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtFreeVirtualMemory(ctx, base);
}

// NtOpenFile: falha REAL — STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente
void __imp__NtOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtOpenFile, "STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente");
    ctx.r3.u32 = 0xC0000034u;
}

// NtQueryDirectoryFile: falha REAL — FS pendente
void __imp__NtQueryDirectoryFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryDirectoryFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtQueryFullAttributesFile: falha REAL — FS pendente
void __imp__NtQueryFullAttributesFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryFullAttributesFile, "FS pendente");
    ctx.r3.u32 = 0xC0000034u;
}

// NtQueryInformationFile: falha REAL — FS pendente
void __imp__NtQueryInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryInformationFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtQueryVirtualMemory: semântica real pendente (issue #16)
void __imp__NtQueryVirtualMemory(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryVirtualMemory, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtQueryVolumeInformationFile: falha REAL — FS pendente
void __imp__NtQueryVolumeInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryVolumeInformationFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtReadFile: falha REAL — sem handle real (FS pendente)
void __imp__NtReadFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtReadFile, "sem handle real (FS pendente)");
    ctx.r3.u32 = 0xC0000001u;
}

// NtReadFileScatter: falha REAL — FS pendente
void __imp__NtReadFileScatter(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtReadFileScatter, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtReleaseMutant: semântica REAL (kernel_real.cpp)
void __imp__NtReleaseMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtReleaseMutant(ctx, base);
}

// NtReleaseSemaphore: semântica REAL (kernel_real.cpp)
void __imp__NtReleaseSemaphore(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtReleaseSemaphore(ctx, base);
}

// NtResumeThread: semântica real pendente (issue #16)
void __imp__NtResumeThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtResumeThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtSetEvent: semântica REAL (kernel_real.cpp)
void __imp__NtSetEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtSetEvent(ctx, base);
}

// NtSetInformationFile: falha REAL — FS pendente
void __imp__NtSetInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtSetInformationFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtSetTimerEx: semântica real pendente (issue #16)
void __imp__NtSetTimerEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtSetTimerEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtSignalAndWaitForSingleObjectEx: semântica REAL (kernel_real.cpp)
void __imp__NtSignalAndWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtSignalAndWaitForSingleObjectEx(ctx, base);
}

// NtWaitForMultipleObjectsEx: semântica REAL (kernel_real.cpp)
void __imp__NtWaitForMultipleObjectsEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtWaitForMultipleObjectsEx(ctx, base);
}

// NtWaitForSingleObjectEx: semântica REAL (kernel_real.cpp)
void __imp__NtWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtWaitForSingleObjectEx(ctx, base);
}

// NtWriteFile: falha REAL — sem handle real (FS pendente)
void __imp__NtWriteFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtWriteFile, "sem handle real (FS pendente)");
    ctx.r3.u32 = 0xC0000001u;
}

// NtWriteFileGather: falha REAL — FS pendente
void __imp__NtWriteFileGather(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtWriteFileGather, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// NtYieldExecution: semântica REAL (kernel_real.cpp)
void __imp__NtYieldExecution(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_NtYieldExecution(ctx, base);
}

// ObCreateObject: semântica real pendente (issue #16)
void __imp__ObCreateObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObCreateObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ObCreateSymbolicLink: semântica real pendente (issue #16)
void __imp__ObCreateSymbolicLink(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObCreateSymbolicLink, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ObDeleteSymbolicLink: semântica real pendente (issue #16)
void __imp__ObDeleteSymbolicLink(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObDeleteSymbolicLink, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ObDereferenceObject: semântica real pendente (issue #16)
void __imp__ObDereferenceObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObDereferenceObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ObIsTitleObject: semântica real pendente (issue #16)
void __imp__ObIsTitleObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObIsTitleObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ObOpenObjectByPointer: semântica real pendente (issue #16)
void __imp__ObOpenObjectByPointer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObOpenObjectByPointer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ObReferenceObject: semântica real pendente (issue #16)
void __imp__ObReferenceObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObReferenceObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ObReferenceObjectByHandle: semântica real pendente (issue #16)
void __imp__ObReferenceObjectByHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ObReferenceObjectByHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// PsCamDeviceRequest: semântica real pendente (issue #16)
void __imp__PsCamDeviceRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(PsCamDeviceRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlCaptureContext: semântica real pendente (issue #16)
void __imp__RtlCaptureContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlCaptureContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlCompareMemory: semântica REAL (kernel_real.cpp)
void __imp__RtlCompareMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlCompareMemory(ctx, base);
}

// RtlCompareMemoryUlong: semântica REAL (kernel_real.cpp)
void __imp__RtlCompareMemoryUlong(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlCompareMemoryUlong(ctx, base);
}

// RtlCompareStringN: semântica real pendente (issue #16)
void __imp__RtlCompareStringN(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlCompareStringN, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlEnterCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlEnterCriticalSection(ctx, base);
}

// RtlFillMemoryUlong: semântica REAL (kernel_real.cpp)
void __imp__RtlFillMemoryUlong(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlFillMemoryUlong(ctx, base);
}

// RtlFreeAnsiString: semântica REAL (kernel_real.cpp)
void __imp__RtlFreeAnsiString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlFreeAnsiString(ctx, base);
}

// RtlImageXexHeaderField: semântica real pendente (issue #16)
void __imp__RtlImageXexHeaderField(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlImageXexHeaderField, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlInitAnsiString: semântica REAL (kernel_real.cpp)
void __imp__RtlInitAnsiString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlInitAnsiString(ctx, base);
}

// RtlInitUnicodeString: semântica REAL (kernel_real.cpp)
void __imp__RtlInitUnicodeString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlInitUnicodeString(ctx, base);
}

// RtlInitializeCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlInitializeCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlInitializeCriticalSection(ctx, base);
}

// RtlInitializeCriticalSectionAndSpinCount: semântica REAL (kernel_real.cpp)
void __imp__RtlInitializeCriticalSectionAndSpinCount(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlInitializeCriticalSectionAndSpinCount(ctx, base);
}

// RtlLeaveCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlLeaveCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlLeaveCriticalSection(ctx, base);
}

// RtlMultiByteToUnicodeN: semântica REAL (kernel_real.cpp)
void __imp__RtlMultiByteToUnicodeN(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlMultiByteToUnicodeN(ctx, base);
}

// RtlNtStatusToDosError: semântica REAL (kernel_real.cpp)
void __imp__RtlNtStatusToDosError(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlNtStatusToDosError(ctx, base);
}

// RtlRaiseException: SEH do guest não suportado (issue #15)
void __imp__RtlRaiseException(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlRaiseException, "SEH do guest não suportado (issue #15)");
    ctx.r3.u32 = 0;
}

// RtlTimeFieldsToTime: semântica REAL (kernel_real.cpp)
void __imp__RtlTimeFieldsToTime(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlTimeFieldsToTime(ctx, base);
}

// RtlTimeToTimeFields: semântica REAL (kernel_real.cpp)
void __imp__RtlTimeToTimeFields(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlTimeToTimeFields(ctx, base);
}

// RtlTryEnterCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlTryEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlTryEnterCriticalSection(ctx, base);
}

// RtlUnicodeStringToAnsiString: semântica REAL (kernel_real.cpp)
void __imp__RtlUnicodeStringToAnsiString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlUnicodeStringToAnsiString(ctx, base);
}

// RtlUnicodeToMultiByteN: semântica REAL (kernel_real.cpp)
void __imp__RtlUnicodeToMultiByteN(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlUnicodeToMultiByteN(ctx, base);
}

// RtlUnwind: semântica real pendente (issue #16)
void __imp__RtlUnwind(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlUnwind, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlUpcaseUnicodeChar: semântica REAL (kernel_real.cpp)
void __imp__RtlUpcaseUnicodeChar(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_RtlUpcaseUnicodeChar(ctx, base);
}

// StfsControlDevice: falha REAL — STFS pendente
void __imp__StfsControlDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(StfsControlDevice, "STFS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// StfsCreateDevice: falha REAL — STFS pendente
void __imp__StfsCreateDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(StfsCreateDevice, "STFS pendente");
    ctx.r3.u32 = 0xC0000001u;
}

// VdCallGraphicsNotificationRoutines: semântica real pendente (issue #16)
void __imp__VdCallGraphicsNotificationRoutines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdCallGraphicsNotificationRoutines, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdEnableDisableClockGating: semântica real pendente (issue #16)
void __imp__VdEnableDisableClockGating(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdEnableDisableClockGating, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdEnableRingBufferRPtrWriteBack: semântica real pendente (issue #16)
void __imp__VdEnableRingBufferRPtrWriteBack(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdEnableRingBufferRPtrWriteBack, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdGetCurrentDisplayGamma: semântica real pendente (issue #16)
void __imp__VdGetCurrentDisplayGamma(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdGetCurrentDisplayGamma, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdGetCurrentDisplayInformation: semântica real pendente (issue #16)
void __imp__VdGetCurrentDisplayInformation(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdGetCurrentDisplayInformation, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdGetSystemCommandBuffer: semântica real pendente (issue #16)
void __imp__VdGetSystemCommandBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdGetSystemCommandBuffer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdInitializeEngines: semântica real pendente (issue #16)
void __imp__VdInitializeEngines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdInitializeEngines, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdInitializeRingBuffer: semântica real pendente (issue #16)
void __imp__VdInitializeRingBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdInitializeRingBuffer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdInitializeScalerCommandBuffer: semântica real pendente (issue #16)
void __imp__VdInitializeScalerCommandBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdInitializeScalerCommandBuffer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdIsHSIOTrainingSucceeded: semântica real pendente (issue #16)
void __imp__VdIsHSIOTrainingSucceeded(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdIsHSIOTrainingSucceeded, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdPersistDisplay: semântica real pendente (issue #16)
void __imp__VdPersistDisplay(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdPersistDisplay, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdQueryVideoFlags: semântica real pendente (issue #16)
void __imp__VdQueryVideoFlags(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdQueryVideoFlags, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdQueryVideoMode: semântica real pendente (issue #16)
void __imp__VdQueryVideoMode(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdQueryVideoMode, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdRetrainEDRAM: semântica real pendente (issue #16)
void __imp__VdRetrainEDRAM(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdRetrainEDRAM, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdRetrainEDRAMWorker: semântica real pendente (issue #16)
void __imp__VdRetrainEDRAMWorker(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdRetrainEDRAMWorker, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdSetDisplayMode: semântica real pendente (issue #16)
void __imp__VdSetDisplayMode(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdSetDisplayMode, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdSetDisplayModeOverride: semântica real pendente (issue #16)
void __imp__VdSetDisplayModeOverride(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdSetDisplayModeOverride, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdSetGraphicsInterruptCallback: semântica real pendente (issue #16)
void __imp__VdSetGraphicsInterruptCallback(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdSetGraphicsInterruptCallback, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdSetSystemCommandBufferGpuIdentifierAddress: semântica real pendente (issue #16)
void __imp__VdSetSystemCommandBufferGpuIdentifierAddress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdSetSystemCommandBufferGpuIdentifierAddress, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdShutdownEngines: semântica real pendente (issue #16)
void __imp__VdShutdownEngines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdShutdownEngines, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// VdSwap: present do GPU — issue #17 (gráficos reais)
void __imp__VdSwap(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdSwap, "present do GPU — issue #17 (gráficos reais)");
    ctx.r3.u32 = 0;
}

// XAudioEnableDucker: semântica real pendente (issue #16)
void __imp__XAudioEnableDucker(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioEnableDucker, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioGetDuckerAttackTime: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerAttackTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioGetDuckerAttackTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioGetDuckerHoldTime: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerHoldTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioGetDuckerHoldTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioGetDuckerLevel: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerLevel(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioGetDuckerLevel, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioGetDuckerReleaseTime: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerReleaseTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioGetDuckerReleaseTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioGetDuckerThreshold: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerThreshold(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioGetDuckerThreshold, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioGetSpeakerConfig: semântica real pendente (issue #16)
void __imp__XAudioGetSpeakerConfig(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioGetSpeakerConfig, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioGetVoiceCategoryVolume: semântica real pendente (issue #16)
void __imp__XAudioGetVoiceCategoryVolume(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioGetVoiceCategoryVolume, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioRegisterRenderDriverClient: semântica real pendente (issue #16)
void __imp__XAudioRegisterRenderDriverClient(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioRegisterRenderDriverClient, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XAudioSubmitRenderDriverFrame: áudio do guest — issue #18
void __imp__XAudioSubmitRenderDriverFrame(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioSubmitRenderDriverFrame, "áudio do guest — issue #18");
    ctx.r3.u32 = 0;
}

// XAudioUnregisterRenderDriverClient: semântica real pendente (issue #16)
void __imp__XAudioUnregisterRenderDriverClient(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XAudioUnregisterRenderDriverClient, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XCustomGetCurrentGamercard: semântica real pendente (issue #16)
void __imp__XCustomGetCurrentGamercard(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XCustomGetCurrentGamercard, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XCustomGetLastActionPressEx: semântica real pendente (issue #16)
void __imp__XCustomGetLastActionPressEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XCustomGetLastActionPressEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XCustomRegisterDynamicActions: semântica real pendente (issue #16)
void __imp__XCustomRegisterDynamicActions(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XCustomRegisterDynamicActions, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XCustomSetDynamicActions: semântica real pendente (issue #16)
void __imp__XCustomSetDynamicActions(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XCustomSetDynamicActions, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XCustomUnregisterDynamicActions: semântica real pendente (issue #16)
void __imp__XCustomUnregisterDynamicActions(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XCustomUnregisterDynamicActions, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XGetAVPack: semântica real pendente (issue #16)
void __imp__XGetAVPack(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XGetAVPack, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XGetGameRegion: semântica real pendente (issue #16)
void __imp__XGetGameRegion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XGetGameRegion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XGetVideoMode: semântica real pendente (issue #16)
void __imp__XGetVideoMode(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XGetVideoMode, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMACreateContext: semântica real pendente (issue #16)
void __imp__XMACreateContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMACreateContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMAReleaseContext: semântica real pendente (issue #16)
void __imp__XMAReleaseContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMAReleaseContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMsgCancelIORequest: semântica real pendente (issue #16)
void __imp__XMsgCancelIORequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMsgCancelIORequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMsgCompleteIORequest: semântica real pendente (issue #16)
void __imp__XMsgCompleteIORequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMsgCompleteIORequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMsgInProcessCall: semântica real pendente (issue #16)
void __imp__XMsgInProcessCall(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMsgInProcessCall, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMsgStartIORequest: semântica real pendente (issue #16)
void __imp__XMsgStartIORequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMsgStartIORequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMsgStartIORequestEx: semântica real pendente (issue #16)
void __imp__XMsgStartIORequestEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMsgStartIORequestEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XMsgSystemProcessCall: semântica real pendente (issue #16)
void __imp__XMsgSystemProcessCall(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XMsgSystemProcessCall, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XNetLogonGetMachineID: semântica real pendente (issue #16)
void __imp__XNetLogonGetMachineID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XNetLogonGetMachineID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XNetLogonGetTitleID: semântica real pendente (issue #16)
void __imp__XNetLogonGetTitleID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XNetLogonGetTitleID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XNotifyGetNext: semântica real pendente (issue #16)
void __imp__XNotifyGetNext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XNotifyGetNext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamAlloc: semântica real pendente (issue #16)
void __imp__XamAlloc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamAlloc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamCacheCloseFile: semântica real pendente (issue #16)
void __imp__XamCacheCloseFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamCacheCloseFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamCacheOpenFile: falha REAL — cache pendente
void __imp__XamCacheOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamCacheOpenFile, "cache pendente");
    ctx.r3.u32 = 0x803500F1u;
}

// XamCacheReset: semântica real pendente (issue #16)
void __imp__XamCacheReset(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamCacheReset, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentClose: semântica real pendente (issue #16)
void __imp__XamContentClose(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentClose, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentCreateEnumerator: semântica real pendente (issue #16)
void __imp__XamContentCreateEnumerator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentCreateEnumerator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentCreateEx: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentCreateEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentCreateEx, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
}

// XamContentDelete: semântica real pendente (issue #16)
void __imp__XamContentDelete(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentDelete, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentFlush: semântica real pendente (issue #16)
void __imp__XamContentFlush(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentFlush, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentGetCreator: semântica real pendente (issue #16)
void __imp__XamContentGetCreator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentGetCreator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentGetDeviceData: semântica real pendente (issue #16)
void __imp__XamContentGetDeviceData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentGetDeviceData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentGetDeviceState: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentGetDeviceState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentGetDeviceState, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
}

// XamContentGetLicenseMask: semântica real pendente (issue #16)
void __imp__XamContentGetLicenseMask(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentGetLicenseMask, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentOpenFile: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentOpenFile, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
}

// XamContentResolve: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentResolve(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentResolve, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
}

// XamCreateEnumeratorHandle: semântica real pendente (issue #16)
void __imp__XamCreateEnumeratorHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamCreateEnumeratorHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamEnableInactivityProcessing: semântica real pendente (issue #16)
void __imp__XamEnableInactivityProcessing(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamEnableInactivityProcessing, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamEnumerate: semântica real pendente (issue #16)
void __imp__XamEnumerate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamEnumerate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamFree: semântica real pendente (issue #16)
void __imp__XamFree(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamFree, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamGetActiveDashAppInfo: semântica real pendente (issue #16)
void __imp__XamGetActiveDashAppInfo(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetActiveDashAppInfo, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamGetCurrentTitleId: semântica REAL (kernel_real.cpp)
void __imp__XamGetCurrentTitleId(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_XamGetCurrentTitleId(ctx, base);
}

// XamGetExecutionId: semântica real pendente (issue #16)
void __imp__XamGetExecutionId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetExecutionId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamGetLanguage: semântica real pendente (issue #16)
void __imp__XamGetLanguage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetLanguage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamGetLocaleEx: semântica real pendente (issue #16)
void __imp__XamGetLocaleEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetLocaleEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamGetOverlappedResult: semântica real pendente (issue #16)
void __imp__XamGetOverlappedResult(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetOverlappedResult, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamGetPrivateEnumStructureFromHandle: semântica real pendente (issue #16)
void __imp__XamGetPrivateEnumStructureFromHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetPrivateEnumStructureFromHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamGetSystemVersion: semântica real pendente (issue #16)
void __imp__XamGetSystemVersion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetSystemVersion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamInputGetCapabilities: semântica REAL (kernel_real.cpp)
void __imp__XamInputGetCapabilities(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_XamInputGetCapabilities(ctx, base);
}

// XamInputGetCapabilitiesEx: semântica REAL (kernel_real.cpp)
void __imp__XamInputGetCapabilitiesEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_XamInputGetCapabilitiesEx(ctx, base);
}

// XamInputGetState: semântica REAL (kernel_real.cpp)
void __imp__XamInputGetState(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_XamInputGetState(ctx, base);
}

// XamInputRawState: semântica real pendente (issue #16)
void __imp__XamInputRawState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamInputRawState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamInputSetState: semântica REAL (kernel_real.cpp)
void __imp__XamInputSetState(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_XamInputSetState(ctx, base);
}

// XamIsUIActive: semântica real pendente (issue #16)
void __imp__XamIsUIActive(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamIsUIActive, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLoaderGetLaunchData: semântica real pendente (issue #16)
void __imp__XamLoaderGetLaunchData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLoaderGetLaunchData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLoaderGetLaunchDataSize: semântica real pendente (issue #16)
void __imp__XamLoaderGetLaunchDataSize(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLoaderGetLaunchDataSize, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLoaderLaunchTitle: semântica real pendente (issue #16)
void __imp__XamLoaderLaunchTitle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLoaderLaunchTitle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLoaderSetLaunchData: semântica real pendente (issue #16)
void __imp__XamLoaderSetLaunchData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLoaderSetLaunchData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLoaderTerminateTitle: semântica real pendente (issue #16)
void __imp__XamLoaderTerminateTitle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLoaderTerminateTitle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLrcEncryptDecryptTitleMessage: semântica real pendente (issue #16)
void __imp__XamLrcEncryptDecryptTitleMessage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLrcEncryptDecryptTitleMessage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLrcLogError: semântica real pendente (issue #16)
void __imp__XamLrcLogError(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLrcLogError, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLrcLogSessionSummary: semântica real pendente (issue #16)
void __imp__XamLrcLogSessionSummary(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLrcLogSessionSummary, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLrcSetTitlePort: semântica real pendente (issue #16)
void __imp__XamLrcSetTitlePort(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLrcSetTitlePort, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamLrcVerifyClientId: semântica real pendente (issue #16)
void __imp__XamLrcVerifyClientId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamLrcVerifyClientId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNotifyCreateListener: semântica real pendente (issue #16)
void __imp__XamNotifyCreateListener(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNotifyCreateListener, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiCameraElevationGetAngle: semântica real pendente (issue #16)
void __imp__XamNuiCameraElevationGetAngle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiCameraElevationGetAngle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiCameraElevationSetAngle: semântica real pendente (issue #16)
void __imp__XamNuiCameraElevationSetAngle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiCameraElevationSetAngle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiCameraElevationStopMovement: semântica real pendente (issue #16)
void __imp__XamNuiCameraElevationStopMovement(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiCameraElevationStopMovement, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiCameraRememberFloor: semântica real pendente (issue #16)
void __imp__XamNuiCameraRememberFloor(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiCameraRememberFloor, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiCameraTiltGetStatus: semântica real pendente (issue #16)
void __imp__XamNuiCameraTiltGetStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiCameraTiltGetStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiCameraTiltReportStatus: semântica real pendente (issue #16)
void __imp__XamNuiCameraTiltReportStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiCameraTiltReportStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiCameraTiltSetCallback: semântica real pendente (issue #16)
void __imp__XamNuiCameraTiltSetCallback(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiCameraTiltSetCallback, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiGetDeviceStatus: semântica real pendente (issue #16)
void __imp__XamNuiGetDeviceStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiGetDeviceStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamNuiIdentityGetSessionId: semântica real pendente (issue #16)
void __imp__XamNuiIdentityGetSessionId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamNuiIdentityGetSessionId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamParseGamerTileKey: semântica real pendente (issue #16)
void __imp__XamParseGamerTileKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamParseGamerTileKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamProfileCreateEnumerator: semântica real pendente (issue #16)
void __imp__XamProfileCreateEnumerator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamProfileCreateEnumerator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamProfileEnumerate: semântica real pendente (issue #16)
void __imp__XamProfileEnumerate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamProfileEnumerate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamReadBiometricData: semântica real pendente (issue #16)
void __imp__XamReadBiometricData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamReadBiometricData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamReadTileToTexture: semântica real pendente (issue #16)
void __imp__XamReadTileToTexture(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamReadTileToTexture, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamResetInactivity: semântica real pendente (issue #16)
void __imp__XamResetInactivity(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamResetInactivity, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamSessionCreateHandle: semântica real pendente (issue #16)
void __imp__XamSessionCreateHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamSessionCreateHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamSessionRefObjByHandle: semântica real pendente (issue #16)
void __imp__XamSessionRefObjByHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamSessionRefObjByHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowAchievementsUI: semântica real pendente (issue #16)
void __imp__XamShowAchievementsUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowAchievementsUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowDeviceSelectorUI: semântica real pendente (issue #16)
void __imp__XamShowDeviceSelectorUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowDeviceSelectorUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowDirtyDiscErrorUI: semântica real pendente (issue #16)
void __imp__XamShowDirtyDiscErrorUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowDirtyDiscErrorUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowFriendsUI: semântica real pendente (issue #16)
void __imp__XamShowFriendsUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowFriendsUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowGameInviteUI: semântica real pendente (issue #16)
void __imp__XamShowGameInviteUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowGameInviteUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowGamerCardUIForXUID: semântica real pendente (issue #16)
void __imp__XamShowGamerCardUIForXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowGamerCardUIForXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowKeyboardUI: semântica real pendente (issue #16)
void __imp__XamShowKeyboardUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowKeyboardUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowMarketplaceDownloadItemsUI: semântica real pendente (issue #16)
void __imp__XamShowMarketplaceDownloadItemsUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowMarketplaceDownloadItemsUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowMarketplaceUI: semântica real pendente (issue #16)
void __imp__XamShowMarketplaceUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowMarketplaceUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowMessageBoxUI: semântica real pendente (issue #16)
void __imp__XamShowMessageBoxUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowMessageBoxUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowMessageBoxUIEx: semântica real pendente (issue #16)
void __imp__XamShowMessageBoxUIEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowMessageBoxUIEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowNuiDeviceSelectorUI: semântica real pendente (issue #16)
void __imp__XamShowNuiDeviceSelectorUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowNuiDeviceSelectorUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowNuiGuideUI: semântica real pendente (issue #16)
void __imp__XamShowNuiGuideUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowNuiGuideUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowNuiMessageBoxUI: semântica real pendente (issue #16)
void __imp__XamShowNuiMessageBoxUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowNuiMessageBoxUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowNuiSigninUI: semântica real pendente (issue #16)
void __imp__XamShowNuiSigninUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowNuiSigninUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowNuiTroubleshooterUI: semântica real pendente (issue #16)
void __imp__XamShowNuiTroubleshooterUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowNuiTroubleshooterUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowPlayersUI: semântica real pendente (issue #16)
void __imp__XamShowPlayersUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowPlayersUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamShowSigninUI: semântica real pendente (issue #16)
void __imp__XamShowSigninUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamShowSigninUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamTaskCloseHandle: semântica real pendente (issue #16)
void __imp__XamTaskCloseHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamTaskCloseHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamTaskSchedule: semântica real pendente (issue #16)
void __imp__XamTaskSchedule(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamTaskSchedule, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamTaskShouldExit: semântica real pendente (issue #16)
void __imp__XamTaskShouldExit(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamTaskShouldExit, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserAreUsersFriends: semântica real pendente (issue #16)
void __imp__XamUserAreUsersFriends(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserAreUsersFriends, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserCheckPrivilege: semântica real pendente (issue #16)
void __imp__XamUserCheckPrivilege(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserCheckPrivilege, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserCreateAchievementEnumerator: semântica real pendente (issue #16)
void __imp__XamUserCreateAchievementEnumerator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserCreateAchievementEnumerator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetAgeGroup: semântica real pendente (issue #16)
void __imp__XamUserGetAgeGroup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetAgeGroup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetDeviceContext: semântica real pendente (issue #16)
void __imp__XamUserGetDeviceContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetDeviceContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetIndexFromXUID: semântica real pendente (issue #16)
void __imp__XamUserGetIndexFromXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetIndexFromXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetMembershipTierFromXUID: semântica real pendente (issue #16)
void __imp__XamUserGetMembershipTierFromXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetMembershipTierFromXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetName: falha REAL — sem perfil assinado (offline real)
void __imp__XamUserGetName(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetName, "sem perfil assinado (offline real)");
    ctx.r3.u32 = 0x80320098u;
}

// XamUserGetOnlineCountryFromXUID: semântica real pendente (issue #16)
void __imp__XamUserGetOnlineCountryFromXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetOnlineCountryFromXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetSigninInfo: falha REAL — sem perfil assinado (offline real)
void __imp__XamUserGetSigninInfo(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetSigninInfo, "sem perfil assinado (offline real)");
    ctx.r3.u32 = 0x80320098u;
}

// XamUserGetSigninState: semântica REAL (kernel_real.cpp)
void __imp__XamUserGetSigninState(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_XamUserGetSigninState(ctx, base);
}

// XamUserGetXUID: falha REAL — sem perfil assinado (offline real)
void __imp__XamUserGetXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetXUID, "sem perfil assinado (offline real)");
    ctx.r3.u32 = 0x80320098u;
}

// XamUserNuiEnableBiometric: semântica real pendente (issue #16)
void __imp__XamUserNuiEnableBiometric(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserNuiEnableBiometric, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserNuiGetEnrollmentIndex: semântica real pendente (issue #16)
void __imp__XamUserNuiGetEnrollmentIndex(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserNuiGetEnrollmentIndex, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserNuiGetUserIndex: semântica real pendente (issue #16)
void __imp__XamUserNuiGetUserIndex(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserNuiGetUserIndex, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserReadProfileSettings: semântica real pendente (issue #16)
void __imp__XamUserReadProfileSettings(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserReadProfileSettings, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamVoiceClose: semântica real pendente (issue #16)
void __imp__XamVoiceClose(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamVoiceClose, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamVoiceCreate: semântica real pendente (issue #16)
void __imp__XamVoiceCreate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamVoiceCreate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamVoiceHeadsetPresent: semântica real pendente (issue #16)
void __imp__XamVoiceHeadsetPresent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamVoiceHeadsetPresent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamVoiceIsActiveProcess: semântica real pendente (issue #16)
void __imp__XamVoiceIsActiveProcess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamVoiceIsActiveProcess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamVoiceSubmitPacket: semântica real pendente (issue #16)
void __imp__XamVoiceSubmitPacket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamVoiceSubmitPacket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamWriteBiometricData: semântica real pendente (issue #16)
void __imp__XamWriteBiometricData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamWriteBiometricData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamXStudioRequest: semântica real pendente (issue #16)
void __imp__XamXStudioRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamXStudioRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamXlfsInitializeUploadQueue: semântica real pendente (issue #16)
void __imp__XamXlfsInitializeUploadQueue(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamXlfsInitializeUploadQueue, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamXlfsMountUploadQueueInstance: semântica real pendente (issue #16)
void __imp__XamXlfsMountUploadQueueInstance(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamXlfsMountUploadQueueInstance, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamXlfsUninitializeUploadQueue: semântica real pendente (issue #16)
void __imp__XamXlfsUninitializeUploadQueue(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamXlfsUninitializeUploadQueue, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamXlfsUnmountUploadQueueInstance: semântica real pendente (issue #16)
void __imp__XamXlfsUnmountUploadQueueInstance(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamXlfsUnmountUploadQueueInstance, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XampXAuthIsLocalSocketAllowed: semântica real pendente (issue #16)
void __imp__XampXAuthIsLocalSocketAllowed(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XampXAuthIsLocalSocketAllowed, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha: semântica real pendente (issue #16)
void __imp__XeCryptSha(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha256Final: semântica real pendente (issue #16)
void __imp__XeCryptSha256Final(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha256Final, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha256Init: semântica real pendente (issue #16)
void __imp__XeCryptSha256Init(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha256Init, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha256Update: semântica real pendente (issue #16)
void __imp__XeCryptSha256Update(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha256Update, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha384Final: semântica real pendente (issue #16)
void __imp__XeCryptSha384Final(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha384Final, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha384Init: semântica real pendente (issue #16)
void __imp__XeCryptSha384Init(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha384Init, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha384Update: semântica real pendente (issue #16)
void __imp__XeCryptSha384Update(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha384Update, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha512Final: semântica real pendente (issue #16)
void __imp__XeCryptSha512Final(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha512Final, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha512Init: semântica real pendente (issue #16)
void __imp__XeCryptSha512Init(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha512Init, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptSha512Update: semântica real pendente (issue #16)
void __imp__XeCryptSha512Update(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptSha512Update, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptShaFinal: semântica real pendente (issue #16)
void __imp__XeCryptShaFinal(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptShaFinal, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptShaInit: semântica real pendente (issue #16)
void __imp__XeCryptShaInit(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptShaInit, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeCryptShaUpdate: semântica real pendente (issue #16)
void __imp__XeCryptShaUpdate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeCryptShaUpdate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeKeysAesCbcUsingKey: semântica real pendente (issue #16)
void __imp__XeKeysAesCbcUsingKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeKeysAesCbcUsingKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeKeysConsolePrivateKeySign: semântica real pendente (issue #16)
void __imp__XeKeysConsolePrivateKeySign(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeKeysConsolePrivateKeySign, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeKeysConsoleSignatureVerification: semântica real pendente (issue #16)
void __imp__XeKeysConsoleSignatureVerification(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeKeysConsoleSignatureVerification, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeKeysGetConsoleID: semântica real pendente (issue #16)
void __imp__XeKeysGetConsoleID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeKeysGetConsoleID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XeKeysObscureKey: semântica real pendente (issue #16)
void __imp__XeKeysObscureKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XeKeysObscureKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XexCheckExecutablePrivilege: semântica real pendente (issue #16)
void __imp__XexCheckExecutablePrivilege(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexCheckExecutablePrivilege, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XexGetModuleHandle: semântica real pendente (issue #16)
void __imp__XexGetModuleHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexGetModuleHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XexGetModuleSection: semântica real pendente (issue #16)
void __imp__XexGetModuleSection(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexGetModuleSection, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XexGetProcedureAddress: falha REAL — STATUS_PROCEDURE_NOT_FOUND — export tables pendentes
void __imp__XexGetProcedureAddress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexGetProcedureAddress, "STATUS_PROCEDURE_NOT_FOUND — export tables pendentes");
    ctx.r3.u32 = 0x8007007Eu;
}

// XexLoadImage: semântica real pendente (issue #16)
void __imp__XexLoadImage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexLoadImage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XexLoadImageHeaders: semântica real pendente (issue #16)
void __imp__XexLoadImageHeaders(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexLoadImageHeaders, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XexUnloadImage: semântica real pendente (issue #16)
void __imp__XexUnloadImage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexUnloadImage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// __C_specific_handler: semântica real pendente (issue #16)
void __imp____C_specific_handler(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(__C_specific_handler, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// _snprintf: semântica REAL (kernel_real.cpp)
void __imp___snprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real__snprintf(ctx, base);
}

// _vsnprintf: semântica REAL (kernel_real.cpp)
void __imp___vsnprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real__vsnprintf(ctx, base);
}

// sprintf: semântica REAL (kernel_real.cpp)
void __imp__sprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_sprintf(ctx, base);
}

// vsprintf: semântica REAL (kernel_real.cpp)
void __imp__vsprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_vsprintf(ctx, base);
}

// vswprintf: semântica REAL (kernel_real.cpp)
void __imp__vswprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::real_vswprintf(ctx, base);
}

#endif // FH2_HAS_RECOMP
