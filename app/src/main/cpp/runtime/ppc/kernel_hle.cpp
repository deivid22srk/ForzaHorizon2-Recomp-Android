// kernel_hle.cpp — stubs HLE dos imports de xboxkrnl.exe/xam.xex usados pelo FH2
//
// GERADO por tools/gen_kernel_hle.py — 388 símbolos exigidos por
// ppc_func_mapping.cpp (ld.lld para no limite de 20 erros, mas o conjunto
// completo vem da análise do mapping). Cada stub: log-once + retorno padrão
// de sucesso (NTSTATUS 0). Semântica real = issue #16. NÃO EDITAR À MÃO.
#if FH2_HAS_RECOMP

#include <android/log.h>
#include "ppc_config.h"
#include "ppc_context.h"

#define HLOG(...) __android_log_print(ANDROID_LOG_WARN, "FH2/HLE", __VA_ARGS__)

#define FH2_HLE_ONCE(name, note) do { static bool _logged_##name = false; if (!_logged_##name) { _logged_##name = true; HLOG("HLE stub: %s — %s", #name, note); } } while (0)

// DbgBreakPoint: semântica real pendente (issue #16)
void __imp__DbgBreakPoint(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(DbgBreakPoint, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// DbgPrint: debug output do guest
void __imp__DbgPrint(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(DbgPrint, "debug output do guest");
    ctx.r3.u32 = 0;
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

// ExAllocatePool: retorna NULL até o allocator HLE (issue #16)
void __imp__ExAllocatePool(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ExAllocatePool, "retorna NULL até o allocator HLE (issue #16)");
    ctx.r3.u32 = 0;
}

// ExAllocatePoolTypeWithTag: retorna NULL até o allocator HLE (issue #16)
void __imp__ExAllocatePoolTypeWithTag(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ExAllocatePoolTypeWithTag, "retorna NULL até o allocator HLE (issue #16)");
    ctx.r3.u32 = 0;
}

// ExCreateThread: semântica real pendente (issue #16)
void __imp__ExCreateThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ExCreateThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// ExFreePool: semântica real pendente (issue #16)
void __imp__ExFreePool(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ExFreePool, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// ExTerminateThread: semântica real pendente (issue #16)
void __imp__ExTerminateThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(ExTerminateThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// FscSetCacheElementCount: semântica real pendente (issue #16)
void __imp__FscSetCacheElementCount(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(FscSetCacheElementCount, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// HalReturnToFirmware: guest pediu reboot
void __imp__HalReturnToFirmware(PPCContext& ctx, uint8_t* base) {
    (void)base;
    HLOG("HLE FATAL: %s — guest pediu reboot", "HalReturnToFirmware");
    ctx.r3.u32 = 0;
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

// KeBugCheck: caminho fatal do guest
void __imp__KeBugCheck(PPCContext& ctx, uint8_t* base) {
    (void)base;
    HLOG("HLE FATAL: %s — caminho fatal do guest", "KeBugCheck");
    ctx.r3.u32 = 0;
}

// KeBugCheckEx: caminho fatal do guest
void __imp__KeBugCheckEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    HLOG("HLE FATAL: %s — caminho fatal do guest", "KeBugCheckEx");
    ctx.r3.u32 = 0;
}

// KeDelayExecutionThread: retorna imediatamente (sync HLE pendente)
void __imp__KeDelayExecutionThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeDelayExecutionThread, "retorna imediatamente (sync HLE pendente)");
    ctx.r3.u32 = 0;
}

// KeEnterCriticalRegion: semântica real pendente (issue #16)
void __imp__KeEnterCriticalRegion(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeEnterCriticalRegion, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeGetCurrentProcessType: semântica real pendente (issue #16)
void __imp__KeGetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeGetCurrentProcessType, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeInitializeDpc: semântica real pendente (issue #16)
void __imp__KeInitializeDpc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeInitializeDpc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeInitializeMutant: semântica real pendente (issue #16)
void __imp__KeInitializeMutant(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeInitializeMutant, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// KeQueryPerformanceFrequency: semântica real pendente (issue #16)
void __imp__KeQueryPerformanceFrequency(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeQueryPerformanceFrequency, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeQuerySystemTime: semântica real pendente (issue #16)
void __imp__KeQuerySystemTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeQuerySystemTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeReleaseMutant: semântica real pendente (issue #16)
void __imp__KeReleaseMutant(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeReleaseMutant, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeReleaseSpinLockFromRaisedIrql: semântica real pendente (issue #16)
void __imp__KeReleaseSpinLockFromRaisedIrql(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeReleaseSpinLockFromRaisedIrql, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeResetEvent: semântica real pendente (issue #16)
void __imp__KeResetEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeResetEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeRestoreFloatingPointState: semântica real pendente (issue #16)
void __imp__KeRestoreFloatingPointState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeRestoreFloatingPointState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeResumeThread: semântica real pendente (issue #16)
void __imp__KeResumeThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeResumeThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// KeSetCurrentProcessType: semântica real pendente (issue #16)
void __imp__KeSetCurrentProcessType(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeSetCurrentProcessType, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeSetCurrentStackPointers: semântica real pendente (issue #16)
void __imp__KeSetCurrentStackPointers(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeSetCurrentStackPointers, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeSetEvent: semântica real pendente (issue #16)
void __imp__KeSetEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeSetEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeTlsAlloc: semântica real pendente (issue #16)
void __imp__KeTlsAlloc(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeTlsAlloc, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeTlsFree: semântica real pendente (issue #16)
void __imp__KeTlsFree(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeTlsFree, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeTlsGetValue: semântica real pendente (issue #16)
void __imp__KeTlsGetValue(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeTlsGetValue, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeTlsSetValue: semântica real pendente (issue #16)
void __imp__KeTlsSetValue(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeTlsSetValue, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeUnlockL2: semântica real pendente (issue #16)
void __imp__KeUnlockL2(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeUnlockL2, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// KeWaitForMultipleObjects: retorna imediatamente (sync HLE pendente)
void __imp__KeWaitForMultipleObjects(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeWaitForMultipleObjects, "retorna imediatamente (sync HLE pendente)");
    ctx.r3.u32 = 0;
}

// KeWaitForSingleObject: semântica real pendente (issue #16)
void __imp__KeWaitForSingleObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(KeWaitForSingleObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// MmAllocatePhysicalMemoryEx: semântica real pendente (issue #16)
void __imp__MmAllocatePhysicalMemoryEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmAllocatePhysicalMemoryEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmCreateKernelStack: semântica real pendente (issue #16)
void __imp__MmCreateKernelStack(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmCreateKernelStack, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmDeleteKernelStack: semântica real pendente (issue #16)
void __imp__MmDeleteKernelStack(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmDeleteKernelStack, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmFreePhysicalMemory: semântica real pendente (issue #16)
void __imp__MmFreePhysicalMemory(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmFreePhysicalMemory, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// MmGetPhysicalAddress: semântica real pendente (issue #16)
void __imp__MmGetPhysicalAddress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmGetPhysicalAddress, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// MmQueryAllocationSize: semântica real pendente (issue #16)
void __imp__MmQueryAllocationSize(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(MmQueryAllocationSize, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// NetDll_WSAStartup: semântica real pendente (issue #16)
void __imp__NetDll_WSAStartup(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NetDll_WSAStartup, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// NtAllocateVirtualMemory: semântica real pendente (issue #16)
void __imp__NtAllocateVirtualMemory(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtAllocateVirtualMemory, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtCancelIoFile: semântica real pendente (issue #16)
void __imp__NtCancelIoFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCancelIoFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtCancelTimer: semântica real pendente (issue #16)
void __imp__NtCancelTimer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCancelTimer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtClearEvent: semântica real pendente (issue #16)
void __imp__NtClearEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtClearEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtClose: semântica real pendente (issue #16)
void __imp__NtClose(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtClose, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtCreateEvent: semântica real pendente (issue #16)
void __imp__NtCreateEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCreateEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtCreateFile: semântica real pendente (issue #16)
void __imp__NtCreateFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCreateFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtCreateMutant: semântica real pendente (issue #16)
void __imp__NtCreateMutant(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCreateMutant, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtCreateSemaphore: semântica real pendente (issue #16)
void __imp__NtCreateSemaphore(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCreateSemaphore, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtCreateTimer: semântica real pendente (issue #16)
void __imp__NtCreateTimer(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtCreateTimer, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtDeviceIoControlFile: semântica real pendente (issue #16)
void __imp__NtDeviceIoControlFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtDeviceIoControlFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtDuplicateObject: semântica real pendente (issue #16)
void __imp__NtDuplicateObject(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtDuplicateObject, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtFlushBuffersFile: semântica real pendente (issue #16)
void __imp__NtFlushBuffersFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtFlushBuffersFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtFreeVirtualMemory: semântica real pendente (issue #16)
void __imp__NtFreeVirtualMemory(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtFreeVirtualMemory, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtOpenFile: semântica real pendente (issue #16)
void __imp__NtOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtOpenFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtQueryDirectoryFile: semântica real pendente (issue #16)
void __imp__NtQueryDirectoryFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryDirectoryFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtQueryFullAttributesFile: semântica real pendente (issue #16)
void __imp__NtQueryFullAttributesFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryFullAttributesFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtQueryInformationFile: semântica real pendente (issue #16)
void __imp__NtQueryInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryInformationFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtQueryVirtualMemory: semântica real pendente (issue #16)
void __imp__NtQueryVirtualMemory(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryVirtualMemory, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtQueryVolumeInformationFile: semântica real pendente (issue #16)
void __imp__NtQueryVolumeInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtQueryVolumeInformationFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtReadFile: semântica real pendente (issue #16)
void __imp__NtReadFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtReadFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtReadFileScatter: semântica real pendente (issue #16)
void __imp__NtReadFileScatter(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtReadFileScatter, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtReleaseMutant: semântica real pendente (issue #16)
void __imp__NtReleaseMutant(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtReleaseMutant, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtReleaseSemaphore: semântica real pendente (issue #16)
void __imp__NtReleaseSemaphore(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtReleaseSemaphore, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtResumeThread: semântica real pendente (issue #16)
void __imp__NtResumeThread(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtResumeThread, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtSetEvent: semântica real pendente (issue #16)
void __imp__NtSetEvent(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtSetEvent, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtSetInformationFile: semântica real pendente (issue #16)
void __imp__NtSetInformationFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtSetInformationFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtSetTimerEx: semântica real pendente (issue #16)
void __imp__NtSetTimerEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtSetTimerEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtSignalAndWaitForSingleObjectEx: semântica real pendente (issue #16)
void __imp__NtSignalAndWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtSignalAndWaitForSingleObjectEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtWaitForMultipleObjectsEx: semântica real pendente (issue #16)
void __imp__NtWaitForMultipleObjectsEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtWaitForMultipleObjectsEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtWaitForSingleObjectEx: semântica real pendente (issue #16)
void __imp__NtWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtWaitForSingleObjectEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtWriteFile: semântica real pendente (issue #16)
void __imp__NtWriteFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtWriteFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtWriteFileGather: semântica real pendente (issue #16)
void __imp__NtWriteFileGather(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtWriteFileGather, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// NtYieldExecution: semântica real pendente (issue #16)
void __imp__NtYieldExecution(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(NtYieldExecution, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// RtlCompareMemory: semântica real pendente (issue #16)
void __imp__RtlCompareMemory(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlCompareMemory, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlCompareMemoryUlong: semântica real pendente (issue #16)
void __imp__RtlCompareMemoryUlong(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlCompareMemoryUlong, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlCompareStringN: semântica real pendente (issue #16)
void __imp__RtlCompareStringN(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlCompareStringN, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlEnterCriticalSection: semântica real pendente (issue #16)
void __imp__RtlEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlEnterCriticalSection, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlFillMemoryUlong: semântica real pendente (issue #16)
void __imp__RtlFillMemoryUlong(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlFillMemoryUlong, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlFreeAnsiString: semântica real pendente (issue #16)
void __imp__RtlFreeAnsiString(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlFreeAnsiString, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlImageXexHeaderField: semântica real pendente (issue #16)
void __imp__RtlImageXexHeaderField(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlImageXexHeaderField, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlInitAnsiString: semântica real pendente (issue #16)
void __imp__RtlInitAnsiString(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlInitAnsiString, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlInitUnicodeString: semântica real pendente (issue #16)
void __imp__RtlInitUnicodeString(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlInitUnicodeString, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlInitializeCriticalSection: semântica real pendente (issue #16)
void __imp__RtlInitializeCriticalSection(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlInitializeCriticalSection, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlInitializeCriticalSectionAndSpinCount: semântica real pendente (issue #16)
void __imp__RtlInitializeCriticalSectionAndSpinCount(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlInitializeCriticalSectionAndSpinCount, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlLeaveCriticalSection: semântica real pendente (issue #16)
void __imp__RtlLeaveCriticalSection(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlLeaveCriticalSection, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlMultiByteToUnicodeN: semântica real pendente (issue #16)
void __imp__RtlMultiByteToUnicodeN(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlMultiByteToUnicodeN, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlNtStatusToDosError: semântica real pendente (issue #16)
void __imp__RtlNtStatusToDosError(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlNtStatusToDosError, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlRaiseException: exceptions de guest não suportadas (issue #15)
void __imp__RtlRaiseException(PPCContext& ctx, uint8_t* base) {
    (void)base;
    HLOG("HLE FATAL: %s — exceptions de guest não suportadas (issue #15)", "RtlRaiseException");
    ctx.r3.u32 = 0;
}

// RtlTimeFieldsToTime: semântica real pendente (issue #16)
void __imp__RtlTimeFieldsToTime(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlTimeFieldsToTime, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlTimeToTimeFields: semântica real pendente (issue #16)
void __imp__RtlTimeToTimeFields(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlTimeToTimeFields, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlTryEnterCriticalSection: semântica real pendente (issue #16)
void __imp__RtlTryEnterCriticalSection(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlTryEnterCriticalSection, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlUnicodeStringToAnsiString: semântica real pendente (issue #16)
void __imp__RtlUnicodeStringToAnsiString(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlUnicodeStringToAnsiString, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlUnicodeToMultiByteN: semântica real pendente (issue #16)
void __imp__RtlUnicodeToMultiByteN(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlUnicodeToMultiByteN, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlUnwind: semântica real pendente (issue #16)
void __imp__RtlUnwind(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlUnwind, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// RtlUpcaseUnicodeChar: semântica real pendente (issue #16)
void __imp__RtlUpcaseUnicodeChar(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(RtlUpcaseUnicodeChar, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// StfsControlDevice: semântica real pendente (issue #16)
void __imp__StfsControlDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(StfsControlDevice, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// StfsCreateDevice: semântica real pendente (issue #16)
void __imp__StfsCreateDevice(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(StfsCreateDevice, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// VdSwap: semântica real pendente (issue #16)
void __imp__VdSwap(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(VdSwap, "semântica real pendente (issue #16)");
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

// XamCacheOpenFile: semântica real pendente (issue #16)
void __imp__XamCacheOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamCacheOpenFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// XamContentCreateEx: semântica real pendente (issue #16)
void __imp__XamContentCreateEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentCreateEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// XamContentGetDeviceState: semântica real pendente (issue #16)
void __imp__XamContentGetDeviceState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentGetDeviceState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentGetLicenseMask: semântica real pendente (issue #16)
void __imp__XamContentGetLicenseMask(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentGetLicenseMask, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentOpenFile: semântica real pendente (issue #16)
void __imp__XamContentOpenFile(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentOpenFile, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamContentResolve: semântica real pendente (issue #16)
void __imp__XamContentResolve(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamContentResolve, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// XamGetCurrentTitleId: semântica real pendente (issue #16)
void __imp__XamGetCurrentTitleId(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamGetCurrentTitleId, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// XamInputGetCapabilities: semântica real pendente (issue #16)
void __imp__XamInputGetCapabilities(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamInputGetCapabilities, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamInputGetCapabilitiesEx: semântica real pendente (issue #16)
void __imp__XamInputGetCapabilitiesEx(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamInputGetCapabilitiesEx, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamInputGetState: semântica real pendente (issue #16)
void __imp__XamInputGetState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamInputGetState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamInputRawState: semântica real pendente (issue #16)
void __imp__XamInputRawState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamInputRawState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamInputSetState: semântica real pendente (issue #16)
void __imp__XamInputSetState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamInputSetState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// XamUserGetName: semântica real pendente (issue #16)
void __imp__XamUserGetName(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetName, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetOnlineCountryFromXUID: semântica real pendente (issue #16)
void __imp__XamUserGetOnlineCountryFromXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetOnlineCountryFromXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetSigninInfo: semântica real pendente (issue #16)
void __imp__XamUserGetSigninInfo(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetSigninInfo, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetSigninState: semântica real pendente (issue #16)
void __imp__XamUserGetSigninState(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetSigninState, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// XamUserGetXUID: semântica real pendente (issue #16)
void __imp__XamUserGetXUID(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XamUserGetXUID, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// XexGetProcedureAddress: semântica real pendente (issue #16)
void __imp__XexGetProcedureAddress(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(XexGetProcedureAddress, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
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

// _snprintf: semântica real pendente (issue #16)
void __imp___snprintf(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(_snprintf, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// _vsnprintf: semântica real pendente (issue #16)
void __imp___vsnprintf(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(_vsnprintf, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// sprintf: semântica real pendente (issue #16)
void __imp__sprintf(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(sprintf, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// vsprintf: semântica real pendente (issue #16)
void __imp__vsprintf(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(vsprintf, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

// vswprintf: semântica real pendente (issue #16)
void __imp__vswprintf(PPCContext& ctx, uint8_t* base) {
    (void)base;
    FH2_HLE_ONCE(vswprintf, "semântica real pendente (issue #16)");
    ctx.r3.u32 = 0;
}

#endif // FH2_HAS_RECOMP
