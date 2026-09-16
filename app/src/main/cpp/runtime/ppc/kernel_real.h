// kernel_real.h — implementações REAIS dos imports do xboxkrnl (issue #16)
//
// Cada real_<Nome> é chamado pelo kernel_hle.cpp (gerado) no lugar do stub.
// Aqui vive o glue com PPCContext: leitura de argumentos (r3..r10), escrita
// de retornos e acesso à memória guest (base+addr, big-endian).
#pragma once

#if FH2_HAS_RECOMP

#include "ppc_config.h"
#include "ppc_context.h"

namespace fh2::kern {

void real_ExAllocatePool(PPCContext& ctx, uint8_t* base);
void real_ExAllocatePoolTypeWithTag(PPCContext& ctx, uint8_t* base);
void real_ExFreePool(PPCContext& ctx, uint8_t* base);
void real_ExCreateThread(PPCContext& ctx, uint8_t* base);
void real_ExTerminateThread(PPCContext& ctx, uint8_t* base);
void real_KeDelayExecutionThread(PPCContext& ctx, uint8_t* base);
void real_KeQueryPerformanceFrequency(PPCContext& ctx, uint8_t* base);
void real_KeQuerySystemTime(PPCContext& ctx, uint8_t* base);
void real_KeInitializeMutant(PPCContext& ctx, uint8_t* base);
void real_KeReleaseMutant(PPCContext& ctx, uint8_t* base);
void real_KeResetEvent(PPCContext& ctx, uint8_t* base);
void real_KeResumeThread(PPCContext& ctx, uint8_t* base);
void real_KeSetEvent(PPCContext& ctx, uint8_t* base);
void real_KeTlsAlloc(PPCContext& ctx, uint8_t* base);
void real_KeTlsFree(PPCContext& ctx, uint8_t* base);
void real_KeTlsGetValue(PPCContext& ctx, uint8_t* base);
void real_KeTlsSetValue(PPCContext& ctx, uint8_t* base);
void real_KeWaitForMultipleObjects(PPCContext& ctx, uint8_t* base);
void real_KeWaitForSingleObject(PPCContext& ctx, uint8_t* base);
void real_KeBugCheck(PPCContext& ctx, uint8_t* base);
void real_KeBugCheckEx(PPCContext& ctx, uint8_t* base);
void real_HalReturnToFirmware(PPCContext& ctx, uint8_t* base);
void real_KeGetCurrentProcessType(PPCContext& ctx, uint8_t* base);
void real_KeSetCurrentProcessType(PPCContext& ctx, uint8_t* base);
void real_KeGetCurrentThread(PPCContext& ctx, uint8_t* base);
void real_NtAllocateVirtualMemory(PPCContext& ctx, uint8_t* base);
void real_NtFreeVirtualMemory(PPCContext& ctx, uint8_t* base);
void real_NtQueryVirtualMemory(PPCContext& ctx, uint8_t* base);
void real_NtCreateEvent(PPCContext& ctx, uint8_t* base);
void real_NtSetEvent(PPCContext& ctx, uint8_t* base);
void real_NtClearEvent(PPCContext& ctx, uint8_t* base);
void real_NtCreateSemaphore(PPCContext& ctx, uint8_t* base);
void real_NtReleaseSemaphore(PPCContext& ctx, uint8_t* base);
void real_NtCreateMutant(PPCContext& ctx, uint8_t* base);
void real_NtReleaseMutant(PPCContext& ctx, uint8_t* base);
void real_NtWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base);
void real_NtWaitForMultipleObjectsEx(PPCContext& ctx, uint8_t* base);
void real_NtSignalAndWaitForSingleObjectEx(PPCContext& ctx, uint8_t* base);
void real_NtYieldExecution(PPCContext& ctx, uint8_t* base);
void real_NtClose(PPCContext& ctx, uint8_t* base);
void real_MmAllocatePhysicalMemoryEx(PPCContext& ctx, uint8_t* base);
void real_MmFreePhysicalMemory(PPCContext& ctx, uint8_t* base);
void real_MmGetPhysicalAddress(PPCContext& ctx, uint8_t* base);
void real_MmQueryAllocationSize(PPCContext& ctx, uint8_t* base);
void real_MmQueryStatistics(PPCContext& ctx, uint8_t* base);
void real_MmCreateKernelStack(PPCContext& ctx, uint8_t* base);
void real_MmDeleteKernelStack(PPCContext& ctx, uint8_t* base);
void real_RtlInitializeCriticalSection(PPCContext& ctx, uint8_t* base);
void real_RtlInitializeCriticalSectionAndSpinCount(PPCContext& ctx, uint8_t* base);
void real_RtlEnterCriticalSection(PPCContext& ctx, uint8_t* base);
void real_RtlLeaveCriticalSection(PPCContext& ctx, uint8_t* base);
void real_RtlTryEnterCriticalSection(PPCContext& ctx, uint8_t* base);
void real_RtlCompareMemory(PPCContext& ctx, uint8_t* base);
void real_RtlCompareMemoryUlong(PPCContext& ctx, uint8_t* base);
void real_RtlFillMemoryUlong(PPCContext& ctx, uint8_t* base);
void real_RtlTimeFieldsToTime(PPCContext& ctx, uint8_t* base);
void real_RtlTimeToTimeFields(PPCContext& ctx, uint8_t* base);
void real_RtlInitAnsiString(PPCContext& ctx, uint8_t* base);
void real_RtlInitUnicodeString(PPCContext& ctx, uint8_t* base);
void real_RtlMultiByteToUnicodeN(PPCContext& ctx, uint8_t* base);
void real_RtlUnicodeToMultiByteN(PPCContext& ctx, uint8_t* base);
void real_RtlUpcaseUnicodeChar(PPCContext& ctx, uint8_t* base);
void real_RtlUnicodeStringToAnsiString(PPCContext& ctx, uint8_t* base);
void real_RtlFreeAnsiString(PPCContext& ctx, uint8_t* base);
void real_RtlNtStatusToDosError(PPCContext& ctx, uint8_t* base);

// módulos XEX (semântica real — issue #16)
void real_XexGetModuleHandle(PPCContext& ctx, uint8_t* base);
void real_XexGetModuleSection(PPCContext& ctx, uint8_t* base);
void real_XexCheckExecutablePrivilege(PPCContext& ctx, uint8_t* base);

// arquivos — Nt I/O real sobre a pasta SAF/local (issues #16 + #19)
void real_NtCreateFile(PPCContext& ctx, uint8_t* base);
void real_NtOpenFile(PPCContext& ctx, uint8_t* base);
void real_NtReadFile(PPCContext& ctx, uint8_t* base);
void real_NtReadFileScatter(PPCContext& ctx, uint8_t* base);
void real_NtWriteFile(PPCContext& ctx, uint8_t* base);
void real_NtQueryInformationFile(PPCContext& ctx, uint8_t* base);
void real_NtQueryVolumeInformationFile(PPCContext& ctx, uint8_t* base);
void real_NtSetInformationFile(PPCContext& ctx, uint8_t* base);
void real_NtFlushBuffersFile(PPCContext& ctx, uint8_t* base);
void real_NtQueryFullAttributesFile(PPCContext& ctx, uint8_t* base);
void real_DbgPrint(PPCContext& ctx, uint8_t* base);
void real_DbgBreakPoint(PPCContext& ctx, uint8_t* base);
void real__snprintf(PPCContext& ctx, uint8_t* base);
void real__vsnprintf(PPCContext& ctx, uint8_t* base);
void real_sprintf(PPCContext& ctx, uint8_t* base);
void real_vsprintf(PPCContext& ctx, uint8_t* base);
void real_vswprintf(PPCContext& ctx, uint8_t* base);
void real_XamInputGetState(PPCContext& ctx, uint8_t* base);
void real_XamInputGetCapabilities(PPCContext& ctx, uint8_t* base);
void real_XamInputGetCapabilitiesEx(PPCContext& ctx, uint8_t* base);
void real_XamInputSetState(PPCContext& ctx, uint8_t* base);
void real_XamGetCurrentTitleId(PPCContext& ctx, uint8_t* base);
void real_XamLoaderGetLaunchDataSize(PPCContext& ctx, uint8_t* base);
void real_XamLoaderGetLaunchData(PPCContext& ctx, uint8_t* base);
void real_XamUserGetSigninState(PPCContext& ctx, uint8_t* base);

// ------------------------------------------------------- rastreio HLE
//
// Diagnóstico REAL do boot: cada import chamado pelo guest é registrado
// (nome, r3..r6 de entrada, r3 de retorno) num ring buffer. Os primeiros
// N segundos/calls também vão para o log; no encerramento (HalReturnTo
// Firmware/ExTerminateThread/bugcheck/fim do main) o ring é despejado —
// mostrando a sequência EXATA que levou ao fim do guest.
void traceCall(const char* name, const PPCContext& ctx);  // antes da impl
void traceReturn(const char* name, const PPCContext& ctx); // depois
/** Despeja as últimas chamadas registradas (ordem) com um motivo. */
void traceDump(const char* reason);

} // namespace fh2::kern

#endif // FH2_HAS_RECOMP
