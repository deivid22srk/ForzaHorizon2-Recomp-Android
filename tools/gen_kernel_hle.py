#!/usr/bin/env python3
"""Gera runtime/ppc/kernel_hle.cpp com stubs HLE para todos os imports do
xboxkrnl.exe/xam.xex referenciados por ppc_func_mapping.cpp (388 no FH2)."""
import re, os

GEN = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "recomp", "generated")
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "app", "src", "main", "cpp", "runtime", "ppc", "kernel_hle.cpp")

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

NOTES = {
    'KeBugCheck': 'caminho fatal do guest',
    'KeBugCheckEx': 'caminho fatal do guest',
    'HalReturnToFirmware': 'guest pediu reboot',
    'RtlRaiseException': 'exceptions de guest não suportadas (issue #15)',
    'DbgPrint': 'debug output do guest',
    'ExAllocatePool': 'retorna NULL até o allocator HLE (issue #16)',
    'ExAllocatePoolTypeWithTag': 'retorna NULL até o allocator HLE (issue #16)',
    'KeWaitForMultipleObjects': 'retorna imediatamente (sync HLE pendente)',
    'KeDelayExecutionThread': 'retorna imediatamente (sync HLE pendente)',
    'XAudioSubmitRenderDriverFrame': 'áudio do guest — issue #18',
}
FATAL = {'KeBugCheck', 'KeBugCheckEx', 'HalReturnToFirmware', 'RtlRaiseException'}

hdr = f'''// kernel_hle.cpp — stubs HLE dos imports de xboxkrnl.exe/xam.xex usados pelo FH2
//
// GERADO por scripts/gen_kernel_hle.py — {len(needed)} símbolos exigidos por
// ppc_func_mapping.cpp (ld.lld para no limite de 20 erros, mas o conjunto
// completo vem da análise do mapping). Cada stub: log-once + retorno padrão
// de sucesso (NTSTATUS 0). Semântica real = issue #16. NÃO EDITAR À MÃO.
#if FH2_HAS_RECOMP

#include <android/log.h>
#include "ppc_config.h"
#include "ppc_context.h"

#define HLOG(...) __android_log_print(ANDROID_LOG_WARN, "FH2/HLE", __VA_ARGS__)

#define FH2_HLE_ONCE(name, note) do {{ static bool _logged_##name = false; if (!_logged_##name) {{ _logged_##name = true; HLOG("HLE stub: %s — %s", #name, note); }} }} while (0)

'''

body = []
for name in needed:
    note = NOTES.get(name, 'semântica real pendente (issue #16)')
    if name in FATAL:
        lvl = 'HLOG("HLE FATAL: %s — %s", "%s", "%s");'
        body.append(f'// {name}: {note}')
        body.append(f'void __imp__{name}(PPCContext& ctx, uint8_t* base) {{')
        body.append(f'    (void)base;')
        body.append(f'    HLOG("HLE FATAL: %s — {note}", "{name}");')
        body.append('    ctx.r3.u32 = 0;')
    else:
        body.append(f'// {name}: {note}')
        body.append(f'void __imp__{name}(PPCContext& ctx, uint8_t* base) {{')
        body.append(f'    (void)base;')
        body.append(f'    FH2_HLE_ONCE({name}, "{note}");')
        body.append('    ctx.r3.u32 = 0;')
    body.append('}')
    body.append('')

with open(OUT, 'w') as f:
    f.write(hdr + '\n'.join(body) + '\n#endif // FH2_HAS_RECOMP\n')
print(f'escrito: {OUT}')
