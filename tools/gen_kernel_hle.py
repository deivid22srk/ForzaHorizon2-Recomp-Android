#!/usr/bin/env python3
"""Gera runtime/ppc/kernel_hle.cpp para os imports do xboxkrnl.exe/xam.xex
referenciados por ppc_func_mapping.cpp (FH2).

Três categorias:
  REAL    — delega para fh2::kern::real_<Nome> em kernel_real.cpp
            (semântica real: heap, tempo, threads, sync, TLS, printf, input)
  FAILURE — stub que retorna status de falha REAL (o estado honesto do
            sistema: recurso indisponível), nunca "sucesso vazio"
  default — stub log-once + NTSTATUS 0 (semântica pendente, issue #16)

NÃO EDITAR kernel_hle.cpp À MÃO — rode: python3 tools/gen_kernel_hle.py
"""
import re, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GEN = os.path.join(ROOT, "recomp", "generated")
OUT = os.path.join(ROOT, "app", "src", "main", "cpp", "runtime", "ppc", "kernel_hle.cpp")

mapping = set()
with open(f'{GEN}/ppc_func_mapping.cpp') as f:
    for m in re.finditer(r'\{ 0x[0-9A-Fa-f]+, __imp__([A-Za-z0-9_]+) \}', f.read()):
        mapping.add(m.group(1))

defined = set()
for name in os.listdir(GEN):
    if name.endswith('.cpp'):
        with open(f'{GEN}/{name}', errors='ignore') as f:
            for m in re.finditer(r'(?:PPC_FUNC_IMPL\(|PPC_WEAK_FUNC\(|void )__imp__([A-Za-z0-9_]+)', f.read()):
                defined.add(m.group(1))

needed = sorted(mapping - defined)
print(f'mapping={len(mapping)} defined={len(defined)} needed={len(needed)}')

# ---- implementações reais (kernel_real.cpp) ----
REAL = {
    'DbgBreakPoint', 'DbgPrint',
    'ExAllocatePool', 'ExAllocatePoolTypeWithTag', 'ExFreePool',
    'ExCreateThread', 'ExTerminateThread',
    'HalReturnToFirmware',
    'KeBugCheck', 'KeBugCheckEx',
    'KeDelayExecutionThread',
    'KeGetCurrentProcessType', 'KeSetCurrentProcessType',
    'KeInitializeMutant', 'KeReleaseMutant',
    'KeInitializeSemaphore', 'KeReleaseSemaphore',
    'KeInitializeTimerEx', 'KeSetTimer', 'KeCancelTimer',
    'KeQueryPerformanceFrequency', 'KeQuerySystemTime',
    'KeResetEvent', 'KeResumeThread', 'KeSetEvent',
    'KeTlsAlloc', 'KeTlsFree', 'KeTlsGetValue', 'KeTlsSetValue',
    'KeWaitForMultipleObjects', 'KeWaitForSingleObject',
    'MmAllocatePhysicalMemoryEx', 'MmFreePhysicalMemory',
    'MmGetPhysicalAddress', 'MmQueryAllocationSize',
    'MmCreateKernelStack', 'MmDeleteKernelStack',
    'NtAllocateVirtualMemory', 'NtFreeVirtualMemory',
    'NtQueryVirtualMemory',
    'NtCreateFile', 'NtOpenFile', 'NtReadFile', 'NtWriteFile',
    'NtReadFileScatter', 'NtQueryInformationFile',
    'NtQueryVolumeInformationFile', 'NtSetInformationFile',
    'NtFlushBuffersFile', 'NtQueryFullAttributesFile',
    'XexGetModuleHandle', 'XexGetModuleSection',
    'XexCheckExecutablePrivilege', 'MmQueryStatistics',
    'NtResumeThread', 'NtSuspendThread',
    'KeSetAffinityThread', 'KeSetBasePriorityThread',
    'ObReferenceObjectByHandle', 'ObDereferenceObject',
    'XGetAVPack', 'ExGetXConfigSetting',
    'NtClearEvent', 'NtClose', 'NtCreateEvent', 'NtCreateMutant',
    'NtCreateSemaphore', 'NtReleaseMutant', 'NtReleaseSemaphore',
    'NtSetEvent', 'NtSignalAndWaitForSingleObjectEx',
    'NtWaitForMultipleObjectsEx', 'NtWaitForSingleObjectEx',
    'NtYieldExecution',
    'NtDeviceIoControlFile',
    'FscSetCacheElementCount',
    'XamContentGetLicenseMask',
    'XeCryptSha', 'ObCreateSymbolicLink',
    'RtlCompareMemory', 'RtlCompareMemoryUlong', 'RtlFillMemoryUlong',
    'RtlEnterCriticalSection', 'RtlInitializeCriticalSection',
    'RtlInitializeCriticalSectionAndSpinCount', 'RtlLeaveCriticalSection',
    'RtlTryEnterCriticalSection',
    'RtlFreeAnsiString', 'RtlInitAnsiString', 'RtlInitUnicodeString',
    'RtlMultiByteToUnicodeN', 'RtlNtStatusToDosError',
    'RtlTimeFieldsToTime', 'RtlTimeToTimeFields',
    'RtlUnicodeStringToAnsiString', 'RtlUnicodeToMultiByteN',
    'RtlUpcaseUnicodeChar',
    '_snprintf', '_vsnprintf', 'sprintf', 'vsprintf', 'vswprintf',
    'XamInputGetCapabilities', 'XamInputGetCapabilitiesEx',
    'XamInputGetState', 'XamInputSetState',
    'XamGetCurrentTitleId',
    'XamLoaderGetLaunchDataSize', 'XamLoaderGetLaunchData',
    'XamLoaderSetLaunchData', 'XamLoaderLaunchTitle',
    'XamLoaderTerminateTitle',
    'XamUserGetSigninState',
    'XexLoadImage', 'XexUnloadImage', 'XexGetProcedureAddress',
    # vídeo real (issue #17 — backend Vulkan/GLES + estado Vd em kernel_real.cpp)
    'VdSwap',
    'VdQueryVideoMode', 'VdQueryVideoFlags',
    'VdGetCurrentDisplayGamma', 'VdGetCurrentDisplayInformation',
    'VdSetSystemCommandBufferGpuIdentifierAddress', 'VdGetSystemCommandBuffer',
    'VdInitializeRingBuffer', 'VdEnableRingBufferRPtrWriteBack',
    'VdSetGraphicsInterruptCallback',
    'VdInitializeEngines', 'VdShutdownEngines', 'VdPersistDisplay',
    'VdEnableDisableClockGating', 'VdIsHSIOTrainingSucceeded',
    'VdRetrainEDRAM', 'VdRetrainEDRAMWorker',
    'VdSetDisplayMode', 'VdSetDisplayModeOverride',
    'VdInitializeScalerCommandBuffer',
}

# ---- falhas honestas: o recurso real não existe neste boot ----
# (sucesso-vazio derrubaria o guest com NULL/garbage downstream)
FAILURE = {
    # sistema de arquivos: iteração real (FsProvider) = próxima fase
    'NtCreateFile':            (0xC0000034, 'STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente'),
    'NtOpenFile':              (0xC0000034, 'STATUS_OBJECT_NAME_NOT_FOUND — FS real pendente'),
    'NtReadFile':              (0xC0000001, 'sem handle real (FS pendente)'),
    'NtWriteFile':             (0xC0000001, 'sem handle real (FS pendente)'),
    'NtQueryDirectoryFile':    (0xC0000001, 'FS pendente'),
    'NtQueryFullAttributesFile': (0xC0000034, 'FS pendente'),
    'NtQueryInformationFile':  (0xC0000001, 'FS pendente'),
    'NtQueryVolumeInformationFile': (0xC0000001, 'FS pendente'),
    'NtSetInformationFile':    (0xC0000001, 'FS pendente'),
    'NtFlushBuffersFile':      (0xC0000001, 'FS pendente'),
    'NtCancelIoFile':          (0xC0000001, 'IO pendente'),
    'NtReadFileScatter':       (0xC0000001, 'FS pendente'),
    'NtWriteFileGather':       (0xC0000001, 'FS pendente'),
    'StfsControlDevice':       (0xC0000001, 'STFS pendente'),
    'StfsCreateDevice':        (0xC0000001, 'STFS pendente'),
    'XamContentCreateEx':      (0x803500F1, 'sem pacote de conteúdo montado'),
    'XamContentOpenFile':      (0x803500F1, 'sem pacote de conteúdo montado'),
    'XamContentResolve':       (0x803500F1, 'sem pacote de conteúdo montado'),
    'XamContentGetDeviceState': (0x803500F1, 'sem pacote de conteúdo montado'),
    'XamCacheOpenFile':        (0x803500F1, 'cache pendente'),
    'XamUserGetXUID':          (0x80320098, 'sem perfil assinado (offline real)'),
    'XamUserGetName':          (0x80320098, 'sem perfil assinado (offline real)'),
    'XamUserGetSigninInfo':    (0x80320098, 'sem perfil assinado (offline real)'),
    'XNetStartup':             (0x800704CF, 'rede real pendente (issue #19)'),
    'NetDll_WSAStartup':       (0x800704CF, 'rede real pendente (issue #19)'),
    'XexGetProcedureAddress':  (0x8007007E, 'STATUS_PROCEDURE_NOT_FOUND — export tables pendentes'),
}

# semântica pendente — mantém stub 0 com nota específica
NOTES = {
    'VdSwap': 'present do GPU — issue #17 (gráficos reais)',
    'XAudioSubmitRenderDriverFrame': 'áudio do guest — issue #18',
    'RtlRaiseException': 'SEH do guest não suportado (issue #15)',
}

hdr = f'''// kernel_hle.cpp — despacho HLE dos imports de xboxkrnl.exe/xam.xex (FH2)
//
// GERADO por tools/gen_kernel_hle.py — {len(needed)} símbolos exigidos por
// ppc_func_mapping.cpp. Três caminhos:
//   REAL  ({sum(1 for n in needed if n in REAL)}): delega para fh2::kern::real_* com semântica real
//         (heap, tempo, threads, sync, TLS, printf, input — kernel_real.cpp)
//   FAIL  ({sum(1 for n in needed if n in FAILURE)}): retorna status de falha REAL do estado do sistema
//   stub  ({len(needed) - sum(1 for n in needed if n in REAL) - sum(1 for n in needed if n in FAILURE)}): log-once + NTSTATUS 0 (semântica pendente — issue #16)
// NÃO EDITAR À MÃO.
#if FH2_HAS_RECOMP

#include <android/log.h>
#include "ppc_config.h"
#include "ppc_context.h"
#include "runtime/ppc/kernel_real.h"
#include "runtime/ppc/kernel_state.h"

#define HLOG(...) __android_log_print(ANDROID_LOG_WARN, "FH2/HLE", __VA_ARGS__)

#define FH2_HLE_ONCE(name, note) do {{ static bool _logged_##name = false; if (!_logged_##name) {{ _logged_##name = true; HLOG("HLE stub: %s — %s", #name, note); }} }} while (0)

'''

body = []
n_real = n_fail = n_stub = 0
for name in needed:
    if name in REAL:
        n_real += 1
        body.append(f'// {name}: semântica REAL (kernel_real.cpp)')
        body.append(f'void __imp__{name}(PPCContext& ctx, uint8_t* base) {{')
        body.append(f'    fh2::kern::traceCall("{name}", ctx);')
        body.append(f'    fh2::kern::real_{name}(ctx, base);')
        body.append(f'    fh2::kern::traceReturn("{name}", ctx);')
    elif name in FAILURE:
        n_fail += 1
        status, why = FAILURE[name]
        body.append(f'// {name}: falha REAL — {why}')
        body.append(f'void __imp__{name}(PPCContext& ctx, uint8_t* base) {{')
        body.append(f'    (void)base;')
        body.append(f'    fh2::kern::traceCall("{name}", ctx);')
        body.append(f'    FH2_HLE_ONCE({name}, "{why}");')
        body.append(f'    ctx.r3.u32 = 0x{status:08X}u;')
        body.append(f'    fh2::kern::traceReturn("{name}", ctx);')
    else:
        n_stub += 1
        note = NOTES.get(name, 'semântica real pendente (issue #16)')
        body.append(f'// {name}: {note}')
        body.append(f'void __imp__{name}(PPCContext& ctx, uint8_t* base) {{')
        body.append(f'    (void)base;')
        body.append(f'    fh2::kern::traceCall("{name}", ctx);')
        body.append(f'    FH2_HLE_ONCE({name}, "{note}");')
        body.append('    ctx.r3.u32 = 0;')
        body.append(f'    fh2::kern::traceReturn("{name}", ctx);')
    body.append('}')
    body.append('')

with open(OUT, 'w') as f:
    f.write(hdr + '\n'.join(body) + '\n#endif // FH2_HAS_RECOMP\n')
print(f'escrito: {OUT}  REAL={n_real} FAIL={n_fail} STUB={n_stub}')
