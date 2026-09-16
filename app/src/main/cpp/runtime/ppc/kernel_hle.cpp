// kernel_hle.cpp — despacho HLE dos imports de xboxkrnl.exe/xam.xex (FH2)
//
// GERADO por tools/gen_kernel_hle.py — 388 símbolos exigidos por
// ppc_func_mapping.cpp. Três caminhos:
//   REAL  (78): delega para fh2::kern::real_* com semântica real
//         (heap, tempo, threads, sync, TLS, printf, input — kernel_real.cpp)
//   FAIL  (26): retorna status de falha REAL do estado do sistema
//   stub  (284): log-once + NTSTATUS 0 (semântica pendente — issue #16)
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
    fh2::kern::traceCall("DbgBreakPoint", ctx);
    fh2::kern::real_DbgBreakPoint(ctx, base);
    fh2::kern::traceReturn("DbgBreakPoint", ctx);
}

// DbgPrint: semântica REAL (kernel_real.cpp)
void __imp__DbgPrint(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("DbgPrint", ctx);
    fh2::kern::real_DbgPrint(ctx, base);
    fh2::kern::traceReturn("DbgPrint", ctx);
}

// EtxProducerLog: semântica real pendente (issue #16)
void __imp__EtxProducerLog(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("EtxProducerLog", ctx);
    FH2_HLE_ONCE(EtxProducerLog, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("EtxProducerLog", ctx);
}

// EtxProducerRegister: semântica real pendente (issue #16)
void __imp__EtxProducerRegister(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("EtxProducerRegister", ctx);
    FH2_HLE_ONCE(EtxProducerRegister, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("EtxProducerRegister", ctx);
}

// EtxProducerUnregister: semântica real pendente (issue #16)
void __imp__EtxProducerUnregister(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("EtxProducerUnregister", ctx);
    FH2_HLE_ONCE(EtxProducerUnregister, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("EtxProducerUnregister", ctx);
}

// ExAllocatePool: semântica REAL (kernel_real.cpp)
void __imp__ExAllocatePool(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("ExAllocatePool", ctx);
    fh2::kern::real_ExAllocatePool(ctx, base);
    fh2::kern::traceReturn("ExAllocatePool", ctx);
}

// ExAllocatePoolTypeWithTag: semântica REAL (kernel_real.cpp)
void __imp__ExAllocatePoolTypeWithTag(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("ExAllocatePoolTypeWithTag", ctx);
    fh2::kern::real_ExAllocatePoolTypeWithTag(ctx, base);
    fh2::kern::traceReturn("ExAllocatePoolTypeWithTag", ctx);
}

// ExCreateThread: semântica REAL (kernel_real.cpp)
void __imp__ExCreateThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("ExCreateThread", ctx);
    fh2::kern::real_ExCreateThread(ctx, base);
    fh2::kern::traceReturn("ExCreateThread", ctx);
}

// ExFreePool: semântica REAL (kernel_real.cpp)
void __imp__ExFreePool(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("ExFreePool", ctx);
    fh2::kern::real_ExFreePool(ctx, base);
    fh2::kern::traceReturn("ExFreePool", ctx);
}

// ExGetXConfigSetting: semântica real pendente (issue #16)
void __imp__ExGetXConfigSetting(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ExGetXConfigSetting", ctx);
    FH2_HLE_ONCE(ExGetXConfigSetting, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ExGetXConfigSetting", ctx);
}

// ExRegisterTitleTerminateNotification: semântica real pendente (issue #16)
void __imp__ExRegisterTitleTerminateNotification(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ExRegisterTitleTerminateNotification", ctx);
    FH2_HLE_ONCE(ExRegisterTitleTerminateNotification, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ExRegisterTitleTerminateNotification", ctx);
}

// ExTerminateThread: semântica REAL (kernel_real.cpp)
void __imp__ExTerminateThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("ExTerminateThread", ctx);
    fh2::kern::real_ExTerminateThread(ctx, base);
    fh2::kern::traceReturn("ExTerminateThread", ctx);
}

// FscSetCacheElementCount: semântica real pendente (issue #16)
void __imp__FscSetCacheElementCount(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("FscSetCacheElementCount", ctx);
    FH2_HLE_ONCE(FscSetCacheElementCount, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("FscSetCacheElementCount", ctx);
}

// HalReturnToFirmware: semântica REAL (kernel_real.cpp)
void __imp__HalReturnToFirmware(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("HalReturnToFirmware", ctx);
    fh2::kern::real_HalReturnToFirmware(ctx, base);
    fh2::kern::traceReturn("HalReturnToFirmware", ctx);
}

// InterlockedFlushSList: semântica real pendente (issue #16)
void __imp__InterlockedFlushSList(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("InterlockedFlushSList", ctx);
    FH2_HLE_ONCE(InterlockedFlushSList, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("InterlockedFlushSList", ctx);
}

// InterlockedPopEntrySList: semântica real pendente (issue #16)
void __imp__InterlockedPopEntrySList(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("InterlockedPopEntrySList", ctx);
    FH2_HLE_ONCE(InterlockedPopEntrySList, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("InterlockedPopEntrySList", ctx);
}

// IoCheckShareAccess: semântica real pendente (issue #16)
void __imp__IoCheckShareAccess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoCheckShareAccess", ctx);
    FH2_HLE_ONCE(IoCheckShareAccess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoCheckShareAccess", ctx);
}

// IoCompleteRequest: semântica real pendente (issue #16)
void __imp__IoCompleteRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoCompleteRequest", ctx);
    FH2_HLE_ONCE(IoCompleteRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoCompleteRequest", ctx);
}

// IoCreateDevice: semântica real pendente (issue #16)
void __imp__IoCreateDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoCreateDevice", ctx);
    FH2_HLE_ONCE(IoCreateDevice, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoCreateDevice", ctx);
}

// IoDeleteDevice: semântica real pendente (issue #16)
void __imp__IoDeleteDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoDeleteDevice", ctx);
    FH2_HLE_ONCE(IoDeleteDevice, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoDeleteDevice", ctx);
}

// IoDismountVolume: semântica real pendente (issue #16)
void __imp__IoDismountVolume(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoDismountVolume", ctx);
    FH2_HLE_ONCE(IoDismountVolume, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoDismountVolume", ctx);
}

// IoDismountVolumeByFileHandle: semântica real pendente (issue #16)
void __imp__IoDismountVolumeByFileHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoDismountVolumeByFileHandle", ctx);
    FH2_HLE_ONCE(IoDismountVolumeByFileHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoDismountVolumeByFileHandle", ctx);
}

// IoInvalidDeviceRequest: semântica real pendente (issue #16)
void __imp__IoInvalidDeviceRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoInvalidDeviceRequest", ctx);
    FH2_HLE_ONCE(IoInvalidDeviceRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoInvalidDeviceRequest", ctx);
}

// IoRemoveShareAccess: semântica real pendente (issue #16)
void __imp__IoRemoveShareAccess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoRemoveShareAccess", ctx);
    FH2_HLE_ONCE(IoRemoveShareAccess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoRemoveShareAccess", ctx);
}

// IoSetShareAccess: semântica real pendente (issue #16)
void __imp__IoSetShareAccess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("IoSetShareAccess", ctx);
    FH2_HLE_ONCE(IoSetShareAccess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("IoSetShareAccess", ctx);
}

// KeAcquireSpinLockAtRaisedIrql: semântica real pendente (issue #16)
void __imp__KeAcquireSpinLockAtRaisedIrql(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeAcquireSpinLockAtRaisedIrql", ctx);
    FH2_HLE_ONCE(KeAcquireSpinLockAtRaisedIrql, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeAcquireSpinLockAtRaisedIrql", ctx);
}

// KeBugCheck: semântica REAL (kernel_real.cpp)
void __imp__KeBugCheck(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeBugCheck", ctx);
    fh2::kern::real_KeBugCheck(ctx, base);
    fh2::kern::traceReturn("KeBugCheck", ctx);
}

// KeBugCheckEx: semântica REAL (kernel_real.cpp)
void __imp__KeBugCheckEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeBugCheckEx", ctx);
    fh2::kern::real_KeBugCheckEx(ctx, base);
    fh2::kern::traceReturn("KeBugCheckEx", ctx);
}

// KeDelayExecutionThread: semântica REAL (kernel_real.cpp)
void __imp__KeDelayExecutionThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeDelayExecutionThread", ctx);
    fh2::kern::real_KeDelayExecutionThread(ctx, base);
    fh2::kern::traceReturn("KeDelayExecutionThread", ctx);
}

// KeEnterCriticalRegion: semântica real pendente (issue #16)
void __imp__KeEnterCriticalRegion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeEnterCriticalRegion", ctx);
    FH2_HLE_ONCE(KeEnterCriticalRegion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeEnterCriticalRegion", ctx);
}

// KeGetCurrentProcessType: semântica REAL (kernel_real.cpp)
void __imp__KeGetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeGetCurrentProcessType", ctx);
    fh2::kern::real_KeGetCurrentProcessType(ctx, base);
    fh2::kern::traceReturn("KeGetCurrentProcessType", ctx);
}

// KeInitializeDpc: semântica real pendente (issue #16)
void __imp__KeInitializeDpc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeInitializeDpc", ctx);
    FH2_HLE_ONCE(KeInitializeDpc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeInitializeDpc", ctx);
}

// KeInitializeMutant: semântica REAL (kernel_real.cpp)
void __imp__KeInitializeMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeInitializeMutant", ctx);
    fh2::kern::real_KeInitializeMutant(ctx, base);
    fh2::kern::traceReturn("KeInitializeMutant", ctx);
}

// KeInsertQueueDpc: semântica real pendente (issue #16)
void __imp__KeInsertQueueDpc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeInsertQueueDpc", ctx);
    FH2_HLE_ONCE(KeInsertQueueDpc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeInsertQueueDpc", ctx);
}

// KeLeaveCriticalRegion: semântica real pendente (issue #16)
void __imp__KeLeaveCriticalRegion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeLeaveCriticalRegion", ctx);
    FH2_HLE_ONCE(KeLeaveCriticalRegion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeLeaveCriticalRegion", ctx);
}

// KeLockL2: semântica real pendente (issue #16)
void __imp__KeLockL2(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeLockL2", ctx);
    FH2_HLE_ONCE(KeLockL2, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeLockL2", ctx);
}

// KeQueryBasePriorityThread: semântica real pendente (issue #16)
void __imp__KeQueryBasePriorityThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeQueryBasePriorityThread", ctx);
    FH2_HLE_ONCE(KeQueryBasePriorityThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeQueryBasePriorityThread", ctx);
}

// KeQueryPerformanceFrequency: semântica REAL (kernel_real.cpp)
void __imp__KeQueryPerformanceFrequency(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeQueryPerformanceFrequency", ctx);
    fh2::kern::real_KeQueryPerformanceFrequency(ctx, base);
    fh2::kern::traceReturn("KeQueryPerformanceFrequency", ctx);
}

// KeQuerySystemTime: semântica REAL (kernel_real.cpp)
void __imp__KeQuerySystemTime(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeQuerySystemTime", ctx);
    fh2::kern::real_KeQuerySystemTime(ctx, base);
    fh2::kern::traceReturn("KeQuerySystemTime", ctx);
}

// KeReleaseMutant: semântica REAL (kernel_real.cpp)
void __imp__KeReleaseMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeReleaseMutant", ctx);
    fh2::kern::real_KeReleaseMutant(ctx, base);
    fh2::kern::traceReturn("KeReleaseMutant", ctx);
}

// KeReleaseSpinLockFromRaisedIrql: semântica real pendente (issue #16)
void __imp__KeReleaseSpinLockFromRaisedIrql(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeReleaseSpinLockFromRaisedIrql", ctx);
    FH2_HLE_ONCE(KeReleaseSpinLockFromRaisedIrql, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeReleaseSpinLockFromRaisedIrql", ctx);
}

// KeResetEvent: semântica REAL (kernel_real.cpp)
void __imp__KeResetEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeResetEvent", ctx);
    fh2::kern::real_KeResetEvent(ctx, base);
    fh2::kern::traceReturn("KeResetEvent", ctx);
}

// KeRestoreFloatingPointState: semântica real pendente (issue #16)
void __imp__KeRestoreFloatingPointState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeRestoreFloatingPointState", ctx);
    FH2_HLE_ONCE(KeRestoreFloatingPointState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeRestoreFloatingPointState", ctx);
}

// KeResumeThread: semântica REAL (kernel_real.cpp)
void __imp__KeResumeThread(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeResumeThread", ctx);
    fh2::kern::real_KeResumeThread(ctx, base);
    fh2::kern::traceReturn("KeResumeThread", ctx);
}

// KeSaveFloatingPointState: semântica real pendente (issue #16)
void __imp__KeSaveFloatingPointState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeSaveFloatingPointState", ctx);
    FH2_HLE_ONCE(KeSaveFloatingPointState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeSaveFloatingPointState", ctx);
}

// KeSetAffinityThread: semântica real pendente (issue #16)
void __imp__KeSetAffinityThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeSetAffinityThread", ctx);
    FH2_HLE_ONCE(KeSetAffinityThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeSetAffinityThread", ctx);
}

// KeSetBasePriorityThread: semântica real pendente (issue #16)
void __imp__KeSetBasePriorityThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeSetBasePriorityThread", ctx);
    FH2_HLE_ONCE(KeSetBasePriorityThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeSetBasePriorityThread", ctx);
}

// KeSetCurrentProcessType: semântica REAL (kernel_real.cpp)
void __imp__KeSetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeSetCurrentProcessType", ctx);
    fh2::kern::real_KeSetCurrentProcessType(ctx, base);
    fh2::kern::traceReturn("KeSetCurrentProcessType", ctx);
}

// KeSetCurrentStackPointers: semântica real pendente (issue #16)
void __imp__KeSetCurrentStackPointers(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeSetCurrentStackPointers", ctx);
    FH2_HLE_ONCE(KeSetCurrentStackPointers, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeSetCurrentStackPointers", ctx);
}

// KeSetEvent: semântica REAL (kernel_real.cpp)
void __imp__KeSetEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeSetEvent", ctx);
    fh2::kern::real_KeSetEvent(ctx, base);
    fh2::kern::traceReturn("KeSetEvent", ctx);
}

// KeTlsAlloc: semântica REAL (kernel_real.cpp)
void __imp__KeTlsAlloc(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeTlsAlloc", ctx);
    fh2::kern::real_KeTlsAlloc(ctx, base);
    fh2::kern::traceReturn("KeTlsAlloc", ctx);
}

// KeTlsFree: semântica REAL (kernel_real.cpp)
void __imp__KeTlsFree(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeTlsFree", ctx);
    fh2::kern::real_KeTlsFree(ctx, base);
    fh2::kern::traceReturn("KeTlsFree", ctx);
}

// KeTlsGetValue: semântica REAL (kernel_real.cpp)
void __imp__KeTlsGetValue(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeTlsGetValue", ctx);
    fh2::kern::real_KeTlsGetValue(ctx, base);
    fh2::kern::traceReturn("KeTlsGetValue", ctx);
}

// KeTlsSetValue: semântica REAL (kernel_real.cpp)
void __imp__KeTlsSetValue(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeTlsSetValue", ctx);
    fh2::kern::real_KeTlsSetValue(ctx, base);
    fh2::kern::traceReturn("KeTlsSetValue", ctx);
}

// KeUnlockL2: semântica real pendente (issue #16)
void __imp__KeUnlockL2(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KeUnlockL2", ctx);
    FH2_HLE_ONCE(KeUnlockL2, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KeUnlockL2", ctx);
}

// KeWaitForMultipleObjects: semântica REAL (kernel_real.cpp)
void __imp__KeWaitForMultipleObjects(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeWaitForMultipleObjects", ctx);
    fh2::kern::real_KeWaitForMultipleObjects(ctx, base);
    fh2::kern::traceReturn("KeWaitForMultipleObjects", ctx);
}

// KeWaitForSingleObject: semântica REAL (kernel_real.cpp)
void __imp__KeWaitForSingleObject(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("KeWaitForSingleObject", ctx);
    fh2::kern::real_KeWaitForSingleObject(ctx, base);
    fh2::kern::traceReturn("KeWaitForSingleObject", ctx);
}

// KfAcquireSpinLock: semântica real pendente (issue #16)
void __imp__KfAcquireSpinLock(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KfAcquireSpinLock", ctx);
    FH2_HLE_ONCE(KfAcquireSpinLock, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KfAcquireSpinLock", ctx);
}

// KfReleaseSpinLock: semântica real pendente (issue #16)
void __imp__KfReleaseSpinLock(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KfReleaseSpinLock", ctx);
    FH2_HLE_ONCE(KfReleaseSpinLock, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KfReleaseSpinLock", ctx);
}

// KiApcNormalRoutineNop: semântica real pendente (issue #16)
void __imp__KiApcNormalRoutineNop(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("KiApcNormalRoutineNop", ctx);
    FH2_HLE_ONCE(KiApcNormalRoutineNop, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("KiApcNormalRoutineNop", ctx);
}

// LDICreateDecompression: semântica real pendente (issue #16)
void __imp__LDICreateDecompression(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("LDICreateDecompression", ctx);
    FH2_HLE_ONCE(LDICreateDecompression, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("LDICreateDecompression", ctx);
}

// LDIDecompress: semântica real pendente (issue #16)
void __imp__LDIDecompress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("LDIDecompress", ctx);
    FH2_HLE_ONCE(LDIDecompress, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("LDIDecompress", ctx);
}

// LDIDestroyDecompression: semântica real pendente (issue #16)
void __imp__LDIDestroyDecompression(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("LDIDestroyDecompression", ctx);
    FH2_HLE_ONCE(LDIDestroyDecompression, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("LDIDestroyDecompression", ctx);
}

// MmAllocatePhysicalMemoryEx: semântica REAL (kernel_real.cpp)
void __imp__MmAllocatePhysicalMemoryEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("MmAllocatePhysicalMemoryEx", ctx);
    fh2::kern::real_MmAllocatePhysicalMemoryEx(ctx, base);
    fh2::kern::traceReturn("MmAllocatePhysicalMemoryEx", ctx);
}

// MmCreateKernelStack: semântica REAL (kernel_real.cpp)
void __imp__MmCreateKernelStack(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("MmCreateKernelStack", ctx);
    fh2::kern::real_MmCreateKernelStack(ctx, base);
    fh2::kern::traceReturn("MmCreateKernelStack", ctx);
}

// MmDeleteKernelStack: semântica REAL (kernel_real.cpp)
void __imp__MmDeleteKernelStack(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("MmDeleteKernelStack", ctx);
    fh2::kern::real_MmDeleteKernelStack(ctx, base);
    fh2::kern::traceReturn("MmDeleteKernelStack", ctx);
}

// MmFreePhysicalMemory: semântica REAL (kernel_real.cpp)
void __imp__MmFreePhysicalMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("MmFreePhysicalMemory", ctx);
    fh2::kern::real_MmFreePhysicalMemory(ctx, base);
    fh2::kern::traceReturn("MmFreePhysicalMemory", ctx);
}

// MmGetPhysicalAddress: semântica REAL (kernel_real.cpp)
void __imp__MmGetPhysicalAddress(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("MmGetPhysicalAddress", ctx);
    fh2::kern::real_MmGetPhysicalAddress(ctx, base);
    fh2::kern::traceReturn("MmGetPhysicalAddress", ctx);
}

// MmMapIoSpace: semântica real pendente (issue #16)
void __imp__MmMapIoSpace(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("MmMapIoSpace", ctx);
    FH2_HLE_ONCE(MmMapIoSpace, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("MmMapIoSpace", ctx);
}

// MmQueryAddressProtect: semântica real pendente (issue #16)
void __imp__MmQueryAddressProtect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("MmQueryAddressProtect", ctx);
    FH2_HLE_ONCE(MmQueryAddressProtect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("MmQueryAddressProtect", ctx);
}

// MmQueryAllocationSize: semântica REAL (kernel_real.cpp)
void __imp__MmQueryAllocationSize(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("MmQueryAllocationSize", ctx);
    fh2::kern::real_MmQueryAllocationSize(ctx, base);
    fh2::kern::traceReturn("MmQueryAllocationSize", ctx);
}

// MmQueryStatistics: semântica real pendente (issue #16)
void __imp__MmQueryStatistics(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("MmQueryStatistics", ctx);
    FH2_HLE_ONCE(MmQueryStatistics, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("MmQueryStatistics", ctx);
}

// MmSetAddressProtect: semântica real pendente (issue #16)
void __imp__MmSetAddressProtect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("MmSetAddressProtect", ctx);
    FH2_HLE_ONCE(MmSetAddressProtect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("MmSetAddressProtect", ctx);
}

// NetDll_WSACancelOverlappedIO: semântica real pendente (issue #16)
void __imp__NetDll_WSACancelOverlappedIO(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSACancelOverlappedIO", ctx);
    FH2_HLE_ONCE(NetDll_WSACancelOverlappedIO, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSACancelOverlappedIO", ctx);
}

// NetDll_WSACleanup: semântica real pendente (issue #16)
void __imp__NetDll_WSACleanup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSACleanup", ctx);
    FH2_HLE_ONCE(NetDll_WSACleanup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSACleanup", ctx);
}

// NetDll_WSACloseEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSACloseEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSACloseEvent", ctx);
    FH2_HLE_ONCE(NetDll_WSACloseEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSACloseEvent", ctx);
}

// NetDll_WSACreateEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSACreateEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSACreateEvent", ctx);
    FH2_HLE_ONCE(NetDll_WSACreateEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSACreateEvent", ctx);
}

// NetDll_WSAEventSelect: semântica real pendente (issue #16)
void __imp__NetDll_WSAEventSelect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSAEventSelect", ctx);
    FH2_HLE_ONCE(NetDll_WSAEventSelect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSAEventSelect", ctx);
}

// NetDll_WSAGetLastError: semântica real pendente (issue #16)
void __imp__NetDll_WSAGetLastError(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSAGetLastError", ctx);
    FH2_HLE_ONCE(NetDll_WSAGetLastError, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSAGetLastError", ctx);
}

// NetDll_WSAGetOverlappedResult: semântica real pendente (issue #16)
void __imp__NetDll_WSAGetOverlappedResult(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSAGetOverlappedResult", ctx);
    FH2_HLE_ONCE(NetDll_WSAGetOverlappedResult, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSAGetOverlappedResult", ctx);
}

// NetDll_WSARecv: semântica real pendente (issue #16)
void __imp__NetDll_WSARecv(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSARecv", ctx);
    FH2_HLE_ONCE(NetDll_WSARecv, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSARecv", ctx);
}

// NetDll_WSARecvFrom: semântica real pendente (issue #16)
void __imp__NetDll_WSARecvFrom(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSARecvFrom", ctx);
    FH2_HLE_ONCE(NetDll_WSARecvFrom, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSARecvFrom", ctx);
}

// NetDll_WSAResetEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSAResetEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSAResetEvent", ctx);
    FH2_HLE_ONCE(NetDll_WSAResetEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSAResetEvent", ctx);
}

// NetDll_WSASend: semântica real pendente (issue #16)
void __imp__NetDll_WSASend(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSASend", ctx);
    FH2_HLE_ONCE(NetDll_WSASend, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSASend", ctx);
}

// NetDll_WSASendTo: semântica real pendente (issue #16)
void __imp__NetDll_WSASendTo(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSASendTo", ctx);
    FH2_HLE_ONCE(NetDll_WSASendTo, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSASendTo", ctx);
}

// NetDll_WSASetEvent: semântica real pendente (issue #16)
void __imp__NetDll_WSASetEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSASetEvent", ctx);
    FH2_HLE_ONCE(NetDll_WSASetEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSASetEvent", ctx);
}

// NetDll_WSASetLastError: semântica real pendente (issue #16)
void __imp__NetDll_WSASetLastError(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSASetLastError", ctx);
    FH2_HLE_ONCE(NetDll_WSASetLastError, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSASetLastError", ctx);
}

// NetDll_WSAStartup: falha REAL — rede real pendente (issue #19)
void __imp__NetDll_WSAStartup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSAStartup", ctx);
    FH2_HLE_ONCE(NetDll_WSAStartup, "rede real pendente (issue #19)");
    ctx.r3.u32 = 0x800704CFu;
    fh2::kern::traceReturn("NetDll_WSAStartup", ctx);
}

// NetDll_WSAWaitForMultipleEvents: semântica real pendente (issue #16)
void __imp__NetDll_WSAWaitForMultipleEvents(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_WSAWaitForMultipleEvents", ctx);
    FH2_HLE_ONCE(NetDll_WSAWaitForMultipleEvents, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_WSAWaitForMultipleEvents", ctx);
}

// NetDll_XNetCleanup: semântica real pendente (issue #16)
void __imp__NetDll_XNetCleanup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetCleanup", ctx);
    FH2_HLE_ONCE(NetDll_XNetCleanup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetCleanup", ctx);
}

// NetDll_XNetConnect: semântica real pendente (issue #16)
void __imp__NetDll_XNetConnect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetConnect", ctx);
    FH2_HLE_ONCE(NetDll_XNetConnect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetConnect", ctx);
}

// NetDll_XNetCreateKey: semântica real pendente (issue #16)
void __imp__NetDll_XNetCreateKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetCreateKey", ctx);
    FH2_HLE_ONCE(NetDll_XNetCreateKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetCreateKey", ctx);
}

// NetDll_XNetGetConnectStatus: semântica real pendente (issue #16)
void __imp__NetDll_XNetGetConnectStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetGetConnectStatus", ctx);
    FH2_HLE_ONCE(NetDll_XNetGetConnectStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetGetConnectStatus", ctx);
}

// NetDll_XNetGetEthernetLinkStatus: semântica real pendente (issue #16)
void __imp__NetDll_XNetGetEthernetLinkStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetGetEthernetLinkStatus", ctx);
    FH2_HLE_ONCE(NetDll_XNetGetEthernetLinkStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetGetEthernetLinkStatus", ctx);
}

// NetDll_XNetGetTitleXnAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetGetTitleXnAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetGetTitleXnAddr", ctx);
    FH2_HLE_ONCE(NetDll_XNetGetTitleXnAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetGetTitleXnAddr", ctx);
}

// NetDll_XNetInAddrToString: semântica real pendente (issue #16)
void __imp__NetDll_XNetInAddrToString(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetInAddrToString", ctx);
    FH2_HLE_ONCE(NetDll_XNetInAddrToString, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetInAddrToString", ctx);
}

// NetDll_XNetInAddrToXnAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetInAddrToXnAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetInAddrToXnAddr", ctx);
    FH2_HLE_ONCE(NetDll_XNetInAddrToXnAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetInAddrToXnAddr", ctx);
}

// NetDll_XNetQosListen: semântica real pendente (issue #16)
void __imp__NetDll_XNetQosListen(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetQosListen", ctx);
    FH2_HLE_ONCE(NetDll_XNetQosListen, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetQosListen", ctx);
}

// NetDll_XNetQosLookup: semântica real pendente (issue #16)
void __imp__NetDll_XNetQosLookup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetQosLookup", ctx);
    FH2_HLE_ONCE(NetDll_XNetQosLookup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetQosLookup", ctx);
}

// NetDll_XNetQosRelease: semântica real pendente (issue #16)
void __imp__NetDll_XNetQosRelease(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetQosRelease", ctx);
    FH2_HLE_ONCE(NetDll_XNetQosRelease, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetQosRelease", ctx);
}

// NetDll_XNetRandom: semântica real pendente (issue #16)
void __imp__NetDll_XNetRandom(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetRandom", ctx);
    FH2_HLE_ONCE(NetDll_XNetRandom, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetRandom", ctx);
}

// NetDll_XNetRegisterKey: semântica real pendente (issue #16)
void __imp__NetDll_XNetRegisterKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetRegisterKey", ctx);
    FH2_HLE_ONCE(NetDll_XNetRegisterKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetRegisterKey", ctx);
}

// NetDll_XNetServerToInAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetServerToInAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetServerToInAddr", ctx);
    FH2_HLE_ONCE(NetDll_XNetServerToInAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetServerToInAddr", ctx);
}

// NetDll_XNetStartup: semântica real pendente (issue #16)
void __imp__NetDll_XNetStartup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetStartup", ctx);
    FH2_HLE_ONCE(NetDll_XNetStartup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetStartup", ctx);
}

// NetDll_XNetUnregisterInAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetUnregisterInAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetUnregisterInAddr", ctx);
    FH2_HLE_ONCE(NetDll_XNetUnregisterInAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetUnregisterInAddr", ctx);
}

// NetDll_XNetUnregisterKey: semântica real pendente (issue #16)
void __imp__NetDll_XNetUnregisterKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetUnregisterKey", ctx);
    FH2_HLE_ONCE(NetDll_XNetUnregisterKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetUnregisterKey", ctx);
}

// NetDll_XNetXnAddrToInAddr: semântica real pendente (issue #16)
void __imp__NetDll_XNetXnAddrToInAddr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetXnAddrToInAddr", ctx);
    FH2_HLE_ONCE(NetDll_XNetXnAddrToInAddr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetXnAddrToInAddr", ctx);
}

// NetDll_XNetXnAddrToMachineId: semântica real pendente (issue #16)
void __imp__NetDll_XNetXnAddrToMachineId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XNetXnAddrToMachineId", ctx);
    FH2_HLE_ONCE(NetDll_XNetXnAddrToMachineId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XNetXnAddrToMachineId", ctx);
}

// NetDll_XnpGetConfigStatus: semântica real pendente (issue #16)
void __imp__NetDll_XnpGetConfigStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_XnpGetConfigStatus", ctx);
    FH2_HLE_ONCE(NetDll_XnpGetConfigStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_XnpGetConfigStatus", ctx);
}

// NetDll___WSAFDIsSet: semântica real pendente (issue #16)
void __imp__NetDll___WSAFDIsSet(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll___WSAFDIsSet", ctx);
    FH2_HLE_ONCE(NetDll___WSAFDIsSet, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll___WSAFDIsSet", ctx);
}

// NetDll_accept: semântica real pendente (issue #16)
void __imp__NetDll_accept(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_accept", ctx);
    FH2_HLE_ONCE(NetDll_accept, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_accept", ctx);
}

// NetDll_bind: semântica real pendente (issue #16)
void __imp__NetDll_bind(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_bind", ctx);
    FH2_HLE_ONCE(NetDll_bind, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_bind", ctx);
}

// NetDll_closesocket: semântica real pendente (issue #16)
void __imp__NetDll_closesocket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_closesocket", ctx);
    FH2_HLE_ONCE(NetDll_closesocket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_closesocket", ctx);
}

// NetDll_connect: semântica real pendente (issue #16)
void __imp__NetDll_connect(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_connect", ctx);
    FH2_HLE_ONCE(NetDll_connect, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_connect", ctx);
}

// NetDll_getpeername: semântica real pendente (issue #16)
void __imp__NetDll_getpeername(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_getpeername", ctx);
    FH2_HLE_ONCE(NetDll_getpeername, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_getpeername", ctx);
}

// NetDll_getsockname: semântica real pendente (issue #16)
void __imp__NetDll_getsockname(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_getsockname", ctx);
    FH2_HLE_ONCE(NetDll_getsockname, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_getsockname", ctx);
}

// NetDll_getsockopt: semântica real pendente (issue #16)
void __imp__NetDll_getsockopt(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_getsockopt", ctx);
    FH2_HLE_ONCE(NetDll_getsockopt, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_getsockopt", ctx);
}

// NetDll_inet_addr: semântica real pendente (issue #16)
void __imp__NetDll_inet_addr(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_inet_addr", ctx);
    FH2_HLE_ONCE(NetDll_inet_addr, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_inet_addr", ctx);
}

// NetDll_ioctlsocket: semântica real pendente (issue #16)
void __imp__NetDll_ioctlsocket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_ioctlsocket", ctx);
    FH2_HLE_ONCE(NetDll_ioctlsocket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_ioctlsocket", ctx);
}

// NetDll_listen: semântica real pendente (issue #16)
void __imp__NetDll_listen(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_listen", ctx);
    FH2_HLE_ONCE(NetDll_listen, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_listen", ctx);
}

// NetDll_recv: semântica real pendente (issue #16)
void __imp__NetDll_recv(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_recv", ctx);
    FH2_HLE_ONCE(NetDll_recv, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_recv", ctx);
}

// NetDll_recvfrom: semântica real pendente (issue #16)
void __imp__NetDll_recvfrom(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_recvfrom", ctx);
    FH2_HLE_ONCE(NetDll_recvfrom, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_recvfrom", ctx);
}

// NetDll_select: semântica real pendente (issue #16)
void __imp__NetDll_select(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_select", ctx);
    FH2_HLE_ONCE(NetDll_select, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_select", ctx);
}

// NetDll_send: semântica real pendente (issue #16)
void __imp__NetDll_send(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_send", ctx);
    FH2_HLE_ONCE(NetDll_send, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_send", ctx);
}

// NetDll_sendto: semântica real pendente (issue #16)
void __imp__NetDll_sendto(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_sendto", ctx);
    FH2_HLE_ONCE(NetDll_sendto, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_sendto", ctx);
}

// NetDll_setsockopt: semântica real pendente (issue #16)
void __imp__NetDll_setsockopt(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_setsockopt", ctx);
    FH2_HLE_ONCE(NetDll_setsockopt, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_setsockopt", ctx);
}

// NetDll_shutdown: semântica real pendente (issue #16)
void __imp__NetDll_shutdown(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_shutdown", ctx);
    FH2_HLE_ONCE(NetDll_shutdown, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_shutdown", ctx);
}

// NetDll_socket: semântica real pendente (issue #16)
void __imp__NetDll_socket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NetDll_socket", ctx);
    FH2_HLE_ONCE(NetDll_socket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NetDll_socket", ctx);
}

// NtAllocateVirtualMemory: semântica REAL (kernel_real.cpp)
void __imp__NtAllocateVirtualMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtAllocateVirtualMemory", ctx);
    fh2::kern::real_NtAllocateVirtualMemory(ctx, base);
    fh2::kern::traceReturn("NtAllocateVirtualMemory", ctx);
}

// NtCancelIoFile: falha REAL — IO pendente
void __imp__NtCancelIoFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtCancelIoFile", ctx);
    FH2_HLE_ONCE(NtCancelIoFile, "IO pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtCancelIoFile", ctx);
}

// NtCancelTimer: semântica real pendente (issue #16)
void __imp__NtCancelTimer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtCancelTimer", ctx);
    FH2_HLE_ONCE(NtCancelTimer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NtCancelTimer", ctx);
}

// NtClearEvent: semântica REAL (kernel_real.cpp)
void __imp__NtClearEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtClearEvent", ctx);
    fh2::kern::real_NtClearEvent(ctx, base);
    fh2::kern::traceReturn("NtClearEvent", ctx);
}

// NtClose: semântica REAL (kernel_real.cpp)
void __imp__NtClose(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtClose", ctx);
    fh2::kern::real_NtClose(ctx, base);
    fh2::kern::traceReturn("NtClose", ctx);
}

// NtCreateEvent: semântica REAL (kernel_real.cpp)
void __imp__NtCreateEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtCreateEvent", ctx);
    fh2::kern::real_NtCreateEvent(ctx, base);
    fh2::kern::traceReturn("NtCreateEvent", ctx);
}

// NtCreateFile: falha REAL — STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente
void __imp__NtCreateFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtCreateFile", ctx);
    FH2_HLE_ONCE(NtCreateFile, "STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente");
    ctx.r3.u32 = 0xC0000034u;
    fh2::kern::traceReturn("NtCreateFile", ctx);
}

// NtCreateMutant: semântica REAL (kernel_real.cpp)
void __imp__NtCreateMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtCreateMutant", ctx);
    fh2::kern::real_NtCreateMutant(ctx, base);
    fh2::kern::traceReturn("NtCreateMutant", ctx);
}

// NtCreateSemaphore: semântica REAL (kernel_real.cpp)
void __imp__NtCreateSemaphore(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtCreateSemaphore", ctx);
    fh2::kern::real_NtCreateSemaphore(ctx, base);
    fh2::kern::traceReturn("NtCreateSemaphore", ctx);
}

// NtCreateTimer: semântica real pendente (issue #16)
void __imp__NtCreateTimer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtCreateTimer", ctx);
    FH2_HLE_ONCE(NtCreateTimer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NtCreateTimer", ctx);
}

// NtDeviceIoControlFile: falha REAL — IOCTL pendente
void __imp__NtDeviceIoControlFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtDeviceIoControlFile", ctx);
    FH2_HLE_ONCE(NtDeviceIoControlFile, "IOCTL pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtDeviceIoControlFile", ctx);
}

// NtDuplicateObject: semântica real pendente (issue #16)
void __imp__NtDuplicateObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtDuplicateObject", ctx);
    FH2_HLE_ONCE(NtDuplicateObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NtDuplicateObject", ctx);
}

// NtFlushBuffersFile: falha REAL — FS pendente
void __imp__NtFlushBuffersFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtFlushBuffersFile", ctx);
    FH2_HLE_ONCE(NtFlushBuffersFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtFlushBuffersFile", ctx);
}

// NtFreeVirtualMemory: semântica REAL (kernel_real.cpp)
void __imp__NtFreeVirtualMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtFreeVirtualMemory", ctx);
    fh2::kern::real_NtFreeVirtualMemory(ctx, base);
    fh2::kern::traceReturn("NtFreeVirtualMemory", ctx);
}

// NtOpenFile: falha REAL — STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente
void __imp__NtOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtOpenFile", ctx);
    FH2_HLE_ONCE(NtOpenFile, "STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente");
    ctx.r3.u32 = 0xC0000034u;
    fh2::kern::traceReturn("NtOpenFile", ctx);
}

// NtQueryDirectoryFile: falha REAL — FS pendente
void __imp__NtQueryDirectoryFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtQueryDirectoryFile", ctx);
    FH2_HLE_ONCE(NtQueryDirectoryFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtQueryDirectoryFile", ctx);
}

// NtQueryFullAttributesFile: falha REAL — FS pendente
void __imp__NtQueryFullAttributesFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtQueryFullAttributesFile", ctx);
    FH2_HLE_ONCE(NtQueryFullAttributesFile, "FS pendente");
    ctx.r3.u32 = 0xC0000034u;
    fh2::kern::traceReturn("NtQueryFullAttributesFile", ctx);
}

// NtQueryInformationFile: falha REAL — FS pendente
void __imp__NtQueryInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtQueryInformationFile", ctx);
    FH2_HLE_ONCE(NtQueryInformationFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtQueryInformationFile", ctx);
}

// NtQueryVirtualMemory: semântica REAL (kernel_real.cpp)
void __imp__NtQueryVirtualMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtQueryVirtualMemory", ctx);
    fh2::kern::real_NtQueryVirtualMemory(ctx, base);
    fh2::kern::traceReturn("NtQueryVirtualMemory", ctx);
}

// NtQueryVolumeInformationFile: falha REAL — FS pendente
void __imp__NtQueryVolumeInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtQueryVolumeInformationFile", ctx);
    FH2_HLE_ONCE(NtQueryVolumeInformationFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtQueryVolumeInformationFile", ctx);
}

// NtReadFile: falha REAL — sem handle real (FS pendente)
void __imp__NtReadFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtReadFile", ctx);
    FH2_HLE_ONCE(NtReadFile, "sem handle real (FS pendente)");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtReadFile", ctx);
}

// NtReadFileScatter: falha REAL — FS pendente
void __imp__NtReadFileScatter(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtReadFileScatter", ctx);
    FH2_HLE_ONCE(NtReadFileScatter, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtReadFileScatter", ctx);
}

// NtReleaseMutant: semântica REAL (kernel_real.cpp)
void __imp__NtReleaseMutant(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtReleaseMutant", ctx);
    fh2::kern::real_NtReleaseMutant(ctx, base);
    fh2::kern::traceReturn("NtReleaseMutant", ctx);
}

// NtReleaseSemaphore: semântica REAL (kernel_real.cpp)
void __imp__NtReleaseSemaphore(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtReleaseSemaphore", ctx);
    fh2::kern::real_NtReleaseSemaphore(ctx, base);
    fh2::kern::traceReturn("NtReleaseSemaphore", ctx);
}

// NtResumeThread: semântica real pendente (issue #16)
void __imp__NtResumeThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtResumeThread", ctx);
    FH2_HLE_ONCE(NtResumeThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NtResumeThread", ctx);
}

// NtSetEvent: semântica REAL (kernel_real.cpp)
void __imp__NtSetEvent(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtSetEvent", ctx);
    fh2::kern::real_NtSetEvent(ctx, base);
    fh2::kern::traceReturn("NtSetEvent", ctx);
}

// NtSetInformationFile: falha REAL — FS pendente
void __imp__NtSetInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtSetInformationFile", ctx);
    FH2_HLE_ONCE(NtSetInformationFile, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtSetInformationFile", ctx);
}

// NtSetTimerEx: semântica real pendente (issue #16)
void __imp__NtSetTimerEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtSetTimerEx", ctx);
    FH2_HLE_ONCE(NtSetTimerEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("NtSetTimerEx", ctx);
}

// NtSignalAndWaitForSingleObjectEx: semântica REAL (kernel_real.cpp)
void __imp__NtSignalAndWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtSignalAndWaitForSingleObjectEx", ctx);
    fh2::kern::real_NtSignalAndWaitForSingleObjectEx(ctx, base);
    fh2::kern::traceReturn("NtSignalAndWaitForSingleObjectEx", ctx);
}

// NtWaitForMultipleObjectsEx: semântica REAL (kernel_real.cpp)
void __imp__NtWaitForMultipleObjectsEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtWaitForMultipleObjectsEx", ctx);
    fh2::kern::real_NtWaitForMultipleObjectsEx(ctx, base);
    fh2::kern::traceReturn("NtWaitForMultipleObjectsEx", ctx);
}

// NtWaitForSingleObjectEx: semântica REAL (kernel_real.cpp)
void __imp__NtWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtWaitForSingleObjectEx", ctx);
    fh2::kern::real_NtWaitForSingleObjectEx(ctx, base);
    fh2::kern::traceReturn("NtWaitForSingleObjectEx", ctx);
}

// NtWriteFile: falha REAL — sem handle real (FS pendente)
void __imp__NtWriteFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtWriteFile", ctx);
    FH2_HLE_ONCE(NtWriteFile, "sem handle real (FS pendente)");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtWriteFile", ctx);
}

// NtWriteFileGather: falha REAL — FS pendente
void __imp__NtWriteFileGather(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("NtWriteFileGather", ctx);
    FH2_HLE_ONCE(NtWriteFileGather, "FS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("NtWriteFileGather", ctx);
}

// NtYieldExecution: semântica REAL (kernel_real.cpp)
void __imp__NtYieldExecution(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("NtYieldExecution", ctx);
    fh2::kern::real_NtYieldExecution(ctx, base);
    fh2::kern::traceReturn("NtYieldExecution", ctx);
}

// ObCreateObject: semântica real pendente (issue #16)
void __imp__ObCreateObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObCreateObject", ctx);
    FH2_HLE_ONCE(ObCreateObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObCreateObject", ctx);
}

// ObCreateSymbolicLink: semântica real pendente (issue #16)
void __imp__ObCreateSymbolicLink(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObCreateSymbolicLink", ctx);
    FH2_HLE_ONCE(ObCreateSymbolicLink, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObCreateSymbolicLink", ctx);
}

// ObDeleteSymbolicLink: semântica real pendente (issue #16)
void __imp__ObDeleteSymbolicLink(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObDeleteSymbolicLink", ctx);
    FH2_HLE_ONCE(ObDeleteSymbolicLink, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObDeleteSymbolicLink", ctx);
}

// ObDereferenceObject: semântica real pendente (issue #16)
void __imp__ObDereferenceObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObDereferenceObject", ctx);
    FH2_HLE_ONCE(ObDereferenceObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObDereferenceObject", ctx);
}

// ObIsTitleObject: semântica real pendente (issue #16)
void __imp__ObIsTitleObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObIsTitleObject", ctx);
    FH2_HLE_ONCE(ObIsTitleObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObIsTitleObject", ctx);
}

// ObOpenObjectByPointer: semântica real pendente (issue #16)
void __imp__ObOpenObjectByPointer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObOpenObjectByPointer", ctx);
    FH2_HLE_ONCE(ObOpenObjectByPointer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObOpenObjectByPointer", ctx);
}

// ObReferenceObject: semântica real pendente (issue #16)
void __imp__ObReferenceObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObReferenceObject", ctx);
    FH2_HLE_ONCE(ObReferenceObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObReferenceObject", ctx);
}

// ObReferenceObjectByHandle: semântica real pendente (issue #16)
void __imp__ObReferenceObjectByHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("ObReferenceObjectByHandle", ctx);
    FH2_HLE_ONCE(ObReferenceObjectByHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("ObReferenceObjectByHandle", ctx);
}

// PsCamDeviceRequest: semântica real pendente (issue #16)
void __imp__PsCamDeviceRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("PsCamDeviceRequest", ctx);
    FH2_HLE_ONCE(PsCamDeviceRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("PsCamDeviceRequest", ctx);
}

// RtlCaptureContext: semântica real pendente (issue #16)
void __imp__RtlCaptureContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("RtlCaptureContext", ctx);
    FH2_HLE_ONCE(RtlCaptureContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("RtlCaptureContext", ctx);
}

// RtlCompareMemory: semântica REAL (kernel_real.cpp)
void __imp__RtlCompareMemory(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlCompareMemory", ctx);
    fh2::kern::real_RtlCompareMemory(ctx, base);
    fh2::kern::traceReturn("RtlCompareMemory", ctx);
}

// RtlCompareMemoryUlong: semântica REAL (kernel_real.cpp)
void __imp__RtlCompareMemoryUlong(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlCompareMemoryUlong", ctx);
    fh2::kern::real_RtlCompareMemoryUlong(ctx, base);
    fh2::kern::traceReturn("RtlCompareMemoryUlong", ctx);
}

// RtlCompareStringN: semântica real pendente (issue #16)
void __imp__RtlCompareStringN(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("RtlCompareStringN", ctx);
    FH2_HLE_ONCE(RtlCompareStringN, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("RtlCompareStringN", ctx);
}

// RtlEnterCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlEnterCriticalSection", ctx);
    fh2::kern::real_RtlEnterCriticalSection(ctx, base);
    fh2::kern::traceReturn("RtlEnterCriticalSection", ctx);
}

// RtlFillMemoryUlong: semântica REAL (kernel_real.cpp)
void __imp__RtlFillMemoryUlong(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlFillMemoryUlong", ctx);
    fh2::kern::real_RtlFillMemoryUlong(ctx, base);
    fh2::kern::traceReturn("RtlFillMemoryUlong", ctx);
}

// RtlFreeAnsiString: semântica REAL (kernel_real.cpp)
void __imp__RtlFreeAnsiString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlFreeAnsiString", ctx);
    fh2::kern::real_RtlFreeAnsiString(ctx, base);
    fh2::kern::traceReturn("RtlFreeAnsiString", ctx);
}

// RtlImageXexHeaderField: semântica real pendente (issue #16)
void __imp__RtlImageXexHeaderField(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("RtlImageXexHeaderField", ctx);
    FH2_HLE_ONCE(RtlImageXexHeaderField, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("RtlImageXexHeaderField", ctx);
}

// RtlInitAnsiString: semântica REAL (kernel_real.cpp)
void __imp__RtlInitAnsiString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlInitAnsiString", ctx);
    fh2::kern::real_RtlInitAnsiString(ctx, base);
    fh2::kern::traceReturn("RtlInitAnsiString", ctx);
}

// RtlInitUnicodeString: semântica REAL (kernel_real.cpp)
void __imp__RtlInitUnicodeString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlInitUnicodeString", ctx);
    fh2::kern::real_RtlInitUnicodeString(ctx, base);
    fh2::kern::traceReturn("RtlInitUnicodeString", ctx);
}

// RtlInitializeCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlInitializeCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlInitializeCriticalSection", ctx);
    fh2::kern::real_RtlInitializeCriticalSection(ctx, base);
    fh2::kern::traceReturn("RtlInitializeCriticalSection", ctx);
}

// RtlInitializeCriticalSectionAndSpinCount: semântica REAL (kernel_real.cpp)
void __imp__RtlInitializeCriticalSectionAndSpinCount(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlInitializeCriticalSectionAndSpinCount", ctx);
    fh2::kern::real_RtlInitializeCriticalSectionAndSpinCount(ctx, base);
    fh2::kern::traceReturn("RtlInitializeCriticalSectionAndSpinCount", ctx);
}

// RtlLeaveCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlLeaveCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlLeaveCriticalSection", ctx);
    fh2::kern::real_RtlLeaveCriticalSection(ctx, base);
    fh2::kern::traceReturn("RtlLeaveCriticalSection", ctx);
}

// RtlMultiByteToUnicodeN: semântica REAL (kernel_real.cpp)
void __imp__RtlMultiByteToUnicodeN(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlMultiByteToUnicodeN", ctx);
    fh2::kern::real_RtlMultiByteToUnicodeN(ctx, base);
    fh2::kern::traceReturn("RtlMultiByteToUnicodeN", ctx);
}

// RtlNtStatusToDosError: semântica REAL (kernel_real.cpp)
void __imp__RtlNtStatusToDosError(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlNtStatusToDosError", ctx);
    fh2::kern::real_RtlNtStatusToDosError(ctx, base);
    fh2::kern::traceReturn("RtlNtStatusToDosError", ctx);
}

// RtlRaiseException: SEH do guest não suportado (issue #15)
void __imp__RtlRaiseException(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("RtlRaiseException", ctx);
    FH2_HLE_ONCE(RtlRaiseException, "SEH do guest não suportado (issue #15)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("RtlRaiseException", ctx);
}

// RtlTimeFieldsToTime: semântica REAL (kernel_real.cpp)
void __imp__RtlTimeFieldsToTime(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlTimeFieldsToTime", ctx);
    fh2::kern::real_RtlTimeFieldsToTime(ctx, base);
    fh2::kern::traceReturn("RtlTimeFieldsToTime", ctx);
}

// RtlTimeToTimeFields: semântica REAL (kernel_real.cpp)
void __imp__RtlTimeToTimeFields(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlTimeToTimeFields", ctx);
    fh2::kern::real_RtlTimeToTimeFields(ctx, base);
    fh2::kern::traceReturn("RtlTimeToTimeFields", ctx);
}

// RtlTryEnterCriticalSection: semântica REAL (kernel_real.cpp)
void __imp__RtlTryEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlTryEnterCriticalSection", ctx);
    fh2::kern::real_RtlTryEnterCriticalSection(ctx, base);
    fh2::kern::traceReturn("RtlTryEnterCriticalSection", ctx);
}

// RtlUnicodeStringToAnsiString: semântica REAL (kernel_real.cpp)
void __imp__RtlUnicodeStringToAnsiString(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlUnicodeStringToAnsiString", ctx);
    fh2::kern::real_RtlUnicodeStringToAnsiString(ctx, base);
    fh2::kern::traceReturn("RtlUnicodeStringToAnsiString", ctx);
}

// RtlUnicodeToMultiByteN: semântica REAL (kernel_real.cpp)
void __imp__RtlUnicodeToMultiByteN(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlUnicodeToMultiByteN", ctx);
    fh2::kern::real_RtlUnicodeToMultiByteN(ctx, base);
    fh2::kern::traceReturn("RtlUnicodeToMultiByteN", ctx);
}

// RtlUnwind: semântica real pendente (issue #16)
void __imp__RtlUnwind(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("RtlUnwind", ctx);
    FH2_HLE_ONCE(RtlUnwind, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("RtlUnwind", ctx);
}

// RtlUpcaseUnicodeChar: semântica REAL (kernel_real.cpp)
void __imp__RtlUpcaseUnicodeChar(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("RtlUpcaseUnicodeChar", ctx);
    fh2::kern::real_RtlUpcaseUnicodeChar(ctx, base);
    fh2::kern::traceReturn("RtlUpcaseUnicodeChar", ctx);
}

// StfsControlDevice: falha REAL — STFS pendente
void __imp__StfsControlDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("StfsControlDevice", ctx);
    FH2_HLE_ONCE(StfsControlDevice, "STFS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("StfsControlDevice", ctx);
}

// StfsCreateDevice: falha REAL — STFS pendente
void __imp__StfsCreateDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("StfsCreateDevice", ctx);
    FH2_HLE_ONCE(StfsCreateDevice, "STFS pendente");
    ctx.r3.u32 = 0xC0000001u;
    fh2::kern::traceReturn("StfsCreateDevice", ctx);
}

// VdCallGraphicsNotificationRoutines: semântica real pendente (issue #16)
void __imp__VdCallGraphicsNotificationRoutines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdCallGraphicsNotificationRoutines", ctx);
    FH2_HLE_ONCE(VdCallGraphicsNotificationRoutines, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdCallGraphicsNotificationRoutines", ctx);
}

// VdEnableDisableClockGating: semântica real pendente (issue #16)
void __imp__VdEnableDisableClockGating(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdEnableDisableClockGating", ctx);
    FH2_HLE_ONCE(VdEnableDisableClockGating, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdEnableDisableClockGating", ctx);
}

// VdEnableRingBufferRPtrWriteBack: semântica real pendente (issue #16)
void __imp__VdEnableRingBufferRPtrWriteBack(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdEnableRingBufferRPtrWriteBack", ctx);
    FH2_HLE_ONCE(VdEnableRingBufferRPtrWriteBack, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdEnableRingBufferRPtrWriteBack", ctx);
}

// VdGetCurrentDisplayGamma: semântica real pendente (issue #16)
void __imp__VdGetCurrentDisplayGamma(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdGetCurrentDisplayGamma", ctx);
    FH2_HLE_ONCE(VdGetCurrentDisplayGamma, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdGetCurrentDisplayGamma", ctx);
}

// VdGetCurrentDisplayInformation: semântica real pendente (issue #16)
void __imp__VdGetCurrentDisplayInformation(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdGetCurrentDisplayInformation", ctx);
    FH2_HLE_ONCE(VdGetCurrentDisplayInformation, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdGetCurrentDisplayInformation", ctx);
}

// VdGetSystemCommandBuffer: semântica real pendente (issue #16)
void __imp__VdGetSystemCommandBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdGetSystemCommandBuffer", ctx);
    FH2_HLE_ONCE(VdGetSystemCommandBuffer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdGetSystemCommandBuffer", ctx);
}

// VdInitializeEngines: semântica real pendente (issue #16)
void __imp__VdInitializeEngines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdInitializeEngines", ctx);
    FH2_HLE_ONCE(VdInitializeEngines, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdInitializeEngines", ctx);
}

// VdInitializeRingBuffer: semântica real pendente (issue #16)
void __imp__VdInitializeRingBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdInitializeRingBuffer", ctx);
    FH2_HLE_ONCE(VdInitializeRingBuffer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdInitializeRingBuffer", ctx);
}

// VdInitializeScalerCommandBuffer: semântica real pendente (issue #16)
void __imp__VdInitializeScalerCommandBuffer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdInitializeScalerCommandBuffer", ctx);
    FH2_HLE_ONCE(VdInitializeScalerCommandBuffer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdInitializeScalerCommandBuffer", ctx);
}

// VdIsHSIOTrainingSucceeded: semântica real pendente (issue #16)
void __imp__VdIsHSIOTrainingSucceeded(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdIsHSIOTrainingSucceeded", ctx);
    FH2_HLE_ONCE(VdIsHSIOTrainingSucceeded, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdIsHSIOTrainingSucceeded", ctx);
}

// VdPersistDisplay: semântica real pendente (issue #16)
void __imp__VdPersistDisplay(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdPersistDisplay", ctx);
    FH2_HLE_ONCE(VdPersistDisplay, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdPersistDisplay", ctx);
}

// VdQueryVideoFlags: semântica real pendente (issue #16)
void __imp__VdQueryVideoFlags(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdQueryVideoFlags", ctx);
    FH2_HLE_ONCE(VdQueryVideoFlags, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdQueryVideoFlags", ctx);
}

// VdQueryVideoMode: semântica real pendente (issue #16)
void __imp__VdQueryVideoMode(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdQueryVideoMode", ctx);
    FH2_HLE_ONCE(VdQueryVideoMode, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdQueryVideoMode", ctx);
}

// VdRetrainEDRAM: semântica real pendente (issue #16)
void __imp__VdRetrainEDRAM(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdRetrainEDRAM", ctx);
    FH2_HLE_ONCE(VdRetrainEDRAM, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdRetrainEDRAM", ctx);
}

// VdRetrainEDRAMWorker: semântica real pendente (issue #16)
void __imp__VdRetrainEDRAMWorker(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdRetrainEDRAMWorker", ctx);
    FH2_HLE_ONCE(VdRetrainEDRAMWorker, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdRetrainEDRAMWorker", ctx);
}

// VdSetDisplayMode: semântica real pendente (issue #16)
void __imp__VdSetDisplayMode(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdSetDisplayMode", ctx);
    FH2_HLE_ONCE(VdSetDisplayMode, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdSetDisplayMode", ctx);
}

// VdSetDisplayModeOverride: semântica real pendente (issue #16)
void __imp__VdSetDisplayModeOverride(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdSetDisplayModeOverride", ctx);
    FH2_HLE_ONCE(VdSetDisplayModeOverride, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdSetDisplayModeOverride", ctx);
}

// VdSetGraphicsInterruptCallback: semântica real pendente (issue #16)
void __imp__VdSetGraphicsInterruptCallback(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdSetGraphicsInterruptCallback", ctx);
    FH2_HLE_ONCE(VdSetGraphicsInterruptCallback, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdSetGraphicsInterruptCallback", ctx);
}

// VdSetSystemCommandBufferGpuIdentifierAddress: semântica real pendente (issue #16)
void __imp__VdSetSystemCommandBufferGpuIdentifierAddress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdSetSystemCommandBufferGpuIdentifierAddress", ctx);
    FH2_HLE_ONCE(VdSetSystemCommandBufferGpuIdentifierAddress, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdSetSystemCommandBufferGpuIdentifierAddress", ctx);
}

// VdShutdownEngines: semântica real pendente (issue #16)
void __imp__VdShutdownEngines(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdShutdownEngines", ctx);
    FH2_HLE_ONCE(VdShutdownEngines, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdShutdownEngines", ctx);
}

// VdSwap: present do GPU — issue #17 (gráficos reais)
void __imp__VdSwap(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("VdSwap", ctx);
    FH2_HLE_ONCE(VdSwap, "present do GPU — issue #17 (gráficos reais)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("VdSwap", ctx);
}

// XAudioEnableDucker: semântica real pendente (issue #16)
void __imp__XAudioEnableDucker(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioEnableDucker", ctx);
    FH2_HLE_ONCE(XAudioEnableDucker, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioEnableDucker", ctx);
}

// XAudioGetDuckerAttackTime: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerAttackTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioGetDuckerAttackTime", ctx);
    FH2_HLE_ONCE(XAudioGetDuckerAttackTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioGetDuckerAttackTime", ctx);
}

// XAudioGetDuckerHoldTime: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerHoldTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioGetDuckerHoldTime", ctx);
    FH2_HLE_ONCE(XAudioGetDuckerHoldTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioGetDuckerHoldTime", ctx);
}

// XAudioGetDuckerLevel: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerLevel(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioGetDuckerLevel", ctx);
    FH2_HLE_ONCE(XAudioGetDuckerLevel, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioGetDuckerLevel", ctx);
}

// XAudioGetDuckerReleaseTime: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerReleaseTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioGetDuckerReleaseTime", ctx);
    FH2_HLE_ONCE(XAudioGetDuckerReleaseTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioGetDuckerReleaseTime", ctx);
}

// XAudioGetDuckerThreshold: semântica real pendente (issue #16)
void __imp__XAudioGetDuckerThreshold(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioGetDuckerThreshold", ctx);
    FH2_HLE_ONCE(XAudioGetDuckerThreshold, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioGetDuckerThreshold", ctx);
}

// XAudioGetSpeakerConfig: semântica real pendente (issue #16)
void __imp__XAudioGetSpeakerConfig(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioGetSpeakerConfig", ctx);
    FH2_HLE_ONCE(XAudioGetSpeakerConfig, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioGetSpeakerConfig", ctx);
}

// XAudioGetVoiceCategoryVolume: semântica real pendente (issue #16)
void __imp__XAudioGetVoiceCategoryVolume(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioGetVoiceCategoryVolume", ctx);
    FH2_HLE_ONCE(XAudioGetVoiceCategoryVolume, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioGetVoiceCategoryVolume", ctx);
}

// XAudioRegisterRenderDriverClient: semântica real pendente (issue #16)
void __imp__XAudioRegisterRenderDriverClient(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioRegisterRenderDriverClient", ctx);
    FH2_HLE_ONCE(XAudioRegisterRenderDriverClient, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioRegisterRenderDriverClient", ctx);
}

// XAudioSubmitRenderDriverFrame: áudio do guest — issue #18
void __imp__XAudioSubmitRenderDriverFrame(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioSubmitRenderDriverFrame", ctx);
    FH2_HLE_ONCE(XAudioSubmitRenderDriverFrame, "áudio do guest — issue #18");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioSubmitRenderDriverFrame", ctx);
}

// XAudioUnregisterRenderDriverClient: semântica real pendente (issue #16)
void __imp__XAudioUnregisterRenderDriverClient(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XAudioUnregisterRenderDriverClient", ctx);
    FH2_HLE_ONCE(XAudioUnregisterRenderDriverClient, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XAudioUnregisterRenderDriverClient", ctx);
}

// XCustomGetCurrentGamercard: semântica real pendente (issue #16)
void __imp__XCustomGetCurrentGamercard(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XCustomGetCurrentGamercard", ctx);
    FH2_HLE_ONCE(XCustomGetCurrentGamercard, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XCustomGetCurrentGamercard", ctx);
}

// XCustomGetLastActionPressEx: semântica real pendente (issue #16)
void __imp__XCustomGetLastActionPressEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XCustomGetLastActionPressEx", ctx);
    FH2_HLE_ONCE(XCustomGetLastActionPressEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XCustomGetLastActionPressEx", ctx);
}

// XCustomRegisterDynamicActions: semântica real pendente (issue #16)
void __imp__XCustomRegisterDynamicActions(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XCustomRegisterDynamicActions", ctx);
    FH2_HLE_ONCE(XCustomRegisterDynamicActions, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XCustomRegisterDynamicActions", ctx);
}

// XCustomSetDynamicActions: semântica real pendente (issue #16)
void __imp__XCustomSetDynamicActions(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XCustomSetDynamicActions", ctx);
    FH2_HLE_ONCE(XCustomSetDynamicActions, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XCustomSetDynamicActions", ctx);
}

// XCustomUnregisterDynamicActions: semântica real pendente (issue #16)
void __imp__XCustomUnregisterDynamicActions(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XCustomUnregisterDynamicActions", ctx);
    FH2_HLE_ONCE(XCustomUnregisterDynamicActions, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XCustomUnregisterDynamicActions", ctx);
}

// XGetAVPack: semântica real pendente (issue #16)
void __imp__XGetAVPack(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XGetAVPack", ctx);
    FH2_HLE_ONCE(XGetAVPack, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XGetAVPack", ctx);
}

// XGetGameRegion: semântica real pendente (issue #16)
void __imp__XGetGameRegion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XGetGameRegion", ctx);
    FH2_HLE_ONCE(XGetGameRegion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XGetGameRegion", ctx);
}

// XGetVideoMode: semântica real pendente (issue #16)
void __imp__XGetVideoMode(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XGetVideoMode", ctx);
    FH2_HLE_ONCE(XGetVideoMode, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XGetVideoMode", ctx);
}

// XMACreateContext: semântica real pendente (issue #16)
void __imp__XMACreateContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMACreateContext", ctx);
    FH2_HLE_ONCE(XMACreateContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMACreateContext", ctx);
}

// XMAReleaseContext: semântica real pendente (issue #16)
void __imp__XMAReleaseContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMAReleaseContext", ctx);
    FH2_HLE_ONCE(XMAReleaseContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMAReleaseContext", ctx);
}

// XMsgCancelIORequest: semântica real pendente (issue #16)
void __imp__XMsgCancelIORequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMsgCancelIORequest", ctx);
    FH2_HLE_ONCE(XMsgCancelIORequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMsgCancelIORequest", ctx);
}

// XMsgCompleteIORequest: semântica real pendente (issue #16)
void __imp__XMsgCompleteIORequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMsgCompleteIORequest", ctx);
    FH2_HLE_ONCE(XMsgCompleteIORequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMsgCompleteIORequest", ctx);
}

// XMsgInProcessCall: semântica real pendente (issue #16)
void __imp__XMsgInProcessCall(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMsgInProcessCall", ctx);
    FH2_HLE_ONCE(XMsgInProcessCall, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMsgInProcessCall", ctx);
}

// XMsgStartIORequest: semântica real pendente (issue #16)
void __imp__XMsgStartIORequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMsgStartIORequest", ctx);
    FH2_HLE_ONCE(XMsgStartIORequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMsgStartIORequest", ctx);
}

// XMsgStartIORequestEx: semântica real pendente (issue #16)
void __imp__XMsgStartIORequestEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMsgStartIORequestEx", ctx);
    FH2_HLE_ONCE(XMsgStartIORequestEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMsgStartIORequestEx", ctx);
}

// XMsgSystemProcessCall: semântica real pendente (issue #16)
void __imp__XMsgSystemProcessCall(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XMsgSystemProcessCall", ctx);
    FH2_HLE_ONCE(XMsgSystemProcessCall, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XMsgSystemProcessCall", ctx);
}

// XNetLogonGetMachineID: semântica real pendente (issue #16)
void __imp__XNetLogonGetMachineID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XNetLogonGetMachineID", ctx);
    FH2_HLE_ONCE(XNetLogonGetMachineID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XNetLogonGetMachineID", ctx);
}

// XNetLogonGetTitleID: semântica real pendente (issue #16)
void __imp__XNetLogonGetTitleID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XNetLogonGetTitleID", ctx);
    FH2_HLE_ONCE(XNetLogonGetTitleID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XNetLogonGetTitleID", ctx);
}

// XNotifyGetNext: semântica real pendente (issue #16)
void __imp__XNotifyGetNext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XNotifyGetNext", ctx);
    FH2_HLE_ONCE(XNotifyGetNext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XNotifyGetNext", ctx);
}

// XamAlloc: semântica real pendente (issue #16)
void __imp__XamAlloc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamAlloc", ctx);
    FH2_HLE_ONCE(XamAlloc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamAlloc", ctx);
}

// XamCacheCloseFile: semântica real pendente (issue #16)
void __imp__XamCacheCloseFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamCacheCloseFile", ctx);
    FH2_HLE_ONCE(XamCacheCloseFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamCacheCloseFile", ctx);
}

// XamCacheOpenFile: falha REAL — cache pendente
void __imp__XamCacheOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamCacheOpenFile", ctx);
    FH2_HLE_ONCE(XamCacheOpenFile, "cache pendente");
    ctx.r3.u32 = 0x803500F1u;
    fh2::kern::traceReturn("XamCacheOpenFile", ctx);
}

// XamCacheReset: semântica real pendente (issue #16)
void __imp__XamCacheReset(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamCacheReset", ctx);
    FH2_HLE_ONCE(XamCacheReset, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamCacheReset", ctx);
}

// XamContentClose: semântica real pendente (issue #16)
void __imp__XamContentClose(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentClose", ctx);
    FH2_HLE_ONCE(XamContentClose, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamContentClose", ctx);
}

// XamContentCreateEnumerator: semântica real pendente (issue #16)
void __imp__XamContentCreateEnumerator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentCreateEnumerator", ctx);
    FH2_HLE_ONCE(XamContentCreateEnumerator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamContentCreateEnumerator", ctx);
}

// XamContentCreateEx: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentCreateEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentCreateEx", ctx);
    FH2_HLE_ONCE(XamContentCreateEx, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
    fh2::kern::traceReturn("XamContentCreateEx", ctx);
}

// XamContentDelete: semântica real pendente (issue #16)
void __imp__XamContentDelete(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentDelete", ctx);
    FH2_HLE_ONCE(XamContentDelete, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamContentDelete", ctx);
}

// XamContentFlush: semântica real pendente (issue #16)
void __imp__XamContentFlush(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentFlush", ctx);
    FH2_HLE_ONCE(XamContentFlush, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamContentFlush", ctx);
}

// XamContentGetCreator: semântica real pendente (issue #16)
void __imp__XamContentGetCreator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentGetCreator", ctx);
    FH2_HLE_ONCE(XamContentGetCreator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamContentGetCreator", ctx);
}

// XamContentGetDeviceData: semântica real pendente (issue #16)
void __imp__XamContentGetDeviceData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentGetDeviceData", ctx);
    FH2_HLE_ONCE(XamContentGetDeviceData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamContentGetDeviceData", ctx);
}

// XamContentGetDeviceState: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentGetDeviceState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentGetDeviceState", ctx);
    FH2_HLE_ONCE(XamContentGetDeviceState, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
    fh2::kern::traceReturn("XamContentGetDeviceState", ctx);
}

// XamContentGetLicenseMask: semântica real pendente (issue #16)
void __imp__XamContentGetLicenseMask(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentGetLicenseMask", ctx);
    FH2_HLE_ONCE(XamContentGetLicenseMask, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamContentGetLicenseMask", ctx);
}

// XamContentOpenFile: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentOpenFile", ctx);
    FH2_HLE_ONCE(XamContentOpenFile, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
    fh2::kern::traceReturn("XamContentOpenFile", ctx);
}

// XamContentResolve: falha REAL — sem pacote de conteúdo montado
void __imp__XamContentResolve(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamContentResolve", ctx);
    FH2_HLE_ONCE(XamContentResolve, "sem pacote de conteúdo montado");
    ctx.r3.u32 = 0x803500F1u;
    fh2::kern::traceReturn("XamContentResolve", ctx);
}

// XamCreateEnumeratorHandle: semântica real pendente (issue #16)
void __imp__XamCreateEnumeratorHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamCreateEnumeratorHandle", ctx);
    FH2_HLE_ONCE(XamCreateEnumeratorHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamCreateEnumeratorHandle", ctx);
}

// XamEnableInactivityProcessing: semântica real pendente (issue #16)
void __imp__XamEnableInactivityProcessing(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamEnableInactivityProcessing", ctx);
    FH2_HLE_ONCE(XamEnableInactivityProcessing, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamEnableInactivityProcessing", ctx);
}

// XamEnumerate: semântica real pendente (issue #16)
void __imp__XamEnumerate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamEnumerate", ctx);
    FH2_HLE_ONCE(XamEnumerate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamEnumerate", ctx);
}

// XamFree: semântica real pendente (issue #16)
void __imp__XamFree(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamFree", ctx);
    FH2_HLE_ONCE(XamFree, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamFree", ctx);
}

// XamGetActiveDashAppInfo: semântica real pendente (issue #16)
void __imp__XamGetActiveDashAppInfo(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamGetActiveDashAppInfo", ctx);
    FH2_HLE_ONCE(XamGetActiveDashAppInfo, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamGetActiveDashAppInfo", ctx);
}

// XamGetCurrentTitleId: semântica REAL (kernel_real.cpp)
void __imp__XamGetCurrentTitleId(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamGetCurrentTitleId", ctx);
    fh2::kern::real_XamGetCurrentTitleId(ctx, base);
    fh2::kern::traceReturn("XamGetCurrentTitleId", ctx);
}

// XamGetExecutionId: semântica real pendente (issue #16)
void __imp__XamGetExecutionId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamGetExecutionId", ctx);
    FH2_HLE_ONCE(XamGetExecutionId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamGetExecutionId", ctx);
}

// XamGetLanguage: semântica real pendente (issue #16)
void __imp__XamGetLanguage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamGetLanguage", ctx);
    FH2_HLE_ONCE(XamGetLanguage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamGetLanguage", ctx);
}

// XamGetLocaleEx: semântica real pendente (issue #16)
void __imp__XamGetLocaleEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamGetLocaleEx", ctx);
    FH2_HLE_ONCE(XamGetLocaleEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamGetLocaleEx", ctx);
}

// XamGetOverlappedResult: semântica real pendente (issue #16)
void __imp__XamGetOverlappedResult(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamGetOverlappedResult", ctx);
    FH2_HLE_ONCE(XamGetOverlappedResult, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamGetOverlappedResult", ctx);
}

// XamGetPrivateEnumStructureFromHandle: semântica real pendente (issue #16)
void __imp__XamGetPrivateEnumStructureFromHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamGetPrivateEnumStructureFromHandle", ctx);
    FH2_HLE_ONCE(XamGetPrivateEnumStructureFromHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamGetPrivateEnumStructureFromHandle", ctx);
}

// XamGetSystemVersion: semântica real pendente (issue #16)
void __imp__XamGetSystemVersion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamGetSystemVersion", ctx);
    FH2_HLE_ONCE(XamGetSystemVersion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamGetSystemVersion", ctx);
}

// XamInputGetCapabilities: semântica REAL (kernel_real.cpp)
void __imp__XamInputGetCapabilities(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamInputGetCapabilities", ctx);
    fh2::kern::real_XamInputGetCapabilities(ctx, base);
    fh2::kern::traceReturn("XamInputGetCapabilities", ctx);
}

// XamInputGetCapabilitiesEx: semântica REAL (kernel_real.cpp)
void __imp__XamInputGetCapabilitiesEx(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamInputGetCapabilitiesEx", ctx);
    fh2::kern::real_XamInputGetCapabilitiesEx(ctx, base);
    fh2::kern::traceReturn("XamInputGetCapabilitiesEx", ctx);
}

// XamInputGetState: semântica REAL (kernel_real.cpp)
void __imp__XamInputGetState(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamInputGetState", ctx);
    fh2::kern::real_XamInputGetState(ctx, base);
    fh2::kern::traceReturn("XamInputGetState", ctx);
}

// XamInputRawState: semântica real pendente (issue #16)
void __imp__XamInputRawState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamInputRawState", ctx);
    FH2_HLE_ONCE(XamInputRawState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamInputRawState", ctx);
}

// XamInputSetState: semântica REAL (kernel_real.cpp)
void __imp__XamInputSetState(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamInputSetState", ctx);
    fh2::kern::real_XamInputSetState(ctx, base);
    fh2::kern::traceReturn("XamInputSetState", ctx);
}

// XamIsUIActive: semântica real pendente (issue #16)
void __imp__XamIsUIActive(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamIsUIActive", ctx);
    FH2_HLE_ONCE(XamIsUIActive, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamIsUIActive", ctx);
}

// XamLoaderGetLaunchData: semântica REAL (kernel_real.cpp)
void __imp__XamLoaderGetLaunchData(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamLoaderGetLaunchData", ctx);
    fh2::kern::real_XamLoaderGetLaunchData(ctx, base);
    fh2::kern::traceReturn("XamLoaderGetLaunchData", ctx);
}

// XamLoaderGetLaunchDataSize: semântica REAL (kernel_real.cpp)
void __imp__XamLoaderGetLaunchDataSize(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamLoaderGetLaunchDataSize", ctx);
    fh2::kern::real_XamLoaderGetLaunchDataSize(ctx, base);
    fh2::kern::traceReturn("XamLoaderGetLaunchDataSize", ctx);
}

// XamLoaderLaunchTitle: semântica real pendente (issue #16)
void __imp__XamLoaderLaunchTitle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLoaderLaunchTitle", ctx);
    FH2_HLE_ONCE(XamLoaderLaunchTitle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLoaderLaunchTitle", ctx);
}

// XamLoaderSetLaunchData: semântica real pendente (issue #16)
void __imp__XamLoaderSetLaunchData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLoaderSetLaunchData", ctx);
    FH2_HLE_ONCE(XamLoaderSetLaunchData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLoaderSetLaunchData", ctx);
}

// XamLoaderTerminateTitle: semântica real pendente (issue #16)
void __imp__XamLoaderTerminateTitle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLoaderTerminateTitle", ctx);
    FH2_HLE_ONCE(XamLoaderTerminateTitle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLoaderTerminateTitle", ctx);
}

// XamLrcEncryptDecryptTitleMessage: semântica real pendente (issue #16)
void __imp__XamLrcEncryptDecryptTitleMessage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLrcEncryptDecryptTitleMessage", ctx);
    FH2_HLE_ONCE(XamLrcEncryptDecryptTitleMessage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLrcEncryptDecryptTitleMessage", ctx);
}

// XamLrcLogError: semântica real pendente (issue #16)
void __imp__XamLrcLogError(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLrcLogError", ctx);
    FH2_HLE_ONCE(XamLrcLogError, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLrcLogError", ctx);
}

// XamLrcLogSessionSummary: semântica real pendente (issue #16)
void __imp__XamLrcLogSessionSummary(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLrcLogSessionSummary", ctx);
    FH2_HLE_ONCE(XamLrcLogSessionSummary, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLrcLogSessionSummary", ctx);
}

// XamLrcSetTitlePort: semântica real pendente (issue #16)
void __imp__XamLrcSetTitlePort(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLrcSetTitlePort", ctx);
    FH2_HLE_ONCE(XamLrcSetTitlePort, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLrcSetTitlePort", ctx);
}

// XamLrcVerifyClientId: semântica real pendente (issue #16)
void __imp__XamLrcVerifyClientId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamLrcVerifyClientId", ctx);
    FH2_HLE_ONCE(XamLrcVerifyClientId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamLrcVerifyClientId", ctx);
}

// XamNotifyCreateListener: semântica real pendente (issue #16)
void __imp__XamNotifyCreateListener(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNotifyCreateListener", ctx);
    FH2_HLE_ONCE(XamNotifyCreateListener, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNotifyCreateListener", ctx);
}

// XamNuiCameraElevationGetAngle: semântica real pendente (issue #16)
void __imp__XamNuiCameraElevationGetAngle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiCameraElevationGetAngle", ctx);
    FH2_HLE_ONCE(XamNuiCameraElevationGetAngle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiCameraElevationGetAngle", ctx);
}

// XamNuiCameraElevationSetAngle: semântica real pendente (issue #16)
void __imp__XamNuiCameraElevationSetAngle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiCameraElevationSetAngle", ctx);
    FH2_HLE_ONCE(XamNuiCameraElevationSetAngle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiCameraElevationSetAngle", ctx);
}

// XamNuiCameraElevationStopMovement: semântica real pendente (issue #16)
void __imp__XamNuiCameraElevationStopMovement(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiCameraElevationStopMovement", ctx);
    FH2_HLE_ONCE(XamNuiCameraElevationStopMovement, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiCameraElevationStopMovement", ctx);
}

// XamNuiCameraRememberFloor: semântica real pendente (issue #16)
void __imp__XamNuiCameraRememberFloor(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiCameraRememberFloor", ctx);
    FH2_HLE_ONCE(XamNuiCameraRememberFloor, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiCameraRememberFloor", ctx);
}

// XamNuiCameraTiltGetStatus: semântica real pendente (issue #16)
void __imp__XamNuiCameraTiltGetStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiCameraTiltGetStatus", ctx);
    FH2_HLE_ONCE(XamNuiCameraTiltGetStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiCameraTiltGetStatus", ctx);
}

// XamNuiCameraTiltReportStatus: semântica real pendente (issue #16)
void __imp__XamNuiCameraTiltReportStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiCameraTiltReportStatus", ctx);
    FH2_HLE_ONCE(XamNuiCameraTiltReportStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiCameraTiltReportStatus", ctx);
}

// XamNuiCameraTiltSetCallback: semântica real pendente (issue #16)
void __imp__XamNuiCameraTiltSetCallback(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiCameraTiltSetCallback", ctx);
    FH2_HLE_ONCE(XamNuiCameraTiltSetCallback, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiCameraTiltSetCallback", ctx);
}

// XamNuiGetDeviceStatus: semântica real pendente (issue #16)
void __imp__XamNuiGetDeviceStatus(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiGetDeviceStatus", ctx);
    FH2_HLE_ONCE(XamNuiGetDeviceStatus, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiGetDeviceStatus", ctx);
}

// XamNuiIdentityGetSessionId: semântica real pendente (issue #16)
void __imp__XamNuiIdentityGetSessionId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamNuiIdentityGetSessionId", ctx);
    FH2_HLE_ONCE(XamNuiIdentityGetSessionId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamNuiIdentityGetSessionId", ctx);
}

// XamParseGamerTileKey: semântica real pendente (issue #16)
void __imp__XamParseGamerTileKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamParseGamerTileKey", ctx);
    FH2_HLE_ONCE(XamParseGamerTileKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamParseGamerTileKey", ctx);
}

// XamProfileCreateEnumerator: semântica real pendente (issue #16)
void __imp__XamProfileCreateEnumerator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamProfileCreateEnumerator", ctx);
    FH2_HLE_ONCE(XamProfileCreateEnumerator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamProfileCreateEnumerator", ctx);
}

// XamProfileEnumerate: semântica real pendente (issue #16)
void __imp__XamProfileEnumerate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamProfileEnumerate", ctx);
    FH2_HLE_ONCE(XamProfileEnumerate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamProfileEnumerate", ctx);
}

// XamReadBiometricData: semântica real pendente (issue #16)
void __imp__XamReadBiometricData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamReadBiometricData", ctx);
    FH2_HLE_ONCE(XamReadBiometricData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamReadBiometricData", ctx);
}

// XamReadTileToTexture: semântica real pendente (issue #16)
void __imp__XamReadTileToTexture(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamReadTileToTexture", ctx);
    FH2_HLE_ONCE(XamReadTileToTexture, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamReadTileToTexture", ctx);
}

// XamResetInactivity: semântica real pendente (issue #16)
void __imp__XamResetInactivity(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamResetInactivity", ctx);
    FH2_HLE_ONCE(XamResetInactivity, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamResetInactivity", ctx);
}

// XamSessionCreateHandle: semântica real pendente (issue #16)
void __imp__XamSessionCreateHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamSessionCreateHandle", ctx);
    FH2_HLE_ONCE(XamSessionCreateHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamSessionCreateHandle", ctx);
}

// XamSessionRefObjByHandle: semântica real pendente (issue #16)
void __imp__XamSessionRefObjByHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamSessionRefObjByHandle", ctx);
    FH2_HLE_ONCE(XamSessionRefObjByHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamSessionRefObjByHandle", ctx);
}

// XamShowAchievementsUI: semântica real pendente (issue #16)
void __imp__XamShowAchievementsUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowAchievementsUI", ctx);
    FH2_HLE_ONCE(XamShowAchievementsUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowAchievementsUI", ctx);
}

// XamShowDeviceSelectorUI: semântica real pendente (issue #16)
void __imp__XamShowDeviceSelectorUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowDeviceSelectorUI", ctx);
    FH2_HLE_ONCE(XamShowDeviceSelectorUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowDeviceSelectorUI", ctx);
}

// XamShowDirtyDiscErrorUI: semântica real pendente (issue #16)
void __imp__XamShowDirtyDiscErrorUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowDirtyDiscErrorUI", ctx);
    FH2_HLE_ONCE(XamShowDirtyDiscErrorUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowDirtyDiscErrorUI", ctx);
}

// XamShowFriendsUI: semântica real pendente (issue #16)
void __imp__XamShowFriendsUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowFriendsUI", ctx);
    FH2_HLE_ONCE(XamShowFriendsUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowFriendsUI", ctx);
}

// XamShowGameInviteUI: semântica real pendente (issue #16)
void __imp__XamShowGameInviteUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowGameInviteUI", ctx);
    FH2_HLE_ONCE(XamShowGameInviteUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowGameInviteUI", ctx);
}

// XamShowGamerCardUIForXUID: semântica real pendente (issue #16)
void __imp__XamShowGamerCardUIForXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowGamerCardUIForXUID", ctx);
    FH2_HLE_ONCE(XamShowGamerCardUIForXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowGamerCardUIForXUID", ctx);
}

// XamShowKeyboardUI: semântica real pendente (issue #16)
void __imp__XamShowKeyboardUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowKeyboardUI", ctx);
    FH2_HLE_ONCE(XamShowKeyboardUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowKeyboardUI", ctx);
}

// XamShowMarketplaceDownloadItemsUI: semântica real pendente (issue #16)
void __imp__XamShowMarketplaceDownloadItemsUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowMarketplaceDownloadItemsUI", ctx);
    FH2_HLE_ONCE(XamShowMarketplaceDownloadItemsUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowMarketplaceDownloadItemsUI", ctx);
}

// XamShowMarketplaceUI: semântica real pendente (issue #16)
void __imp__XamShowMarketplaceUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowMarketplaceUI", ctx);
    FH2_HLE_ONCE(XamShowMarketplaceUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowMarketplaceUI", ctx);
}

// XamShowMessageBoxUI: semântica real pendente (issue #16)
void __imp__XamShowMessageBoxUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowMessageBoxUI", ctx);
    FH2_HLE_ONCE(XamShowMessageBoxUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowMessageBoxUI", ctx);
}

// XamShowMessageBoxUIEx: semântica real pendente (issue #16)
void __imp__XamShowMessageBoxUIEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowMessageBoxUIEx", ctx);
    FH2_HLE_ONCE(XamShowMessageBoxUIEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowMessageBoxUIEx", ctx);
}

// XamShowNuiDeviceSelectorUI: semântica real pendente (issue #16)
void __imp__XamShowNuiDeviceSelectorUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowNuiDeviceSelectorUI", ctx);
    FH2_HLE_ONCE(XamShowNuiDeviceSelectorUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowNuiDeviceSelectorUI", ctx);
}

// XamShowNuiGuideUI: semântica real pendente (issue #16)
void __imp__XamShowNuiGuideUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowNuiGuideUI", ctx);
    FH2_HLE_ONCE(XamShowNuiGuideUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowNuiGuideUI", ctx);
}

// XamShowNuiMessageBoxUI: semântica real pendente (issue #16)
void __imp__XamShowNuiMessageBoxUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowNuiMessageBoxUI", ctx);
    FH2_HLE_ONCE(XamShowNuiMessageBoxUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowNuiMessageBoxUI", ctx);
}

// XamShowNuiSigninUI: semântica real pendente (issue #16)
void __imp__XamShowNuiSigninUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowNuiSigninUI", ctx);
    FH2_HLE_ONCE(XamShowNuiSigninUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowNuiSigninUI", ctx);
}

// XamShowNuiTroubleshooterUI: semântica real pendente (issue #16)
void __imp__XamShowNuiTroubleshooterUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowNuiTroubleshooterUI", ctx);
    FH2_HLE_ONCE(XamShowNuiTroubleshooterUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowNuiTroubleshooterUI", ctx);
}

// XamShowPlayersUI: semântica real pendente (issue #16)
void __imp__XamShowPlayersUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowPlayersUI", ctx);
    FH2_HLE_ONCE(XamShowPlayersUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowPlayersUI", ctx);
}

// XamShowSigninUI: semântica real pendente (issue #16)
void __imp__XamShowSigninUI(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamShowSigninUI", ctx);
    FH2_HLE_ONCE(XamShowSigninUI, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamShowSigninUI", ctx);
}

// XamTaskCloseHandle: semântica real pendente (issue #16)
void __imp__XamTaskCloseHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamTaskCloseHandle", ctx);
    FH2_HLE_ONCE(XamTaskCloseHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamTaskCloseHandle", ctx);
}

// XamTaskSchedule: semântica real pendente (issue #16)
void __imp__XamTaskSchedule(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamTaskSchedule", ctx);
    FH2_HLE_ONCE(XamTaskSchedule, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamTaskSchedule", ctx);
}

// XamTaskShouldExit: semântica real pendente (issue #16)
void __imp__XamTaskShouldExit(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamTaskShouldExit", ctx);
    FH2_HLE_ONCE(XamTaskShouldExit, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamTaskShouldExit", ctx);
}

// XamUserAreUsersFriends: semântica real pendente (issue #16)
void __imp__XamUserAreUsersFriends(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserAreUsersFriends", ctx);
    FH2_HLE_ONCE(XamUserAreUsersFriends, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserAreUsersFriends", ctx);
}

// XamUserCheckPrivilege: semântica real pendente (issue #16)
void __imp__XamUserCheckPrivilege(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserCheckPrivilege", ctx);
    FH2_HLE_ONCE(XamUserCheckPrivilege, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserCheckPrivilege", ctx);
}

// XamUserCreateAchievementEnumerator: semântica real pendente (issue #16)
void __imp__XamUserCreateAchievementEnumerator(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserCreateAchievementEnumerator", ctx);
    FH2_HLE_ONCE(XamUserCreateAchievementEnumerator, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserCreateAchievementEnumerator", ctx);
}

// XamUserGetAgeGroup: semântica real pendente (issue #16)
void __imp__XamUserGetAgeGroup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetAgeGroup", ctx);
    FH2_HLE_ONCE(XamUserGetAgeGroup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserGetAgeGroup", ctx);
}

// XamUserGetDeviceContext: semântica real pendente (issue #16)
void __imp__XamUserGetDeviceContext(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetDeviceContext", ctx);
    FH2_HLE_ONCE(XamUserGetDeviceContext, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserGetDeviceContext", ctx);
}

// XamUserGetIndexFromXUID: semântica real pendente (issue #16)
void __imp__XamUserGetIndexFromXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetIndexFromXUID", ctx);
    FH2_HLE_ONCE(XamUserGetIndexFromXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserGetIndexFromXUID", ctx);
}

// XamUserGetMembershipTierFromXUID: semântica real pendente (issue #16)
void __imp__XamUserGetMembershipTierFromXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetMembershipTierFromXUID", ctx);
    FH2_HLE_ONCE(XamUserGetMembershipTierFromXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserGetMembershipTierFromXUID", ctx);
}

// XamUserGetName: falha REAL — sem perfil assinado (offline real)
void __imp__XamUserGetName(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetName", ctx);
    FH2_HLE_ONCE(XamUserGetName, "sem perfil assinado (offline real)");
    ctx.r3.u32 = 0x80320098u;
    fh2::kern::traceReturn("XamUserGetName", ctx);
}

// XamUserGetOnlineCountryFromXUID: semântica real pendente (issue #16)
void __imp__XamUserGetOnlineCountryFromXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetOnlineCountryFromXUID", ctx);
    FH2_HLE_ONCE(XamUserGetOnlineCountryFromXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserGetOnlineCountryFromXUID", ctx);
}

// XamUserGetSigninInfo: falha REAL — sem perfil assinado (offline real)
void __imp__XamUserGetSigninInfo(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetSigninInfo", ctx);
    FH2_HLE_ONCE(XamUserGetSigninInfo, "sem perfil assinado (offline real)");
    ctx.r3.u32 = 0x80320098u;
    fh2::kern::traceReturn("XamUserGetSigninInfo", ctx);
}

// XamUserGetSigninState: semântica REAL (kernel_real.cpp)
void __imp__XamUserGetSigninState(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("XamUserGetSigninState", ctx);
    fh2::kern::real_XamUserGetSigninState(ctx, base);
    fh2::kern::traceReturn("XamUserGetSigninState", ctx);
}

// XamUserGetXUID: falha REAL — sem perfil assinado (offline real)
void __imp__XamUserGetXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserGetXUID", ctx);
    FH2_HLE_ONCE(XamUserGetXUID, "sem perfil assinado (offline real)");
    ctx.r3.u32 = 0x80320098u;
    fh2::kern::traceReturn("XamUserGetXUID", ctx);
}

// XamUserNuiEnableBiometric: semântica real pendente (issue #16)
void __imp__XamUserNuiEnableBiometric(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserNuiEnableBiometric", ctx);
    FH2_HLE_ONCE(XamUserNuiEnableBiometric, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserNuiEnableBiometric", ctx);
}

// XamUserNuiGetEnrollmentIndex: semântica real pendente (issue #16)
void __imp__XamUserNuiGetEnrollmentIndex(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserNuiGetEnrollmentIndex", ctx);
    FH2_HLE_ONCE(XamUserNuiGetEnrollmentIndex, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserNuiGetEnrollmentIndex", ctx);
}

// XamUserNuiGetUserIndex: semântica real pendente (issue #16)
void __imp__XamUserNuiGetUserIndex(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserNuiGetUserIndex", ctx);
    FH2_HLE_ONCE(XamUserNuiGetUserIndex, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserNuiGetUserIndex", ctx);
}

// XamUserReadProfileSettings: semântica real pendente (issue #16)
void __imp__XamUserReadProfileSettings(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamUserReadProfileSettings", ctx);
    FH2_HLE_ONCE(XamUserReadProfileSettings, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamUserReadProfileSettings", ctx);
}

// XamVoiceClose: semântica real pendente (issue #16)
void __imp__XamVoiceClose(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamVoiceClose", ctx);
    FH2_HLE_ONCE(XamVoiceClose, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamVoiceClose", ctx);
}

// XamVoiceCreate: semântica real pendente (issue #16)
void __imp__XamVoiceCreate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamVoiceCreate", ctx);
    FH2_HLE_ONCE(XamVoiceCreate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamVoiceCreate", ctx);
}

// XamVoiceHeadsetPresent: semântica real pendente (issue #16)
void __imp__XamVoiceHeadsetPresent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamVoiceHeadsetPresent", ctx);
    FH2_HLE_ONCE(XamVoiceHeadsetPresent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamVoiceHeadsetPresent", ctx);
}

// XamVoiceIsActiveProcess: semântica real pendente (issue #16)
void __imp__XamVoiceIsActiveProcess(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamVoiceIsActiveProcess", ctx);
    FH2_HLE_ONCE(XamVoiceIsActiveProcess, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamVoiceIsActiveProcess", ctx);
}

// XamVoiceSubmitPacket: semântica real pendente (issue #16)
void __imp__XamVoiceSubmitPacket(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamVoiceSubmitPacket", ctx);
    FH2_HLE_ONCE(XamVoiceSubmitPacket, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamVoiceSubmitPacket", ctx);
}

// XamWriteBiometricData: semântica real pendente (issue #16)
void __imp__XamWriteBiometricData(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamWriteBiometricData", ctx);
    FH2_HLE_ONCE(XamWriteBiometricData, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamWriteBiometricData", ctx);
}

// XamXStudioRequest: semântica real pendente (issue #16)
void __imp__XamXStudioRequest(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamXStudioRequest", ctx);
    FH2_HLE_ONCE(XamXStudioRequest, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamXStudioRequest", ctx);
}

// XamXlfsInitializeUploadQueue: semântica real pendente (issue #16)
void __imp__XamXlfsInitializeUploadQueue(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamXlfsInitializeUploadQueue", ctx);
    FH2_HLE_ONCE(XamXlfsInitializeUploadQueue, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamXlfsInitializeUploadQueue", ctx);
}

// XamXlfsMountUploadQueueInstance: semântica real pendente (issue #16)
void __imp__XamXlfsMountUploadQueueInstance(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamXlfsMountUploadQueueInstance", ctx);
    FH2_HLE_ONCE(XamXlfsMountUploadQueueInstance, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamXlfsMountUploadQueueInstance", ctx);
}

// XamXlfsUninitializeUploadQueue: semântica real pendente (issue #16)
void __imp__XamXlfsUninitializeUploadQueue(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamXlfsUninitializeUploadQueue", ctx);
    FH2_HLE_ONCE(XamXlfsUninitializeUploadQueue, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamXlfsUninitializeUploadQueue", ctx);
}

// XamXlfsUnmountUploadQueueInstance: semântica real pendente (issue #16)
void __imp__XamXlfsUnmountUploadQueueInstance(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XamXlfsUnmountUploadQueueInstance", ctx);
    FH2_HLE_ONCE(XamXlfsUnmountUploadQueueInstance, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XamXlfsUnmountUploadQueueInstance", ctx);
}

// XampXAuthIsLocalSocketAllowed: semântica real pendente (issue #16)
void __imp__XampXAuthIsLocalSocketAllowed(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XampXAuthIsLocalSocketAllowed", ctx);
    FH2_HLE_ONCE(XampXAuthIsLocalSocketAllowed, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XampXAuthIsLocalSocketAllowed", ctx);
}

// XeCryptSha: semântica real pendente (issue #16)
void __imp__XeCryptSha(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha", ctx);
    FH2_HLE_ONCE(XeCryptSha, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha", ctx);
}

// XeCryptSha256Final: semântica real pendente (issue #16)
void __imp__XeCryptSha256Final(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha256Final", ctx);
    FH2_HLE_ONCE(XeCryptSha256Final, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha256Final", ctx);
}

// XeCryptSha256Init: semântica real pendente (issue #16)
void __imp__XeCryptSha256Init(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha256Init", ctx);
    FH2_HLE_ONCE(XeCryptSha256Init, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha256Init", ctx);
}

// XeCryptSha256Update: semântica real pendente (issue #16)
void __imp__XeCryptSha256Update(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha256Update", ctx);
    FH2_HLE_ONCE(XeCryptSha256Update, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha256Update", ctx);
}

// XeCryptSha384Final: semântica real pendente (issue #16)
void __imp__XeCryptSha384Final(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha384Final", ctx);
    FH2_HLE_ONCE(XeCryptSha384Final, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha384Final", ctx);
}

// XeCryptSha384Init: semântica real pendente (issue #16)
void __imp__XeCryptSha384Init(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha384Init", ctx);
    FH2_HLE_ONCE(XeCryptSha384Init, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha384Init", ctx);
}

// XeCryptSha384Update: semântica real pendente (issue #16)
void __imp__XeCryptSha384Update(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha384Update", ctx);
    FH2_HLE_ONCE(XeCryptSha384Update, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha384Update", ctx);
}

// XeCryptSha512Final: semântica real pendente (issue #16)
void __imp__XeCryptSha512Final(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha512Final", ctx);
    FH2_HLE_ONCE(XeCryptSha512Final, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha512Final", ctx);
}

// XeCryptSha512Init: semântica real pendente (issue #16)
void __imp__XeCryptSha512Init(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha512Init", ctx);
    FH2_HLE_ONCE(XeCryptSha512Init, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha512Init", ctx);
}

// XeCryptSha512Update: semântica real pendente (issue #16)
void __imp__XeCryptSha512Update(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptSha512Update", ctx);
    FH2_HLE_ONCE(XeCryptSha512Update, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptSha512Update", ctx);
}

// XeCryptShaFinal: semântica real pendente (issue #16)
void __imp__XeCryptShaFinal(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptShaFinal", ctx);
    FH2_HLE_ONCE(XeCryptShaFinal, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptShaFinal", ctx);
}

// XeCryptShaInit: semântica real pendente (issue #16)
void __imp__XeCryptShaInit(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptShaInit", ctx);
    FH2_HLE_ONCE(XeCryptShaInit, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptShaInit", ctx);
}

// XeCryptShaUpdate: semântica real pendente (issue #16)
void __imp__XeCryptShaUpdate(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeCryptShaUpdate", ctx);
    FH2_HLE_ONCE(XeCryptShaUpdate, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeCryptShaUpdate", ctx);
}

// XeKeysAesCbcUsingKey: semântica real pendente (issue #16)
void __imp__XeKeysAesCbcUsingKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeKeysAesCbcUsingKey", ctx);
    FH2_HLE_ONCE(XeKeysAesCbcUsingKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeKeysAesCbcUsingKey", ctx);
}

// XeKeysConsolePrivateKeySign: semântica real pendente (issue #16)
void __imp__XeKeysConsolePrivateKeySign(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeKeysConsolePrivateKeySign", ctx);
    FH2_HLE_ONCE(XeKeysConsolePrivateKeySign, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeKeysConsolePrivateKeySign", ctx);
}

// XeKeysConsoleSignatureVerification: semântica real pendente (issue #16)
void __imp__XeKeysConsoleSignatureVerification(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeKeysConsoleSignatureVerification", ctx);
    FH2_HLE_ONCE(XeKeysConsoleSignatureVerification, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeKeysConsoleSignatureVerification", ctx);
}

// XeKeysGetConsoleID: semântica real pendente (issue #16)
void __imp__XeKeysGetConsoleID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeKeysGetConsoleID", ctx);
    FH2_HLE_ONCE(XeKeysGetConsoleID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeKeysGetConsoleID", ctx);
}

// XeKeysObscureKey: semântica real pendente (issue #16)
void __imp__XeKeysObscureKey(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XeKeysObscureKey", ctx);
    FH2_HLE_ONCE(XeKeysObscureKey, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XeKeysObscureKey", ctx);
}

// XexCheckExecutablePrivilege: semântica real pendente (issue #16)
void __imp__XexCheckExecutablePrivilege(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XexCheckExecutablePrivilege", ctx);
    FH2_HLE_ONCE(XexCheckExecutablePrivilege, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XexCheckExecutablePrivilege", ctx);
}

// XexGetModuleHandle: semântica real pendente (issue #16)
void __imp__XexGetModuleHandle(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XexGetModuleHandle", ctx);
    FH2_HLE_ONCE(XexGetModuleHandle, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XexGetModuleHandle", ctx);
}

// XexGetModuleSection: semântica real pendente (issue #16)
void __imp__XexGetModuleSection(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XexGetModuleSection", ctx);
    FH2_HLE_ONCE(XexGetModuleSection, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XexGetModuleSection", ctx);
}

// XexGetProcedureAddress: falha REAL — STATUS_PROCEDURE_NOT_FOUND — export tables pendentes
void __imp__XexGetProcedureAddress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XexGetProcedureAddress", ctx);
    FH2_HLE_ONCE(XexGetProcedureAddress, "STATUS_PROCEDURE_NOT_FOUND — export tables pendentes");
    ctx.r3.u32 = 0x8007007Eu;
    fh2::kern::traceReturn("XexGetProcedureAddress", ctx);
}

// XexLoadImage: semântica real pendente (issue #16)
void __imp__XexLoadImage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XexLoadImage", ctx);
    FH2_HLE_ONCE(XexLoadImage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XexLoadImage", ctx);
}

// XexLoadImageHeaders: semântica real pendente (issue #16)
void __imp__XexLoadImageHeaders(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XexLoadImageHeaders", ctx);
    FH2_HLE_ONCE(XexLoadImageHeaders, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XexLoadImageHeaders", ctx);
}

// XexUnloadImage: semântica real pendente (issue #16)
void __imp__XexUnloadImage(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("XexUnloadImage", ctx);
    FH2_HLE_ONCE(XexUnloadImage, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("XexUnloadImage", ctx);
}

// __C_specific_handler: semântica real pendente (issue #16)
void __imp____C_specific_handler(PPCContext& ctx, uint8_t* base) {
    (void)base;
    fh2::kern::traceCall("__C_specific_handler", ctx);
    FH2_HLE_ONCE(__C_specific_handler, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
    fh2::kern::traceReturn("__C_specific_handler", ctx);
}

// _snprintf: semântica REAL (kernel_real.cpp)
void __imp___snprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("_snprintf", ctx);
    fh2::kern::real__snprintf(ctx, base);
    fh2::kern::traceReturn("_snprintf", ctx);
}

// _vsnprintf: semântica REAL (kernel_real.cpp)
void __imp___vsnprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("_vsnprintf", ctx);
    fh2::kern::real__vsnprintf(ctx, base);
    fh2::kern::traceReturn("_vsnprintf", ctx);
}

// sprintf: semântica REAL (kernel_real.cpp)
void __imp__sprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("sprintf", ctx);
    fh2::kern::real_sprintf(ctx, base);
    fh2::kern::traceReturn("sprintf", ctx);
}

// vsprintf: semântica REAL (kernel_real.cpp)
void __imp__vsprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("vsprintf", ctx);
    fh2::kern::real_vsprintf(ctx, base);
    fh2::kern::traceReturn("vsprintf", ctx);
}

// vswprintf: semântica REAL (kernel_real.cpp)
void __imp__vswprintf(PPCContext& ctx, uint8_t* base) {
    fh2::kern::traceCall("vswprintf", ctx);
    fh2::kern::real_vswprintf(ctx, base);
    fh2::kern::traceReturn("vswprintf", ctx);
}

#endif // FH2_HAS_RECOMP
