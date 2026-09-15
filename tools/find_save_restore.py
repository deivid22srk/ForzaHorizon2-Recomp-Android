#!/usr/bin/env python3
"""Localiza __restgprlr_14/__savegprlr_14/restfpr/savefpr/restvmx/savevmx no
XEX descriptografado via padrões de bytes documentados no README do XenonRecomp.

Uso: python3 tools/find_save_restore.py [path/to/default_dec.xex]
"""
import sys

XEX = sys.argv[1] if len(sys.argv) > 1 else 'recomp/private/default_dec.xex'
LOAD = 0x82000000  # load address do FH2 (XexTool -l confirma)
EXE_OFF = 0x4000   # exe_offset do header XEX2

PATTERNS = {
    'restgprlr_14_address': bytes.fromhex('e9c1ff68'),  # ld r14, -0x98(r1)
    'savegprlr_14_address': bytes.fromhex('f9c1ff68'),  # std r14, -0x98(r1)
    'restfpr_14_address':   bytes.fromhex('c9ccff70'),  # lfd f14, -0x90(r12)
    'savefpr_14_address':   bytes.fromhex('d9ccff70'),  # stfd f14, -0x90(r12)
    'restvmx_14_address':   bytes.fromhex('3960fee07dcb60ce'),  # li r11,-0x120; lvx v14,r11,r12
    'savevmx_14_address':   bytes.fromhex('3960fee07dcb61ce'),  # li r11,-0x120; stvx v14,r11,r12
    'restvmx_64_address':   bytes.fromhex('3960fc00100b60cb'),  # li r11,-0x400; lvx128 v64,r11,r12
    'savevmx_64_address':   bytes.fromhex('3960fc00100b61cb'),  # li r11,-0x400; stvx128 v64,r11,r12
}

def main():
    data = open(XEX, 'rb').read()
    print(f"{'função':<22} {'endereço virtual':>18}  (file offset)")
    for name, pat in PATTERNS.items():
        hits, start = [], EXE_OFF
        while True:
            i = data.find(pat, start)
            if i < 0:
                break
            hits.append(i)
            start = i + 1
        if not hits:
            print(f"{name:<22}  NÃO ENCONTRADO")
            continue
        for h in hits:
            print(f"{name:<22} {LOAD + (h - EXE_OFF):#x}  ({h:#x})")
    print()

if __name__ == '__main__':
    main()
